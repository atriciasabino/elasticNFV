#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>

int run(char*, char*, int);
unsigned int getSize(char*, int);
double getNow(void);
int RndI( int, int);
void Randomize( void);
void SleepExp(void);
sem_t sem;
float alfa;
char alvo[15]; 
char tag[20];
char nomearq[30];
int id;

void *cliente(void *);


int main(int argc, char *argv[])
{
	FILE *arq_out;
        int TempExp = 0;
        char s[200];
        int i = 0;
        int j;
	
	Randomize();
	pthread_t th_id[10000]; //10 mil threads 

	sem_init(&sem, 0, 1);

	if (argc != 6 ) {
		printf("Uso: cliente [id] [tempo] [Alvo] [alfa] [Tag]\n");
		return 0;

	}

        id = atoi(argv[1]);
        TempExp = atoi(argv[2]);
        strcpy(alvo, argv[3]);
        alfa = 1;
        alfa = atof(argv[4]);
        memcpy(tag, argv[5], 20);

	sprintf(nomearq, "res/cliente%04u.txt", id);
        arq_out = fopen(nomearq,"w");
        if (arq_out != NULL) {
                fclose(arq_out);
        } else {
                printf("Erro criando relatorio...\n");
                return 0;
        }

        TempExp += (unsigned)time(NULL);
        while (TempExp >= (unsigned)time(NULL)) {
		SleepExp();
		i++;
		pthread_create( &th_id[i-1], NULL, cliente, (void *)&i);
	}
	//printf("Concluindo as threads... \n");
	for (j=0; j<i; j++) {
		pthread_join(th_id[j], NULL);
	}

	return 0;
}


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

double getNow(){
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (double) (tv.tv_sec) + (double)(tv.tv_usec) / 1000000;
}

void Randomize( void) {
    srand( time( NULL));
}

int RndI( int min, int max) {
    int k;
    double d;
    d = (double) rand( ) / ((double) RAND_MAX + 1);
    k = d * (max - min + 1);
    return min + k;
}

void SleepExp()
{
        int s;
        long ns;
        float r;
	float resto;
        float e;
        r = (float) rand() / ((float) RAND_MAX +1);
        e =  -1 * log(1 - r) / alfa;
        resto = (float) e - (int) e;
        s = (int) e;
        ns = (long) (resto * 1000000000L); 
//	printf("s:%d ns:%lu\n",s,ns);
        nanosleep((struct timespec[]){{s, ns}}, NULL);
	return;
}

void * cliente( void * arg){
	int i = * (int *) arg;
        double Ti, Tdns, Twget;
        unsigned int Timestamp;
	FILE *arq_out;
        int n;
        char s[200];
        char saida[255];
        Timestamp = (unsigned)time(NULL);
        Ti = getNow();
        n = RndI(0, 99);
        strcpy(s,"");
        sprintf(s,"wget %s/%d.png -nv --delete-after -O res/tmp%s%04d%06d  2>&1", alvo, n, tag, id, i);

        if (run(s, saida, 255) < 0) 
                Twget=0;
        else
                Twget = getNow() - Ti - Tdns;

        sem_wait(&sem);
        arq_out = fopen(nomearq,"a");
        if (arq_out == NULL) {
                printf("Erro gravando relatorio na thread:%s...\n",nomearq);
                return 0;
        }
        fprintf(arq_out,"%u; %s; %u; %u; %.6f; %.6f; %.6f; %d; %s", id, tag, Timestamp, i, Ti,(double) 0, Twget, getSize(saida, sizeof(saida)), saida);
	fclose(arq_out);
        sem_post(&sem);

        return;
        //id, tag, timestamp, amostra, Inicio, TempoDns, TempoWget, Tamanho, 'SaidaWget'
}


