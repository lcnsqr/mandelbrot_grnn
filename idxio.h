/*
 * Funções para entrada/saída de arquivos IDX
 * para armazenar vetores multidimensionais.
 * Referência: http://yann.lecun.com/exdb/mnist/
 */

// Tamanho padrão de palavra (4 bytes)
#define WSIZE 8

// Structure that holds IDX data
struct Idx {
	// Tipos de variáveis:
	// 0x08: char (unsigned byte)
	// 0x0C: int (4 bytes)
	// 0x0D: float (4 bytes)
	// 0x0E: double (8 bytes)
	char type;
	// Número de dimensões
	char dimCount;
	// Número de elementos para cada dimensão 
	unsigned int *dimSizes;
	// Quantidade total de elementos
	unsigned int size;
	// Ponteiro para os dados
	union {
		char *c;
		int *i;
		float *f;
		double *d;
	} data;
};

void idxLoad(const char* filename, struct Idx *idx){
	// Buffer no tamanho da palavra
	int *w = malloc(WSIZE);
	// Ponteiro do arquivo de entrada
	FILE *fp;
	fp = fopen(filename, "r");
	// Magic number
	fread(w, WSIZE, 1, fp);
	// Tipo dos dados
	idx->type = (*w & 0x0000FF00) >> 8;
	// Contagem de dimensões
	idx->dimCount = (*w & 0x0000000FF);
	// Liberar buffer
	free(w);
	// Número de elementos em cada dimensão
	idx->dimSizes = malloc(idx->dimCount*WSIZE);
	fread(idx->dimSizes, WSIZE, idx->dimCount, fp);
	// Número de elementos em todas as dimensões
	idx->size = 1;
	for (int d = 0; d < idx->dimCount; d++) idx->size *= idx->dimSizes[d];
	// Ler dados
	switch ( idx->type ){
		case 0x08:
			// unsigned char
			idx->data.c = malloc(idx->size);
			fread(idx->data.c, idx->size, 1, fp);
		break;
		case 0x0c:
			// integer
			idx->data.i = malloc(idx->size * sizeof(int));
			fread(idx->data.i, idx->size * sizeof(int), 1, fp);
		break;
		case 0x0d:
			// float
			idx->data.f = malloc(idx->size * sizeof(float));
			fread(idx->data.f, idx->size * sizeof(float), 1, fp);
		break;
		case 0x0e:
			// double
			idx->data.d = malloc(idx->size * sizeof(double));
			fread(idx->data.d, idx->size * sizeof(double), 1, fp);
		break;
	}
	// Liberar arquivo aberto
	fclose(fp);
}

void idxSaveHeader(const char* filename, struct Idx *idx){
	// Buffer da palavra
	int *w = malloc(WSIZE);
	*w = idx->type << 8;
	*w |= idx->dimCount;
	// Ponteiro do arquivo
	FILE *fp;
	fp = fopen(filename, "w");
	// Escrever Magic number e contagem de dimensões
	fwrite(w, WSIZE, 1, fp);
	// Liberar buffer
	free(w);
	// Tamanho de cada dimensão
	fwrite(idx->dimSizes, WSIZE, idx->dimCount, fp);
	// Fechar ponteiro do arquivo
	fclose(fp);
}

void idxSaveData(const char* filename, struct Idx *idx){
	// Ponteiro do arquivo
	FILE *fp;
	fp = fopen(filename, "w");
	// Escrever dados
	switch ( idx->type ){
		case 0x08:
			// unsigned char
			fwrite(idx->data.c, idx->size, 1, fp);
		break;
		case 0x0c:
			// integer
			fwrite(idx->data.i, idx->size * sizeof(int), 1, fp);
		break;
		case 0x0d:
			// float
			fwrite(idx->data.f, idx->size * sizeof(float), 1, fp);
		break;
		case 0x0e:
			// double
			fwrite(idx->data.d, idx->size * sizeof(double), 1, fp);
		break;
	}
	// Fechar ponteiro do arquivo
	fclose(fp);
}

void idxSave(const char* filename, struct Idx *idx){
	// Buffer da palavra
	int *w = malloc(WSIZE);
	*w = idx->type << 8;
	*w |= idx->dimCount;
	// Ponteiro do arquivo
	FILE *fp;
	fp = fopen(filename, "w");
	// Escrever Magic number e contagem de dimensões
	fwrite(w, WSIZE, 1, fp);
	// Liberar buffer
	free(w);
	// Tamanho de cada dimensão
	fwrite(idx->dimSizes, WSIZE, idx->dimCount, fp);
	// Escrever dados
	switch ( idx->type ){
		case 0x08:
			// unsigned char
			fwrite(idx->data.c, idx->size, 1, fp);
		break;
		case 0x0c:
			// integer
			fwrite(idx->data.i, idx->size * sizeof(int), 1, fp);
		break;
		case 0x0d:
			// float
			fwrite(idx->data.f, idx->size * sizeof(float), 1, fp);
		break;
		case 0x0e:
			// double
			fwrite(idx->data.d, idx->size * sizeof(double), 1, fp);
		break;
	}
	// Fechar ponteiro do arquivo
	fclose(fp);
}
