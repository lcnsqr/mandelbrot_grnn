#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include "idxio.h"

// Parâmetros iniciais
#define XMIN -2.5
#define XMAX 1.0
#define YMIN -1.0
#define YMAX 1.0
#define ITERATIONS 1000

// Dimensões da malha no plano
#define WIDTH 5600
#define HEIGHT 3200

// Arquivo das amostras
#define TRAIN_X "train_x.idx"
#define TRAIN_Y "train_y.idx"
// Arquivo das amostras de teste
#define TEST_X "test_x.idx"
#define TEST_Y "test_y.idx"

// Estrutura para o contexto de execução de cada thread
struct PointsCtx {
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	int iterations;
	double *data_x;
	double *data_y;
	int M;
	int width;
	int height;
	int from;
	int to;
};

// Valor aleatório uniforme em [0,1)
#define RAND ((float)(rand() >> 1)/((RAND_MAX >> 1) + 1))

// cx, cy: Ponto candidato a estar no conjunto
// Limite de iterações: iterations
double iter(double cx, double cy, int iterations){
	double x = 0, y = 0;
	double xx = 0, yy = 0, xy = 0;
	int i = 0;
	while (i < iterations && xx + yy <= 4){
		xy = x * y;
		xx = x * x;
		yy = y * y;
		x = xx - yy + cx;
		y = xy + xy + cy;
		i++;
	}
	return (double)i;
}

// Gerar parcial do total de pontos (thread separado) para o conjunto de treinamento
// vCtx: ponteiro para a estrutura com os parâmetros para gerar os dados
void *train_data(void *vCtx){
	struct PointsCtx *ctx = (struct PointsCtx *)vCtx;
	int i, j;
	// Pontos formam uma grade no plano complexo
	for (int p = ctx->from; p < ctx->to; p++){
		j = p % ctx->width;
		i = p / ctx->width;
		// No eixo real
      ctx->data_x[2*p] = ctx->xmin + (ctx->xmax - ctx->xmin) * j / (ctx->width-1);
		// No eixo imaginário
      ctx->data_x[2*p+1] = ctx->ymin + (ctx->ymax - ctx->ymin) * i / (ctx->height-1);
		// Iterações (variável dependente)
		ctx->data_y[p] = iter(ctx->data_x[2*p], ctx->data_x[2*p+1], ctx->iterations)/ctx->iterations;
	}
	return NULL;
}

// Gerar parcial do total de pontos (thread separado) para o conjunto de teste
// vCtx: ponteiro para a estrutura com os parâmetros para gerar os dados
void *test_data(void *vCtx){
	struct PointsCtx *ctx = (struct PointsCtx *)vCtx;
	// Pontos aleatórios no plano complexo
	for (int p = ctx->from; p < ctx->to; p++){
		// No eixo real
      ctx->data_x[2*p] = ctx->xmin + RAND * (ctx->xmax - ctx->xmin);
		// No eixo imaginário
      ctx->data_x[2*p+1] = ctx->ymin + RAND * (ctx->ymax - ctx->ymin);
		// Iterações (variável dependente)
		ctx->data_y[p] = iter(ctx->data_x[2*p], ctx->data_x[2*p+1], ctx->iterations)/ctx->iterations;
	}
	return NULL;
}

