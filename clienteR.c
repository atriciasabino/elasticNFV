#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>

#define FALSE 0
#define TRUE 1


int run(char*, char*, int);
unsigned int getSize(char*, int);
double getNow(void);
int RndI( int, int);
void Randomize( void);
void SleepExp(void);
float getRTT(char*, int);

sem_t sem;
float alfa;
char alvo[16]; 
char tag[21];
char nomearq[255];
int id;
char fim = FALSE;

void *cliente(void *);
void *ping();

int main(int argc, char *argv[])
{
	FILE *arq_out;
        int TempExp = 0;
        char s[255];
        int i = 0;
        int j;
	float alfaMax=1;
        float alfaMin=0.05;
	int pg=0;
	Randomize();
	pthread_t th_id[20000]; //20 mil threads 
	pthread_t thPing;
	sem_init(&sem, 0, 1);

	if (argc != 7 ) {
		printf("Uso: cliente [id] [tempo] [Alvo] [alfa] [Tag] [ping:0/1]\n");
		return 0;

	}

        id = atoi(argv[1]);
        TempExp = atoi(argv[2]);
        strcpy(alvo, argv[3]);
        alfa = 1;
        alfaMax = atof(argv[4]);
        memcpy(tag, argv[5], 20);
	pg = atoi(argv[6]);

	if (pg) {
		sprintf(nomearq, "res/ping%04u.txt", id);
        	arq_out = fopen(nomearq,"w");
        	if (arq_out != NULL) {
                	fclose(arq_out);
        	} else {
                	printf("Erro criando relatorio...\n");
                	return 0;
		}
        }
        sprintf(nomearq, "res/cliente%04u.txt", id);
        arq_out = fopen(nomearq,"w");
        if (arq_out != NULL) {
                fclose(arq_out);
        } else {
                printf("Erro criando relatorio...\n");
                return 0;
        }

        int Estado=0;
        unsigned int TempEstado[5];
        //definir inicio (5% do experimento)
        float passo;
        alfa = alfaMin;
        passo = (alfaMax - alfaMin) / (TempExp * 0.4);
        TempEstado[0]=(unsigned)time(NULL) + (unsigned)(TempExp * 0.05);
        //definir rampa ascendente
        TempEstado[1]=(unsigned)time(NULL) + (unsigned)(TempExp * 0.45);
        //definir patamar
        TempEstado[2]=(unsigned)time(NULL) + (unsigned)(TempExp * 0.55);
        //definir rampa descendente
        TempEstado[3]=(unsigned)time(NULL) + (unsigned)(TempExp * 0.95);
        //definir finalizacao
        TempExp += (unsigned)time(NULL);
        unsigned int T1, T2;

        if (pg) pthread_create( &thPing, NULL, ping, NULL);

        while (TempExp >= (unsigned)time(NULL)) {
//		printf("\n...\n");
		//printf("Alfa %f\n",alfa);
                T2 = (unsigned)time(NULL);
                if (TempEstado[Estado] <= T2)
                    Estado++;
                switch (Estado) {
                        case 0:
                            alfa = alfaMin;
                            break;
                        case 1:
                            alfa += (float)(T2 - T1) * passo;
                            break;
                        case 2:
                            alfa = alfaMax;
                            break;
                        case 3:
                            alfa -= (float)(T2 - T1) * passo;
                            if (alfa==0) alfa = alfaMin;
                            break;
                        case 4:
                            alfa = alfaMin;
                            break;
                }
                T1 = T2;
                SleepExp();
		i++;
		pthread_create( &th_id[i-1], NULL, cliente, (void *)&i);
	}
	//printf("Concluindo as threads... \n");
	fim = TRUE;
	for (j=0; j<i; j++) {
		pthread_join(th_id[j], NULL);
	}
	if (pg) pthread_join(thPing, NULL);
	
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
        char s[30];
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
        char s[255];
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


void * ping(){
        FILE *arq_out;
        char nomearq[19];
        char saida[255];
	char comando[255];
	int i=0;
	unsigned int Timestamp;
	float Ti;
        memset(comando,' ',255);
        sprintf(comando,"fping %s -e -a -t 200 2>&1", alvo);
	while (!fim) {
		i++;
	        sprintf(nomearq, "res/ping%04d.txt", id);
	        arq_out = fopen(nomearq,"a");
	        if (arq_out != NULL) {
        	        Timestamp = (unsigned)time(NULL);
	                Ti = 0;
                	strcpy(saida,"erro\n");
                	if (run(comando, saida, 255) == 0) {
                        	Ti=getRTT(saida, sizeof(saida));
                	}
                	fprintf(arq_out,"%u; %s; %u; %u; %.6f; %s", id, tag, Timestamp, i, Ti, saida);
        	        fclose(arq_out);
        	}
		sleep(10);
        }
}

float getRTT(char* ss, int length) {
        int c=0, l=0;
        char s[30];
        while (ss[c] != '(' && c<length) c++;
        if (c<length) //achei
                while (ss[c+l] != ')' && (c+l)<length) l++;

        if ((c+l) < length) {
                memcpy(s, ss+c+1, l-1);
                return atof(s);
        } else return 0;
}


