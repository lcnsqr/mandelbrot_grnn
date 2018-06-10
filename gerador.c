#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include "idxio.h"

// Parâmetros iniciais
#define XMIN -2.5
#define XMAX 1.0
#define YMIN 1.0
#define YMAX -1.0
#define ITERATIONS 1000

// Dimensões da malha no plano
#define WIDTH 5600
#define HEIGHT 3200
#define DEPTH 4

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
#define RAND ((double)(rand() >> 1)/((RAND_MAX >> 1) + 1))

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
// ctx: ponteiro para a estrutura com os parâmetros para gerar os dados
void train_data(struct PointsCtx *ctx){
	int i, j;
	// Cor
	unsigned int c, rgba[4];
	// Pontos formam uma grade no plano complexo
	for (int p = ctx->from; p < ctx->to; p++){
		j = p % ctx->width;
		i = p / ctx->width;
		// No eixo real
      ctx->data_x[2*p] = ctx->xmin + (ctx->xmax - ctx->xmin) * j / ctx->width;
		// No eixo imaginário
      ctx->data_x[2*p+1] = ctx->ymin + (ctx->ymax - ctx->ymin) * i / ctx->height;
		// Iterações (variável dependente)
		ctx->data_y[p] = iter(ctx->data_x[2*p], ctx->data_x[2*p+1], ctx->iterations)/ctx->iterations;
	}
}

// Gerar parcial do total de pontos (thread separado) para o conjunto de teste
// ctx: ponteiro para a estrutura com os parâmetros para gerar os dados
void test_data(struct PointsCtx *ctx){
	// Pontos aleatórios no plano complexo
	for (int p = ctx->from; p < ctx->to; p++){
		// No eixo real
      ctx->data_x[2*p] = ctx->xmin + RAND * (ctx->xmax - ctx->xmin);
		// No eixo imaginário
      ctx->data_x[2*p+1] = ctx->ymin + RAND * (ctx->ymax - ctx->ymin);
		// Iterações (variável dependente)
		ctx->data_y[p] = iter(ctx->data_x[2*p], ctx->data_x[2*p+1], ctx->iterations)/ctx->iterations;
	}
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

	// Contexto do conjunto de treinamento
	struct PointsCtx trainCtx;
	// Contexto do conjunto de treinamento
	struct PointsCtx testCtx;

	// Contexto (treinamento)
	trainCtx.xmin = XMIN;
	trainCtx.xmax = XMAX;
	trainCtx.ymin = YMIN;
	trainCtx.ymax = YMAX;
	trainCtx.iterations = ITERATIONS;
	trainCtx.width = WIDTH;
	trainCtx.height = HEIGHT;
	// Quantidade de pontos para o thread
	trainCtx.M = trainCtx.width * trainCtx.height;
	// Memória compartilhada
	trainCtx.data_x = train_data_x;
	trainCtx.data_y = train_data_y;
	trainCtx.from = 0;
	trainCtx.to = trainCtx.M;

	// Contexto (teste)
	testCtx.xmin = XMIN;
	testCtx.xmax = XMAX;
	testCtx.ymin = YMIN;
	testCtx.ymax = YMAX;
	testCtx.iterations = ITERATIONS;
	testCtx.width = WIDTH;
	testCtx.height = HEIGHT;
	// Quantidade de pontos para o thread
	testCtx.M = (testCtx.width * testCtx.height) / 10;
	// Memória compartilhada
	testCtx.data_x = test_data_x;
	testCtx.data_y = test_data_y;
	testCtx.from = 0;
	testCtx.to = testCtx.M;

	// Conjunto de treinamento
	train_data(&trainCtx);

	// Conjunto de teste
	test_data(&testCtx);

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

	printf("Quantidade de pontos do conjunto de treinamento: %d\n", trainCtx.M);
	printf("Quantidade de pontos do conjunto de teste: %d\n", testCtx.M);

}
