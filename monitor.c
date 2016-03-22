#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

double getTime(clock_t t1, clock_t t2) {
	double t = 0;
	t = (t2 - t1) / CLOCKS_PER_SEC;
	if (t < 0) t = t * -1;
	return t;
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

int SleepExp()
{
	int s;
	long ns;
	long n;
	float r;
	float Lambda = 0.01;

	r = (float) rand() / ((float) RAND_MAX +1);
	n = (long) -1 / Lambda * log(1 - r ) * 1000000;

	ns = n % 1000000;
	s = (long) n / 1000000000L;
	nanosleep((struct timespec[]){{s, ns}}, NULL);
}


int main(int argc, char *argv[])
{
	Randomize();
	if (argc < 3 ) {
		printf("Uso: monitor [id] [K(vm)/X(en)/L(xc)]\n");
		return 0;
	}
	int TempExp = 0;
	TempExp = atoi(argv[3]);
	char s[200];
	int rel, dns;
	rel = atoi(argv[1]);

	printf("Arquivo: relatorio%03d.txt\n", rel);
        sprintf(s, "echo 'insert into dados (exp, c, r, d,  cliente, amostra, tinicial, ip, tdns, wget, tfinal) values ' > res/relatorio%03d.txt", rel);
        system(s);
	int tipo=0;
	if (strcmp(argv[4],"d")==0)
		//apenas dns
		tipo=1;
	else if (strcmp(argv[4],"w")==0)
		//apenas wget
		tipo=-1;

	int i = 0;
	int n = 0;
	time_t inicio, atual;
	clock_t t1, t2;
	inicio = time(NULL);
	atual = time(NULL);

	while (difftime(atual, inicio) < TempExp)  {
		i++;
		SleepExp();
		n = RndI(0, 99);
		if (i>1) {
	                memset(s,'\0', sizeof(s));
        	        sprintf(s,"echo \\, >> res/relatorio%03d.txt", rel);
                	system(s);
		}
		memset(s,'\0', sizeof(s));
		sprintf(s,"echo -n \\(@ex\\, @cl\\, @rt\\, @dns\\, \\'%d\\'\\,\\'%d\\' >> res/relatorio%03d.txt", rel, i, rel);
		system(s);
	 
		strcpy(s,"");
       		sprintf(s, "echo -n \\,\\'$(date +%%s.%%N)\\'\\, >> res/relatorio%03d.txt", rel);
		system(s);

		strcpy(s,"");
		if (tipo>=0) 
			sprintf(s, "echo -n \\'$(dig @192.168.0.%d registro.br +short +time=3 +tries=1 >&-)\\'\\, >> res/relatorio%03d.txt", dns, rel);
		else
                        sprintf(s, "echo -n \\'wget only\\'\\, >> res/relatorio%03d.txt", rel);
		system(s);
		
		strcpy(s,"");
	        sprintf(s, "echo -n \\'$(date +%%s.%%N)\\'\\, >> res/relatorio%03d.txt", rel);
		system(s);

                strcpy(s,"");
                if (tipo<=0) 	
			sprintf(s,"echo -n \\'$(wget 172.17.132.60/%d.png -nv --delete-after -O res/tmp%d  2>&1)\\'\\, >> res/relatorio%03d.txt", n, rel, rel);
		else
			sprintf(s,"echo -n \\'DNS only\\'\\, >> res/relatorio%03d.txt", rel);
                system(s);
		
		strcpy(s,"");
		sprintf(s, "echo -n \\'$(date +%%s.%%N)\\'\\) >> res/relatorio%03d.txt", rel);
		system(s);
		
		atual = time(NULL);
	}
        memset(s,'\0', sizeof(s));
        sprintf(s,"echo \\; >> res/relatorio%03d.txt", rel);
        system(s);

	return 0;
}


