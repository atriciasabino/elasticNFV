#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


int main(int argc, char *argv[])
{
	if (argc < 5 ) {
		printf("Uso: monitor [id] [intervalo(s)] [tempo(s)] [Tag]\n");
		return 0;
	}
	int id;
	int Intervalo = 10;
        char tag[20];
	int tempo=0;
	tempo = atoi(argv[3]);
	Intervalo = atoi(argv[2]);
	memcpy(tag, argv[4], 20);
        id = atoi(argv[1]);
	char s[200];
	FILE *arq_in;
	FILE *arq_out;
	char t[3];
	int cpu[10];
	int b;
	int i=0;
	char mem[10];
	unsigned int memT;
        unsigned int memF;
	float carga=0;
	char unid[3];
	char linha[255];
	unsigned int net[16];
	unsigned int offsetIn[2][8];
        unsigned int offsetOut[2][8];
	unsigned int netIn[2][8];
	unsigned int netOut[2][8];
	unsigned int conntrack=0;
	char arquivo[]="relatorio0000.txt";
	sprintf(arquivo, "relatorio%04u.txt", id);

        arq_out = fopen(arquivo,"w");

        if (arq_out != NULL) {
		//fprintf(arq_out, "Inicio:\n");
        	fclose(arq_out);
	} else {
                printf("Erro criando relatorio...\n");
                return 0;
	}

	//definindo off-set da rede
        arq_in = fopen("/proc/net/dev","r");
        if (arq_in == NULL) {
                printf("Erro\n");
        	return 0;
        } else {
                while ( (fscanf(arq_in, "%s %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u", 
                            linha, &net[0],&net[1],&net[2],&net[3],&net[4],&net[5],&net[6],&net[7],&net[8],&net[9],&net[10],&net[11],&net[12],&net[13],&net[14],&net[15])) != EOF) {
                	if (strncmp(linha, "eth0",4) == 0 ){
                        	//printf("\n%s:\n",linha);
                                for (i=0; i<8; i++) {
                                	offsetIn[0][i] = net[i];
                                        offsetOut[0][i] = net[i+8];
                               	}
                        }
                        if (strncmp(linha, "eth1",4) == 0 ){
                                //printf("\n%s:\n",linha);
                                for (i=0; i<8; i++) {
                                        offsetIn[1][i] = net[i];
                                        offsetOut[1][i] = net[i+8];
                                }                                                       
                        }
                }
                fclose(arq_in);
	}



//	memcpy(net,net_old,sizeof(net));


	tempo += (unsigned)time(NULL);
	while (tempo >= (unsigned)time(NULL)) {
		sleep(Intervalo);
		// CPU
		arq_in = fopen("/proc/stat","r");
		if (arq_in == NULL) {
			printf("Erro\n");
			return 0;
		} else {
			//printf("Lido:\n");
			fscanf(arq_in, "%s %d %d %d %d %d %d %d", t, &cpu[0], &cpu[1], &cpu[2], &cpu[3], &cpu[4], &cpu[5], &cpu[6]);
                        fclose(arq_in);
		}
                carga = 0;
                if ( (cpu[0]+cpu[2]+cpu[3]) != 0 )
                        carga = (float) (cpu[0]+cpu[2])*100 / (cpu[0]+cpu[2]+cpu[3]);

		// Memoria
                arq_in = fopen("/proc/meminfo","r");
                if (arq_in == NULL) {
                        printf("Erro\n");
                        return 0;
                } else {
                        //printf("Lido:\n");
                        fscanf(arq_in, "%s %u %s", mem, &memT, unid);
			fscanf(arq_in, "%s %u %s", mem, &memF, unid);
                        fclose(arq_in);
                }

                // Rede
                arq_in = fopen("/proc/net/dev","r");
                if (arq_in == NULL) {
                        printf("Erro\n");
                        return 0;
                } else {
			while ( (fscanf(arq_in, "%s %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u", 
						linha, &net[0],&net[1],&net[2],&net[3],&net[4],&net[5],&net[6],&net[7],&net[8],&net[9],&net[10],&net[11],&net[12],&net[13],&net[14],&net[15])) != EOF) {
				if (strncmp(linha, "eth0",4) == 0 ){
					//printf("\n%s:\n",linha);
					for (i=0; i<8; i++) {
						netIn[0][i] = net[i] - offsetIn[0][i];
						netOut[0][i] = net[i+8] - offsetOut[0][i];
                                                offsetIn[0][i] = net[i]; 
                                                offsetOut[0][i] = net[i+8]; 
					}
				}
                                if (strncmp(linha, "eth1",4) == 0 ){
                                        //printf("\n%s:\n",linha);
                                        for (i=0; i<8; i++) {
                                                netIn[1][i] = net[i] - offsetIn[1][i];
                                                netOut[1][i] = net[i+8] - offsetOut[1][i];
                                                offsetIn[1][i] = net[i];
                                                offsetOut[1][i] = net[i+8];

                                        }                                                       
                                }
			}
                        fclose(arq_in);
                }

                // Conntrack
                conntrack=0;
		arq_in = fopen("/proc/sys/net/netfilter/nf_conntrack_count","r");
                if (arq_in == NULL) {
                        //printf("Erro\n");
                        //return 0;
			conntrack=0;
			//printf("erro conntrack\n");
                } else {
                        //printf("Lido:\n");
                        fscanf(arq_in, "%u", &conntrack);
                        fclose(arq_in);
                }


		// Grava a saída dos dados...
                arq_out = fopen(arquivo,"a");
                if (arq_out == NULL) {
                        printf("Erro\n");
                        return 0;
		} else {
			// id; timestamp; cpu; memTotal; memUsada; eth0 bytes; pct; drop; out bytes; pct; drop;  eth1 bytes; pct; drop; out bytes; pct; drop; conntrack 
                        fprintf(arq_out, "%u; %u; %f; %u; %u; %u; %u; %u; %u; %u; %u; %u; %u; %u; %u; %u; %u; %u; %u; %s\n", 
					id, (unsigned)time(NULL), carga, memT, memF, (memT - memF), 
					netIn[0][0], netIn[0][1], netIn[0][3], netOut[0][0], netOut[0][1], netOut[0][3],
					netIn[1][0], netIn[1][1], netIn[1][3], netOut[1][0], netOut[1][1], netOut[1][3],
					conntrack, tag);
                        fclose(arq_out);
		}

	}

	return 0;
}

