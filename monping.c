#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int run(char* comando, char* saida, int max){
	FILE *fp;
	fp = popen(comando, "r");
	if (fp == NULL) //erro
		return -1;
	fgets(saida, max -1, fp);
	pclose(fp);
	return 0;
}

unsigned int getSize(char* ss, int length) {
	int c=0, l=0;
	char s[20];
	while (ss[c] != '[' && c<length) c++;
	if (c<length) //achei
		while (ss[c+l] != '/' && (c+l)<length) l++;

	if ((c+l) < length) {
		memcpy(s, ss+c+1, l-1);
		return atoi(s);
	} else return 0;
}

float getRTT(char* ss, int length) {
        int c=0, l=0;
        char s[20];
        while (ss[c] != '(' && c<length) c++;
        if (c<length) //achei
                while (ss[c+l] != ')' && (c+l)<length) l++;

        if ((c+l) < length) {
                memcpy(s, ss+c+1, l-1);
                return atof(s);
        } else return 0;
}

double getNow(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double) (tv.tv_sec) + (double)(tv.tv_usec) / 1000000;
}

int main(int argc, char *argv[])
{
	if (argc < 5 ) {
		printf("Uso: monping [id] [tempo] [intervalo] [Alvo] [Tag]\n");
		return 0;
	}

        int id = 0;
        id = atoi(argv[1]);

        int TempExp = 0;
        TempExp = atoi(argv[2]);

        int Intervalo = 0;
        Intervalo = atoi(argv[3]);

        char alvo[] = "               ";
        strcpy(alvo, argv[4]);

        char tag[20];
        memcpy(tag, argv[5], 20);

	char s[200];
	FILE *arq_out;

	char nomearq[19];
	char saida[255];

	sprintf(nomearq, "res/ping%04d.txt", id);
        arq_out = fopen(nomearq,"w");
        if (arq_out != NULL) {
                fclose(arq_out);
        } else {
                printf("Erro criando relatorio...\n");
                return 0;
        }

	int i = 0;
	int n = 0;

	double Ti;
	unsigned int Timestamp;

	arq_out = fopen(nomearq,"a");
	if (arq_out == NULL) {
		printf("Erro gravando relatorio...\n");
		return 0;
	}


        TempExp += (unsigned)time(NULL);
	int ci, cc;
	char TiStr[10];

        strcpy(s,"");
        sprintf(s,"fping %s -e -a -t 200 2>&1", alvo);

        while (TempExp >= (unsigned)time(NULL)) {
		i++;
		sleep(Intervalo);
 		Timestamp = (unsigned)time(NULL);
		Ti = 0;
		strcpy(saida,"erro\n");
 		if (run(s, saida, 255) == 0) {
			Ti=getRTT(saida, sizeof(saida));
		}

		fprintf(arq_out,"%u; %s; %u; %u; %.6f; %s", id, tag, Timestamp, i, Ti, saida);

	        //id, tag, timestamp, amostra, Tempo, 'Saida fping'
	}
	fclose(arq_out);
	return 0;
}