int main(){
	// Semente aleatória
	srand((unsigned int)time(NULL));

	// Variáveis independentes (coordenadas no plano complexo) do conjunto de treinamento
	double *train_data_x = malloc(2*WIDTH*HEIGHT*sizeof(double));
	// Variáveis dependentes (iterações para cada ponto no plano) do conjunto de treinamento
	double *train_data_y = malloc(WIDTH*HEIGHT*sizeof(double));

	// Variáveis independentes (coordenadas no plano complexo) do conjunto de teste
	// 1/10 do conjunto de treinamento
	double *test_data_x = malloc(2*WIDTH*HEIGHT*sizeof(double)/10);
	// Variáveis dependentes (iterações para cada ponto no plano) do conjunto de teste
	// 1/10 do conjunto de treinamento
	double *test_data_y = malloc(WIDTH*HEIGHT*sizeof(double)/10);

	// Contexto dos threads do conjunto de treinamento
	struct PointsCtx trainCtx[2];
	// Contexto dos threads do conjunto de treinamento
	struct PointsCtx testCtx[2];

	// Contexto thread 0 (treinamento)
	trainCtx[0].xmin = XMIN;
	trainCtx[0].xmax = XMAX;
	trainCtx[0].ymin = YMIN;
	trainCtx[0].ymax = (YMIN + YMAX) / 2;
	trainCtx[0].iterations = ITERATIONS;
	trainCtx[0].width = WIDTH;
	trainCtx[0].height = HEIGHT / 2;
	// Quantidade de pontos para o thread
	trainCtx[0].M = trainCtx[0].width * trainCtx[0].height;
	// Memória compartilhada
	trainCtx[0].data_x = train_data_x;
	trainCtx[0].data_y = train_data_y;
	trainCtx[0].from = 0;
	trainCtx[0].to = trainCtx[0].M;

	// Contexto thread 1 (treinamento)
	trainCtx[1].xmin = XMIN;
	trainCtx[1].xmax = XMAX;
	trainCtx[1].ymin = (YMIN + YMAX) / 2;
	trainCtx[1].ymax = YMAX;
	trainCtx[1].iterations = ITERATIONS;
	trainCtx[1].width = WIDTH;
	trainCtx[1].height = HEIGHT / 2;
	// Quantidade de pontos para o thread
	trainCtx[1].M = trainCtx[1].width * trainCtx[1].height;
	// Memória compartilhada
	trainCtx[1].data_x = train_data_x;
	trainCtx[1].data_y = train_data_y;
	trainCtx[1].from = trainCtx[1].M;
	trainCtx[1].to = 2 * trainCtx[1].M;

	// Contexto thread 0 (teste)
	testCtx[0].xmin = XMIN;
	testCtx[0].xmax = XMAX;
	testCtx[0].ymin = YMIN;
	testCtx[0].ymax = (YMIN + YMAX) / 2;
	testCtx[0].iterations = ITERATIONS;
	testCtx[0].width = WIDTH;
	testCtx[0].height = HEIGHT / 2;
	// Quantidade de pontos para o thread
	testCtx[0].M = (testCtx[0].width * testCtx[0].height) / 10;
	// Memória compartilhada
	testCtx[0].data_x = test_data_x;
	testCtx[0].data_y = test_data_y;
	testCtx[0].from = 0;
	testCtx[0].to = testCtx[0].M;

	// Contexto thread 1 (teste)
	testCtx[1].xmin = XMIN;
	testCtx[1].xmax = XMAX;
	testCtx[1].ymin = (YMIN + YMAX) / 2;
	testCtx[1].ymax = YMAX;
	testCtx[1].iterations = ITERATIONS;
	testCtx[1].width = WIDTH;
	testCtx[1].height = HEIGHT / 2;
	// Quantidade de pontos para o thread
	testCtx[1].M = (testCtx[1].width * testCtx[1].height) / 10;
	// Memória compartilhada
	testCtx[1].data_x = test_data_x;
	testCtx[1].data_y = test_data_y;
	testCtx[1].from = testCtx[1].M;
	testCtx[1].to = 2 * testCtx[1].M;

	// Dois threads para gerar o conjunto de treinamento
	pthread_t train_t[2];

	// Dois threads para gerar o conjunto de teste
	pthread_t test_t[2];

	// Primeiro thread do conjunto de teste
	if(pthread_create(&test_t[0], NULL, test_data, &testCtx[0])) {
		fprintf(stderr, "Erro criando o thread 0 do conjunto de teste\n");
		return -1;
	}

	// Segundo thread do conjunto de teste
	if(pthread_create(&test_t[1], NULL, test_data, &testCtx[1])) {
		fprintf(stderr, "Erro criando o thread 1 do conjunto de teste\n");
		return -1;
	}

	// Primeiro thread do conjunto de treinamento
	if(pthread_create(&train_t[0], NULL, train_data, &trainCtx[0])) {
		fprintf(stderr, "Erro criando o thread 0 do conjunto de treinamento\n");
		return -1;
	}

	// Segundo thread do conjunto de treinamento
	if(pthread_create(&train_t[1], NULL, train_data, &trainCtx[1])) {
		fprintf(stderr, "Erro criando o thread 1 do conjunto de treinamento\n");
		return -1;
	}

	// Esperar término dos threads
	if(pthread_join(test_t[0], NULL)) {
		fprintf(stderr, "Error joining thread 0\n");
		return -1;
	}
	if(pthread_join(test_t[1], NULL)) {
		fprintf(stderr, "Error joining thread 1\n");
		return -1;
	}
	if(pthread_join(train_t[0], NULL)) {
		fprintf(stderr, "Error joining thread 0\n");
		return -1;
	}
	if(pthread_join(train_t[1], NULL)) {
		fprintf(stderr, "Error joining thread 1\n");
		return -1;
	}

	// Armazenar amostras do conjunto de treinamento

	// Variáveis independentes
	struct Idx idx_train_x;
	// Tipo double (8 bytes)
	idx_train_x.type = 0x0e;
	idx_train_x.dimCount = 2;
	// Tamanho das dimensões é int (4 bytes)
	idx_train_x.dimSizes = malloc(idx_train_x.dimCount*4);
	idx_train_x.dimSizes[0] = WIDTH*HEIGHT;
	// Cada ponto possui apenas duas coordenadas
	idx_train_x.dimSizes[1] = 2;
	idx_train_x.size = 1;
	for (int d = 0; d < idx_train_x.dimCount; d++) idx_train_x.size *= idx_train_x.dimSizes[d];
	idx_train_x.data.d = train_data_x;
	idxSave(TRAIN_X, &idx_train_x);

	// Variáveis dependentes
	struct Idx idx_train_y;
	// Tipo double (8 bytes)
	idx_train_y.type = 0x0e;
	idx_train_y.dimCount = 2;
	// Tamanho das dimensões é int (4 bytes)
	idx_train_y.dimSizes = malloc(idx_train_y.dimCount*4);
	idx_train_y.dimSizes[0] = WIDTH*HEIGHT;
	// A variável dependente é um valor real
	idx_train_y.dimSizes[1] = 1;
	idx_train_y.size = 1;
	for (int d = 0; d < idx_train_y.dimCount; d++) idx_train_y.size *= idx_train_y.dimSizes[d];
	// Copiar as coordenadas dos dois contextos
	idx_train_y.data.d = train_data_y;
	idxSave(TRAIN_Y, &idx_train_y);

	// Armazenar amostras do conjunto de teste

	// Variáveis independentes
	struct Idx idx_test_x;
	// Tipo double (8 bytes)
	idx_test_x.type = 0x0e;
	idx_test_x.dimCount = 2;
	// Tamanho das dimensões é int (4 bytes)
	idx_test_x.dimSizes = malloc(idx_test_x.dimCount*4);
	idx_test_x.dimSizes[0] = WIDTH*HEIGHT/10;
	// Cada ponto possui apenas duas coordenadas
	idx_test_x.dimSizes[1] = 2;
	idx_test_x.size = 1;
	for (int d = 0; d < idx_test_x.dimCount; d++) idx_test_x.size *= idx_test_x.dimSizes[d];
	idx_test_x.data.d = test_data_x;
	idxSave(TEST_X, &idx_test_x);

	// Variáveis dependentes
	struct Idx idx_test_y;
	// Tipo double (8 bytes)
	idx_test_y.type = 0x0e;
	idx_test_y.dimCount = 2;
	// Tamanho das dimensões é int (4 bytes)
	idx_test_y.dimSizes = malloc(idx_test_y.dimCount*4);
	idx_test_y.dimSizes[0] = WIDTH*HEIGHT/10;
	// A variável dependente é um valor real
	idx_test_y.dimSizes[1] = 1;
	idx_test_y.size = 1;
	for (int d = 0; d < idx_test_y.dimCount; d++) idx_test_y.size *= idx_test_y.dimSizes[d];
	// Copiar as coordenadas dos dois contextos
	idx_test_y.data.d = test_data_y;
	idxSave(TEST_Y, &idx_test_y);

	printf("Quantidade de pontos do conjunto de treinamento: %d\n", trainCtx[0].M + trainCtx[1].M);
	printf("Quantidade de pontos do conjunto de teste: %d\n", testCtx[0].M + testCtx[1].M);

}
