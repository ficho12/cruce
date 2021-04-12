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
	int semid, memid;
	int procesos;
	int argumentos;
	pid_t pidDelPadre;
	pid_t pidDeLosHijos[1089];
}datos;

void ESPERA(struct sembuf sops, int numSem, int semID)
{
  sops.sem_num=numSem;
  sops.sem_op=-1;
  sops.sem_flg=0;

  if(semop(semID,&sops,1)==-1)
  {
	perror("Operación WAIT en semáforo");
	exit(6);
  }
}

void SENHAL(struct sembuf sops, int numSem, int semID)
{
  sops.sem_num=numSem;
  sops.sem_op=1;
  sops.sem_flg=0;

  if(semop(semID,&sops,1)==-1)
  {
	perror("Operación SIGNAL en semáforo");
	exit(6);
  }
}

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

//Manejadora para la señal SIGINT

void terminar (int sig) {
	int i;
	pid_t self = getpid();

	//Eliminamos los semaforos
	if(datos.semid != -1){
		semctl(datos.semid,1,IPC_RMID);
	//Esto no funciona bien
		/*if(semctl(datos.semid,1,IPC_RMID)==-1){
			perror("Semaforos no eliminados correctamente.\n");
			//MAndar otra vez o se finaliza 
			exit (0);
		}*/
	}
	//Eliminamos la memoria compartida
	if(datos.memid != -1){
		if(shmctl(datos.memid,IPC_RMID,NULL)==-1){
			perror("Memoria no liberada correctamente.\n");
		}
	}
	if(datos.pidDelPadre != self){ 
		_exit(0);
	}

	for(i=0; i< datos.procesos-1; ++i){

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
	int numProc, velocidad;
	char * zona;
	int tipoProceso;
	datos.argumentos = argc;
	struct posiciOn pos1, pos2, pos3;
	struct sembuf sops;

	//Comprobamos que los parametros introducidos son los correctos
	/*
	if((datos.argumentos != 3)){
		printf("Error en el numero de parametros\n\n");
		ayuda();
		return 1;
	}
	*/

	if(!verify(argv[1]))
		numProc = atoi(argv[1]);

	if(numProc < 1){
		printf("Error, el numero de procesos tiene que ser mayor o igual que uno.\n\n");
		ayuda();
		return 2;
	}

	if(!verify(argv[2]))
		velocidad = atoi(argv[2]);

	if(velocidad < 0){
		printf("Error. El parametro velocidad debe ser igual o mayor que cero.\n\n");
		ayuda();
		return 3;
	}
	
	//Configuramos la señal SIGINT
	struct sigaction ss;
	ss.sa_handler = terminar;
	sigfillset(&ss.sa_mask);
	ss.sa_flags = 0;
	if(sigaction(SIGINT,&ss, NULL)==-1){
		perror("Error al configurar SIGINT.\n");
		exit(1);
	}
	
	//Creamos los semaforos y la memoria
	datos.semid=semget(IPC_PRIVATE, 2, IPC_CREAT|0600);
	if(datos.semid==-1){
		perror("Error al crear los semaforos");
		kill(getpid(),SIGTERM);							//Llamar manejadora
	}

	if(semctl(datos.semid,0,SETVAL,1) == -1)
    {
        perror("Error en la inicialización del semáforo de procesos");
		kill(getpid(),SIGTERM);		
    }

	if(semctl(datos.semid,1,SETVAL,1) == -1)
    {
        perror("Error en la inicialización del semáforo de procesos");
		kill(getpid(),SIGTERM);		
    }

	datos.memid=shmget(IPC_PRIVATE, 256, IPC_CREAT|0600);
	if(datos.memid==-1){
		perror("Error al crear la zona de memoria compartida");
		kill(getpid(),SIGTERM);							//Llamar manejadora
	}
	
	zona=shmat(datos.memid, NULL, 0);
	if(zona==(char *)-1){
		perror("Error asociando zona de memoria compartida");
		kill(getpid(),SIGTERM);							//Llamamos a la manejadora
	}
		
	if(CRUCE_inicio(velocidad, numProc, datos.semid, zona)==-1){
		perror("Error al llamar a CRUCE_inicio");
		kill(getpid(),SIGTERM);							//Llamamos a la manejadora
	}
	for(;;){ 
		tipoProceso=CRUCE_nuevo_proceso();
		fprintf(stderr,"Soy el padre con PID %d con tipoProceso %d\n",getpid(),tipoProceso);
		//Creamos un peaton y lo movemos para ver las posiciones por las que pasa
		/*
		SEM_WAIT(sops,0);
		if(semop(datos.semid,&sops,1)==-1)
		{
			perror("Operación WAIT en semáforo");
			exit(6);
		}
		*/

		ESPERA(sops,0,datos.semid);


		if(CRUCE_nuevo_proceso()==PEAToN){
			//Creamos un nuevo proceso para que gestione el peaton
			
			switch (fork())
			{
			case -1:
				perror("Error al crear el proceso peaton");
				kill(getpid(),SIGTERM);							//Llamamos a la manejadora
			case 0:
				sleep(5);
				//Proceso peaton

				pos1=CRUCE_inicio_peatOn_ext(&pos2);
				fprintf(stderr, "Soy el peaton con PID %d. pos1.x=%d, pos1.y=%d pos2.x=%d pos2.y=%d\n", getpid(),
					pos1.x,pos1.y,pos2.x,pos2.y);

				do{ 
					pos3=CRUCE_avanzar_peatOn(pos1);
					fprintf(stderr, "Soy el peaton con PID %d.Avanzo pos1.x=%d, pos1.y=%d pos3.x=%d pos3.y=%d\n", getpid(),
					pos1.x,pos1.y,pos3.x,pos3.y);
					pausa();
					pos1=pos3;
				}while(pos3.y>=0);

				CRUCE_fin_peatOn();

				return 0;
			}

			//SEM_SIGNAL(sops,0);

		} else {
			//Creamos un nuevo proceso para que gestione el coche
			//SEM_WAIT(sops,1);

			switch (fork())
			{
			case -1:
				perror("Error al crear el proceso coche");
				kill(getpid(),SIGTERM);							//Llamamos a la manejadora
			case 0:
				//Proceso coche
				pos1=CRUCE_inicio_coche();

				fprintf(stderr, "Soy el coche con PID %d. pos1.x=%d, pos1.y=%d pos2.x=%d pos2.y=%d\n", getpid(),
					pos1.x,pos1.y,pos2.x,pos2.y);
				do{ 
					pos3=CRUCE_avanzar_coche(pos1);
					fprintf(stderr, "Soy el coche con PID avanzo %d. pos1.x=%d, pos1.y=%d pos3.x=%d pos3.y=%d\n", getpid(),
					pos1.x,pos1.y,pos3.x,pos3.y);
					pausa_coche();
					pos1=pos3;
				}while(pos3.y>=0);

				CRUCE_fin_coche();
				
				return 0;	
			}
			//SEM_SIGNAL(sops,1);
		}
		//pause();
		/*
		SEM_SIGNAL(sops,0);
		if(semop(datos.semid,&sops,1)==-1)
		{
			perror("Operación SIGNAL en semáforo");
			exit(6);
		}
		*/
		SENHAL(sops,0,datos.semid);
	}
	
	CRUCE_fin();
	return 0;
}
	
