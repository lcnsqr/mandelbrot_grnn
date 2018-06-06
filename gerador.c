#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
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

// Gerar parcial do total de pontos (thread separado)
// data: Pontos e respectivo número normalizado de iterações: [x,y,i/iterations]
// m: Quantidade de pontos
void *points_func(void *vCtx){
	struct PointsCtx *ctx = (struct PointsCtx *)vCtx;
	int i, j;
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

int main(){
	// Variáveis independentes (coordenadas no plano complexo)
	double *data_x = malloc(2*WIDTH*HEIGHT*sizeof(double));
	// Variáveis dependentes (iterações para cada ponto no plano)
	double *data_y = malloc(WIDTH*HEIGHT*sizeof(double));

	// Contexto dos threads
	struct PointsCtx ctx[2];

	// Contexto thread 0
	ctx[0].xmin = XMIN;
	ctx[0].xmax = XMAX;
	ctx[0].ymin = YMIN;
	ctx[0].ymax = (YMIN + YMAX) / 2;
	ctx[0].iterations = ITERATIONS;
	ctx[0].width = WIDTH;
	ctx[0].height = HEIGHT / 2;
	// Quantidade de pontos para o thread
	ctx[0].M = ctx[0].width * ctx[0].height;
	// Memória compartilhada
	ctx[0].data_x = data_x;
	ctx[0].data_y = data_y;
	ctx[0].from = 0;
	ctx[0].to = ctx[0].M;

	// Contexto thread 1
	ctx[1].xmin = XMIN;
	ctx[1].xmax = XMAX;
	ctx[1].ymin = (YMIN + YMAX) / 2;
	ctx[1].ymax = YMAX;
	ctx[1].iterations = ITERATIONS;
	ctx[1].width = WIDTH;
	ctx[1].height = HEIGHT / 2;
	// Quantidade de pontos para o thread
	ctx[1].M = ctx[1].width * ctx[1].height;
	// Memória compartilhada
	ctx[1].data_x = data_x;
	ctx[1].data_y = data_y;
	ctx[1].from = ctx[1].M;
	ctx[1].to = 2 * ctx[1].M;

	// Dois threads
	pthread_t points_t[2];

	// Primeiro thread
	if(pthread_create(&points_t[0], NULL, points_func, &ctx[0])) {
		fprintf(stderr, "Error creating thread 0\n");
		return -1;
	}

	// Segundo thread
	if(pthread_create(&points_t[1], NULL, points_func, &ctx[1])) {
		fprintf(stderr, "Error creating thread 1\n");
		return -1;
	}

	// Esperar término dos threads
	if(pthread_join(points_t[0], NULL)) {
		fprintf(stderr, "Error joining thread 0\n");
		return -1;
	}
	if(pthread_join(points_t[1], NULL)) {
		fprintf(stderr, "Error joining thread 1\n");
		return -1;
	}

	// Armazenar amostras

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
	idx_train_x.data.d = data_x;
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
	idx_train_y.data.d = data_y;
	idxSave(TRAIN_Y, &idx_train_y);
	/*
	idx.dimSizes[1] = 1;
	idx.size = 1;
	for (int d = 0; d < idx.dimCount; d++) idx.size *= idx.dimSizes[d];
	idx.data.f = train[1]._;
	idxSave(TRAIN_Y, &idx);
	*/

	printf("Quantidade de pontos gerados: %d\n", ctx[0].M + ctx[1].M);

}
