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
#define PORTA 5999
#define MAXCONNTRACK 8000

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

unsigned int inEth0offset;
unsigned int outEth0offset;
unsigned int inEth1offset;
unsigned int outEth1offset;

int setOffSet(void);

int getInfo(registro*);



int main(void)
{
	setOffSet();
	int listenfd = 0,connfd = 0;

	struct sockaddr_in serv_addr;
	registro sendBuff;

	int numrv;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	printf("socket retrieve success\n");

	memset(&serv_addr, '0', sizeof(serv_addr));
	memset(&sendBuff, '0', sizeof(sendBuff));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORTA);

	bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));

	if(listen(listenfd, 10) == -1){
		printf("Failed to listen\n");
      		return -1;
	}


	while(1) {
		connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL); // accept awaiting request
//                printf("%u %u\n", sendBuff.inEth0, sendBuff.outEth0 );
		getInfo(&sendBuff);
		sendBuff.ts = (unsigned)time(NULL);
		write(connfd, &sendBuff, sizeof(sendBuff));
      		close(connfd);
		sleep(1);
    	}
  	return 0;
}

int getInfo(registro* rec){
	//coleta as informacoes e preenche a struct
	FILE *arq_in;
        char linha[255];
        unsigned int net[16];
	int cpu[10];
	char mem[10];
	unsigned int memT;
        unsigned int memF;
	char unid[3];


	//dados da CPU
	arq_in = fopen("/proc/stat","r");
	if (arq_in == NULL) {
		printf("Erro\n");
		return 0;
	} else {
		fscanf(arq_in, "%s %d %d %d %d %d %d %d", linha, &cpu[0], &cpu[1], &cpu[2], &cpu[3], &cpu[4], &cpu[5], &cpu[6]);
                fclose(arq_in);
	}
        rec->cpu=0;
        if ( (cpu[0]+cpu[2]+cpu[3]) != 0 )
                rec->cpu = (float) (cpu[0]+cpu[2])*100 / (cpu[0]+cpu[2]+cpu[3]);

	// Memoria
        arq_in = fopen("/proc/meminfo","r");
        if (arq_in == NULL) {
                printf("Erro\n");
                        return 0;
        } else {
                fscanf(arq_in, "%s %u %s", mem, &memT, unid);
		fscanf(arq_in, "%s %u %s", mem, &memF, unid);
		rec->memoria = (memT-memF);
                fclose(arq_in);
        }

	// Conntrack
	arq_in = fopen("/proc/sys/net/netfilter/nf_conntrack_count","r");
        rec->conntrack=0;
        rec->nivel=0;
	if (arq_in != NULL) {
		fscanf(arq_in, "%u", &rec->conntrack);
                fclose(arq_in);
//		if (rec->conntrack >= MAXCONNTRACK)
//			rec->nivel = 100;
//		else {
			rec->nivel = (int) ((float)(rec->conntrack * 100)/MAXCONNTRACK);
			//printf("conntrack: %u\n", rec->conntrack);
                        //printf("relacao: %f\n", (float) (rec->conntrack*100)/MAXCONNTRACK);
			//printf("nivel: %d\n",rec->nivel);
//		}
	}
	//dados da rede
        arq_in = fopen("/proc/net/dev","r");
        if (arq_in == NULL) {
                printf("Erro\n");
                return 0;
        } else {
                while ( (fscanf(arq_in, "%s %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u", 
                            linha, &net[0],&net[1],&net[2],&net[3],&net[4],&net[5],&net[6],&net[7],&net[8],&net[9],&net[10],&net[11],&net[12],&net[13],&net[14],&net[15])) != EOF) {
                        if (strncmp(linha, "eth0",4) == 0 ){
	                        rec->inEth0 = net[0] - inEth0offset;
                                rec->outEth0 = net[8] - outEth0offset;
                                inEth0offset = net[0];
                                outEth0offset = net[8];
                        }
                        if (strncmp(linha, "eth1",4) == 0 ){
                                rec->inEth1 = net[0] - inEth1offset;
                                rec->outEth1 = net[8] - outEth1offset;
                                inEth1offset = net[0];
                                outEth1offset = net[8];
                        }
                }
                fclose(arq_in);
        }

}


int setOffSet(){
        FILE *arq_in;
	char linha[255];
	unsigned int net[16];

        arq_in = fopen("/proc/net/dev","r");
        if (arq_in == NULL) {
                printf("Erro\n");
        	return 0;
        } else {
                while ( (fscanf(arq_in, "%s %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u", 
                            linha, &net[0],&net[1],&net[2],&net[3],&net[4],&net[5],&net[6],&net[7],&net[8],&net[9],&net[10],&net[11],&net[12],&net[13],&net[14],&net[15])) != EOF) {
                	if (strncmp(linha, "eth0",4) == 0 ){
			        inEth0offset = net[0];
        			outEth0offset = net[8];
                        }
                        if (strncmp(linha, "eth1",4) == 0 ){
                                inEth1offset = net[0];
                                outEth1offset = net[8];
                        }
                }
                fclose(arq_in);
	}
}

