#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#define PORTA 5999
#define TRUE 1
#define FALSE 0
#define INTERVALO 10
#define MAXCLI 128
#define MAXVNF 32

//Estruturas
typedef struct {
	unsigned int ts; //timestamp
	int nivel; //0 a 100
	unsigned int conntrack;
	float cpu;
	float memoria;
	unsigned int inEth0;
	unsigned int outEth0;
	unsigned int inEth1;
	unsigned int outEth1;
} registro;

typedef struct {
	int id;
	unsigned char ativo;
	char ip[16];
	int nivel;
	char erro;
	registro estado;
} registroVNF;

typedef struct {
	int id;
	unsigned char ativo;
	char ip[16];
	int processos;
	registroVNF *vnf;
} registroCLI;


// Funcoes
int logVNF(void);
int createLogs(void);
int getEstado(registro*, char* );
int getQtdCli(registroVNF* );
int run(char*, char*, int);
void *runCli();
void *runPing();
void *TrUpdateVNFstatus();
void *Monitor();
void Move(int, int);
int addVNF();
int getMinVNF();
int getMaxVNF();
void delVNF(int);



// Variaveis
registroVNF vnf[MAXVNF];
registroCLI cli[MAXCLI];
char fim = FALSE;
char tag[20];
sem_t semCli;
sem_t semVNF;
pthread_t trUpVNF;
pthread_t trMonitor;
pthread_t trCli[MAXCLI];
pthread_t trCli2[MAXCLI];
pthread_t trPing[MAXCLI];
int LimSup, LimInf;
int Tolerancia;
int qtdVNF;

