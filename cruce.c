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

#define LONGITUD 128

// struct para mensajes 
struct mensaje {
	int boolean;
    long tipo; 
    char txt_msj[LONGITUD];
} message;

struct datos{
	int semid, memid;
	int buzon;
	int procesos;
	int argumentos;
	int fase;
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
	perror("Error en operación WAIT en semáforo");
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
	perror("Error en operación SIGNAL en semáforo");
	exit(6);
  }
}

void ambar(){
	
	if(datos.fase ==1){ 
	CRUCE_pon_semAforo(SEM_C2,AMARILLO);
	pausa();
	pausa();
	}

	if(datos.fase ==2){ 
	CRUCE_pon_semAforo(SEM_C1 ,AMARILLO);
	pausa();
	pausa();
	}

	if(datos.fase ==3){ 
	CRUCE_pon_semAforo(SEM_C1,AMARILLO);
	CRUCE_pon_semAforo(SEM_C2,AMARILLO);
	pausa();
	pausa();
	}
}

int cambiarColorSem()
{
	int i, recibir;

	recibir = msgrcv(buzon, message, sizeof(message)-sizeof(long), 0, MSG_NOERROR);
		if(recibir==-1)
		{
			perror("Error al recibir el mensaje");
			exit(-1);
		}

	for(;;){ 
		//Primera fase duracion 6 pausas

			CRUCE_pon_semAforo(SEM_C1,VERDE);
			CRUCE_pon_semAforo(SEM_P1,ROJO);	
			ambar();
			CRUCE_pon_semAforo(SEM_C2,ROJO);
			CRUCE_pon_semAforo(SEM_P2,VERDE);

			for(i=0; i<6; i++)
			{
				pausa();
			}

		datos.fase=2;

		//Segunda fase duracion 9 pausas
			ambar();
			CRUCE_pon_semAforo(SEM_C1,ROJO);
			CRUCE_pon_semAforo(SEM_P1,ROJO);
			CRUCE_pon_semAforo(SEM_C2,VERDE);
			CRUCE_pon_semAforo(SEM_P2,ROJO);
	
			for(i=0; i<9; i++)
			{
				pausa();
			}

		datos.fase = 3;
		//Tercera fase duracion 12 pausas
			ambar();
			CRUCE_pon_semAforo(SEM_C1,ROJO);
			CRUCE_pon_semAforo(SEM_C2,ROJO);
			CRUCE_pon_semAforo(SEM_P2,ROJO);
			CRUCE_pon_semAforo(SEM_P1,VERDE);

			for(i=0; i<12; i++)
			{
				pausa();
			}

			datos.fase = 1;
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

	if(datos.pidDelPadre != self){ 
		_exit(0);
	}

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
	if(datos.buzon !=-1){
		if(msgctl(datos.buzon,IPC_RMID,NULL)==-1){
			perror("Buzon no eliminado correctamente.\n");
		}
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

	printf("Programa acabado correctamente.\n"); //Quitarlo antes de entregar
	CRUCE_fin();
	exit(0);
}


int main (int argc, char *argv[]){
	int numProc, velocidad, buzon;
	char * zona;
	int tipoProceso;
	datos.argumentos = argc;
	struct posiciOn pos1, pos2, pos3;
	struct sembuf sops;
	datos.fase=1;

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
		perror("Error al crear los semaforos.\n");
		kill(getpid(),SIGTERM);							//Llamar manejadora
	}

	if(semctl(datos.semid,1,SETVAL,1) == -1)
    {
        perror("Error en la inicialización del semáforo de procesos.\n");
		kill(getpid(),SIGTERM);		
    }

	datos.memid=shmget(IPC_PRIVATE, 256, IPC_CREAT|0600);
	if(datos.memid==-1){
		perror("Error al crear la zona de memoria compartida.\n");
		kill(getpid(),SIGTERM);							//Llamar manejadora
	}
	
	zona=shmat(datos.memid, NULL, 0);
	if(zona==(char *)-1){
		perror("Error asociando zona de memoria compartida.\n");
		kill(getpid(),SIGTERM);							//Llamamos a la manejadora
	}

	datos.buzon = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
	if(datos.buzon==-1)
	{
		perror("Error al crear el buzon.\n");
		kill(getpid(),SIGTERM);							//Llamar manejadora
	}
		
	if(CRUCE_inicio(velocidad, numProc, datos.semid, zona)==-1){
		perror("Error al llamar a CRUCE_inicio.\n");
		kill(getpid(),SIGTERM);							//Llamamos a la manejadora
	}

	switch(fork())
	{
		case -1:	 
			perror("Error al crear el proceso Gestor Semafórico.\n");
			kill(getpid(),SIGTERM);							//Llamamos a la manejadora
			break;
		case 0: 
			cambiarColorSem();						//Gestor Semafórico
			break;
	}
	
	for(;;){ 
		ESPERA(sops,1,datos.semid);

		tipoProceso=CRUCE_nuevo_proceso();
		fprintf(stderr,"Soy el padre con PID %d con tipoProceso %d\n",getpid(),tipoProceso);
		//Creamos un peaton y lo movemos para ver las posiciones por las que pasa

		if(CRUCE_nuevo_proceso()==PEAToN){
			//Creamos un nuevo proceso para que gestione el peaton
			
			switch (fork())
			{
			case -1:
				perror("Error al crear el proceso peaton.\n");
				kill(getpid(),SIGTERM);							//Llamamos a la manejadora
			case 0:
				//Proceso peaton

				pos1=CRUCE_inicio_peatOn_ext(&pos2);
				fprintf(stderr, "Soy el peaton con PID %d. pos1.x=%d, pos1.y=%d pos2.x=%d pos2.y=%d\n", getpid(),
					pos1.x,pos1.y,pos2.x,pos2.y);

				do{ 
					pos3=CRUCE_avanzar_peatOn(pos1);
					if(pos3.y==12){
						for(i=21;i<28;i++){
							if((pos3.x==i){
									//Mandar mensaje a gestor semafórico
									//Esperar mensaje de gestor semaforico
								break;
							}
						}
					}
					if(pos3.x==29){
						for(i=13;i<16;i++){
							if((pos3.y==i){
									//Mandar mensaje a gestor semafórico
									message.boolean=1;
									if(msgsnd(datos.buzon,message,sizeof(message)-sizeof(long),SEM_P1)==-1){
										//mandar el error
									}
									//Esperar mensaje de gestor semaforico
									msgrcv(datos.buzon,message,sizeof(message)-sizeof(long),SEM_P1,0);
								break;
							}	
						}
					}	
					fprintf(stderr, "Soy el peaton con PID %d.Avanzo pos1.x=%d, pos1.y=%d pos3.x=%d pos3.y=%d\n", getpid(),
					pos1.x,pos1.y,pos3.x,pos3.y);
					pausa();
					pos1=pos3;
				}while(pos3.y>=0);

				CRUCE_fin_peatOn();
				SENHAL(sops,1,datos.semid); //Suma uno al semaforo
				return 0;
			}

		} else {
			//Creamos un nuevo proceso para que gestione el coche

			switch (fork())
			{
			case -1:
				perror("Error al crear el proceso coche.\n");
				kill(getpid(),SIGTERM);							//Llamamos a la manejadora
			case 0:
				//Proceso coche
				pause();
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
				SENHAL(sops,1,datos.semid); //Suma uno al semaforo
				return 0;	
			}
		}
		//pause();
	}
	return 0;
}
	
