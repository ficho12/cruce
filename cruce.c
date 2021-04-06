#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include "cruce.h"

#define SEM_WAIT(x,n)\
  x.sem_num=n;\
  x.sem_op=-1;\
  x.sem_flg=0;

#define SEM_SIGNAL(x,n)\
  x.sem_num=n;\
  x.sem_op=1;\
  x.sem_flg=0;

#define LONGITUD 128

// struct para mensajes 
struct mensaje { 
    long tipo_msj; 
    char txt_msj[LONGITUD]; 
} message; 
struct datos{
	int procesos;
	int argumentos;
	pid_t pidDelPadre;
	pid_t pidDeLosHijos[1089];
}datos;

void ayuda()
{
    printf("La practica se invocara especificando dos parámetros obligatorios desde la linea de ordenes.\n"
	"El primer parametro sera el numero maximo de procesos que puede haber en ejecucion simultanea.\n"
	"El segundo consistira en un valor entero mayor o igual que cero. Si es 1 o mayor, la practica funcionara\n"
	"tanto mas lenta cuanto mayor sea el parametro y no debera consumir CPU apreciablemente.Si es 0, ira a la\n"
	"maxima velocidad, aunque el consumo de CPU sí será mayor");
}

int verify(char * string)
{
    int x = 0;
    int len = strlen(string);

    while(x < len) {
           if(!isdigit(*(string+x)))
           return 1;

           ++x;
    }

    return 0;
}

void terminar (int sig) {
	int i;
	pid_t self = getpid();
	if(datos.pidDelPadre != self) _exit(0);
	for(i=0; i< datos.procesos-1; i++){

		int status;
		for(;;){
			pid_t hijo = wait(&status);
			if(hijo > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0){
			} else if (hijo < 0 && errno == EINTR){
				continue;
			} else {
				perror("wait");
				abort();
			}
			break;
		}
	}
	printf("Programa acabado correctamente.\n");
	exit(0);
}


int main (int argc, char *argv[]){
	int semid, memid, numProc, velocidad;
	char * zona;
	datos.argumentos = argc;

	//Comprobamos que los parametros introducidos son los correctos
	if((datos.argumentos != 3)){
		printf("Error en el numero de parametros\n\n");
		ayuda();
		return 1;
	}

	if(!verify(argv[1]))
		numProc = atoi(*(argv+1));

	if(numProc < 1){
		printf("Error, el numero de procesos tiene que ser mayor o igual que uno.\n\n");
		ayuda();
		return 2;
	}

	if(!verify(argv[2]))
		velocidad = atoi(*(argv+2));

	if(velocidad < 0){
		printf("Error. El parametro velocidad debe ser igual o mayor que cero.\n\n");
		ayuda();
		return 3;
	}

	struct sigaction ss;
	ss.sa_handler = terminar;
	sigfillset(&ss.sa_mask);
	ss.sa_flags = 0;
	if(sigaction(SIGINT,&ss, NULL)==-1){
		perror("Error al configurar SIGINT.\n");
		exit(1);
	}
	
	semid=semget(IPC_PRIVATE, 1, IPC_CREAT|0600);
	memid=shmget(IPC_PRIVATE, 256, IPC_CREAT|0600);
	zona=shmat(memid, NULL, 0);
	
	printf("Hola\n");
	
	CRUCE_inicio(numProc, velocidad, semid, zona);

	for(;;)
	{
		
	}
	
	semctl(semid, 0, IPC_RMID);
	shmctl(memid, IPC_RMID, NULL);
	
	return 0;
}
	