int main(int argc, char *argv[]) {
	if (argc!=8) {
		printf("Uso: gestor [tempo] [Alvo] [lim.Sup] [lim.Inf] [tolerancia] [qtdVNF] [Tag] \n");
		return 0;
	}
        FILE *arq;
        char comando[255];
        int tempo=30;
        char arquivo[]="alfas.txt";
        char alvo[16];
	createLogs();

        tempo = atoi(argv[1]);
        strcpy(alvo, argv[2]);
	LimSup = atoi(argv[3]);
        LimInf = atoi(argv[4]);
        Tolerancia = atoi(argv[5]);
        qtdVNF = atoi(argv[6]);
        strcpy(tag, argv[7]);
	if (qtdVNF>MAXVNF || qtdVNF<1) {
		printf("Qtd. de VNFs não permitido!");
		return 0;
	}

        sem_init(&semCli, 0, 1);
	sem_init(&semVNF, 0, 1);

	int i;
	for (i=0; i<MAXVNF; i++) {
		vnf[i].id = i+1;
		vnf[i].ativo = FALSE;
		vnf[i].nivel=0;
		vnf[i].erro = FALSE;
		memset(vnf[i].ip, ' ', 16);
		sprintf(vnf[i].ip, "192.168.%d.1", (i+1));
	}
	vnf[0].ativo=TRUE;
	for (i=0; i<MAXCLI; i++) {
		cli[i].id = i+1;
		cli[i].ativo = FALSE;
                memset(cli[i].ip, ' ', 16);
                sprintf(cli[i].ip, "192.168.0.%d", i+101);
		cli[i].vnf = &vnf[0];
	}
	//abrir arquivo distribuicao pracas
        float alfa[MAXCLI];
	arq = fopen(arquivo,"r");
	int qtdAlfa=0;
        while ( (fscanf(arq, "%f", &alfa[qtdAlfa])) != EOF && qtdAlfa<MAXCLI) qtdAlfa++;
        fclose(arq);


        pthread_create( &trUpVNF, NULL, TrUpdateVNFstatus, NULL); //lanca thread que atualiza as tabelas de estado...

	//loop iniciando clientes
	for (i=0; i<qtdAlfa; i++){
		printf("Iniciando cliente %03d:alfa=%0.3f\n", (i+1), alfa[i]);
		// Cliente A
                sem_wait(&semCli);
                memset(comando, ' ', 255);
                sprintf(comando, "ssh root@cl%d \"./cliente %d %d %s %0.3f %s 1\"", 
                                               (i+1), (i+1), tempo, alvo, alfa[i], tag);
	        pthread_create( &trCli[i], NULL, runCli, &comando); //lanca thread que atualiza as tabelas de estado...
                cli[i].processos=1;
		// Cliente B
		sem_wait(&semCli);
                memset(comando, ' ', 255);
                sprintf(comando, "ssh root@cl%d \"./cliente %d %d %s %0.3f %s 0\"", 
                                                (i+1), (i+128), tempo, alvo, alfa[i], tag);
                pthread_create( &trCli2[i], NULL, runCli, &comando); //lanca thread que atualiza as tabelas de estado...
		cli[i].processos++;
		cli[i].ativo = TRUE;
	}
	//thread monitor...
	pthread_create( &trMonitor, NULL, Monitor, NULL); //lanca thread manipula os clientes de acordo com as regras de balanceamento de carga...

	//Finaliza a simulacao...
	for (i=0; i<qtdAlfa; i++)
		pthread_join(trCli[i], NULL);
        printf("Clientes A encerrados...\n");

        for (i=0; i<qtdAlfa; i++)
                pthread_join(trCli2[i], NULL);
        printf("Clientes B encerrados...\n");

        fim = TRUE;
        pthread_join(trMonitor, NULL);
        pthread_join(trUpVNF, NULL);
        printf("FIM.\n");
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

void Move(int idCli, int idVNF){
	char comando[255];
        char saida[255];
	memset(comando, ' ', 255);
        sprintf(comando, "ssh root@cl%d \"route del -net 0.0.0.0/0; route add -net 0.0.0.0/0 gw %s\"", (idCli+1), vnf[idVNF].ip);
        sem_wait(&semCli);
	run(comando, saida, 255);
	cli[idCli].vnf = &vnf[idVNF];
        sem_post(&semCli);
}


int createLogs() {
        char logFile[]="vnf.dat";
        FILE *arq_out;
        arq_out = fopen(logFile,"w");
        if (arq_out == NULL) {
                printf("Erro gravando log de VNF.\n");
                return 0;
        } else {
                //fprintf(arq_out, "id; status; timestamp local; nivel; qtdcli; erro; ts remoto; cpu; memoria; eth0 in; eth0 out; eth1 in; eth1 out; tag\n");
                fclose(arq_out);
        }
        return 1;
}



int logVNF() {
	char logFile[]="vnf.dat";
	FILE *arq_out;
	arq_out = fopen(logFile,"a");
        if (arq_out == NULL) {
                printf("Erro gravando log de VNF.\n");
                return 0;
	} else {
		int i;
		for (i=0; i<qtdVNF; i++) {
			// id; status; timestamp local; nivel; qtdcli; erro; ts remoto; cpu; memoria; eth0 in; eth0 out; eth1 in; eth1 out; tag 
			fprintf(arq_out, "%d;%d;%u;%d;%d;%d;%u;%0.3f;%0.3f;%u;%u;%u;%u;%s\n", vnf[i].id, vnf[i].ativo, (unsigned)time(NULL), 
						vnf[i].nivel, getQtdCli(&vnf[i]),vnf[i].erro, vnf[i].estado.ts, vnf[i].estado.cpu, vnf[i].estado.memoria,
						vnf[i].estado.inEth0, vnf[i].estado.outEth0, vnf[i].estado.inEth1, vnf[i].estado.outEth1, tag);
		}
                fclose(arq_out);
	}
	return 1;
}



int getQtdCli(registroVNF *v){
	int i, c=0;
	for (i=0; i<MAXCLI; i++){
		if (cli[i].ativo && cli[i].vnf==v) c++;
	}
	return c;
}

void *runCli(void *arg){
	char com[255];
	strcpy(com, arg);
        sem_post(&semCli);
        FILE *fp;
        fp = popen(com, "r");
        if (fp == NULL) //erro
                return;
        pclose(fp);
        return;
}

void *runPing(void *arg){
        char com[255];
        strcpy(com, arg);
        sem_post(&semCli);
// 	printf("-- %s --\n", com);
        FILE *fp;
        fp = popen(com, "r");
        if (fp == NULL) //erro
                return;
        pclose(fp);
        return;
}



void *TrUpdateVNFstatus(){
	int i;
	registro rec;
	
        int sockfd = 0, n = 0;
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORTA);
	while (TRUE) {
//		printf("v.");
		if (fim) break;
		for (i=0; i<qtdVNF; i++){
			if (vnf[i].ativo) {
				memset(&rec, '0' ,36);
			        if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0) {
                			printf("\n Error : Could not create socket \n");
                		} else {
					serv_addr.sin_addr.s_addr = inet_addr(vnf[i].ip);
        				if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) {
						vnf[i].erro = TRUE;
        				} else {
	        				if ((n = read(sockfd, &rec, 36)) > 0) {
							//atualiza estado da vnf
					        	sem_wait(&semVNF);
							vnf[i].nivel = rec.nivel;
							memcpy(&vnf[i].estado, &rec, sizeof(rec));
							sem_post(&semVNF);
        					}
					}
					close(sockfd);
				}
			}
		}
		logVNF();
		sleep(INTERVALO);
	}
	printf("\nFim Thread VNF\n");
}

