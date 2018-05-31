#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

// Parâmetros iniciais
#define XMIN -2.5
#define XMAX 1.0
#define YMIN -1.0
#define YMAX 1.0
#define ITERATIONS 1000

// Dimensões da malha no plano
#define WIDTH 5600
#define HEIGHT 3200

// Estrutura para o contexto de execução de cada thread
struct PointsCtx {
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	int iterations;
	double *data;
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
	for (int p = ctx->from; p < 3*ctx->to; p += 3){
		j = (p/3) % ctx->width;
		i = (p/3) / ctx->width;
      ctx->data[p] = ctx->xmin + (ctx->xmax - ctx->xmin) * j / (ctx->width-1);
      ctx->data[p+1] = ctx->ymin + (ctx->ymax - ctx->ymin) * i / (ctx->height-1);
		ctx->data[p+2] = iter(ctx->data[p], ctx->data[p+1], ctx->iterations)/ctx->iterations;
	}
	return NULL;
}

int main(){
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
	ctx[0].M = ctx[0].width*ctx[0].height / 2;
	// Conjunto de pontos e respectivas iterações
	ctx[0].data = malloc(3*ctx[0].M*sizeof(double));
	ctx[0].from = 0;
	ctx[0].to = ctx[0].M / 2;

	// Contexto thread 1
	ctx[1].xmin = XMIN;
	ctx[1].xmax = XMAX;
	ctx[1].ymin = (YMIN + YMAX) / 2;
	ctx[1].ymax = YMAX;
	ctx[1].iterations = ITERATIONS;
	ctx[1].width = WIDTH;
	ctx[1].height = HEIGHT / 2;
	// Quantidade de pontos para o thread
	ctx[1].M = ctx[1].width*ctx[1].height / 2;
	// Conjunto de pontos e respectivas iterações
	ctx[1].data = malloc(3*ctx[1].M*sizeof(double));
	ctx[1].from = ctx[1].M / 2;
	ctx[1].to = ctx[1].M;

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

	printf("Quantidade de pontos gerados: %d\n", ctx[0].M + ctx[1].M);

}