int addVNF(){
        sem_wait(&semVNF);
	int i;
	for (i=0; i<qtdVNF; i++) {
		if (!vnf[i].ativo) {
			vnf[i].nivel=0;
			vnf[i].ativo=1;
		        sem_post(&semVNF);
			return TRUE;
		}
	}
        sem_post(&semVNF);
	return FALSE;
}
void delVNF(int i){
        sem_wait(&semVNF);
	vnf[i].nivel = 0;
	vnf[i].ativo = FALSE;
        sem_post(&semVNF);
	printf("\ndel!!!!\n");
	return;
}

int getMinVNF(){
        sem_wait(&semVNF);
	int i, m=-1;
	for (i=0; i<qtdVNF; i++){
		if (m<0 && vnf[i].ativo) m=i;
		if (m>=0) 
			if (vnf[i].nivel < vnf[m].nivel && vnf[i].ativo) 
				m=i; 
	}
        sem_post(&semVNF);
	return m;
}

int getMaxVNF(){
        sem_wait(&semVNF);
        int i, m=-1;
        for (i=0; i<qtdVNF; i++){
                if (m<0 && vnf[i].ativo) m=i;
                if (m>=0) 
                        if (vnf[i].nivel > vnf[m].nivel && vnf[i].ativo) 
                                m=i; 
        }
        sem_post(&semVNF);
        return m;
}

void *Monitor(){
        int i, j, n, min, max;
	int qtd;
	int carga;
        while (TRUE) {
                if (fim) break;
                sem_wait(&semVNF);
		qtd=0;
		carga=0;
                for (i=0; i<MAXVNF; i++) {
			if (vnf[i].ativo) {
				qtd++;
				carga += vnf[i].nivel;
			}
		}
                sem_post(&semVNF);

		if (carga >= qtd*(LimSup+Tolerancia)) {
 			//sobrecarga
			if (addVNF()) qtd++;
		}
		
                if (qtd>1 && carga <= qtd*LimInf) {
                        //sub-utilizacao
			min = getMinVNF();
			for (i=0; i<qtdVNF; i++) {
			        sem_wait(&semVNF);
				if (vnf[i].ativo && i!=min && (vnf[i].nivel+vnf[min].nivel) < (LimSup+Tolerancia) ) {
					//transferir todos
					sem_post(&semVNF);
					for (j=0; j<MAXCLI; j++) {
						if (cli[j].vnf == &vnf[min]) 
							Move(j, i);
					}
					delVNF(min);
					break;
				} else
				        sem_post(&semVNF);
			}
		}
		//balancear quantidade de clientes...
                max = getMaxVNF();
                min = getMinVNF();
		if (getQtdCli(&vnf[max]) > (int) getQtdCli(&vnf[min]) * (1+ (float) (Tolerancia/100) ) ) {
			//diferenca e grande...vamos transferir
			n = (int) (getQtdCli(&vnf[max]) - getQtdCli(&vnf[min]))/2;
			if (n>0) {
				i = 0;
				while (i<MAXCLI && n>0) {
					if (cli[i].vnf == &vnf[max]) {
						Move(i, min);
						n--;
					}
					i++;
				}
			}
		}
                sleep(INTERVALO);
        }
        printf("\nFim Thread Monitor\n");
}



int getEstado(registro* rec, char* ip){
        int sockfd = 0, n = 0;
        struct sockaddr_in serv_addr;
        memset(rec, '0' ,36);

        if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
        {
                printf("\n Error : Could not create socket \n");
                return 1;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORTA);
        serv_addr.sin_addr.s_addr = inet_addr(ip);


        if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) {
                printf("\n Error : Connect Failed \n");
                return 1;
        }

        while ((n = read(sockfd, rec, 36)) > 0) {
                //recvBuff[n] = 0;
		//printf("%d\n",n);
		break;
        }


        if( n < 0) {
                printf("\n Read Error \n");
        }
}

