#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include "cruce.h"

#define SEM_IPC_C1 2
#define SEM_IPC_C2 3
#define SEM_IPC_P1 4
#define SEM_IPC_P2 5
#define ZONA_CRITICA 6
#define CRUCE_COCHES 7

// struct para mensajes 
struct mensaje {
    long tipo;
};

struct datos{
	int semid, memid, buzon;
	int fase;
	pid_t pidDelPadre;
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

	if(datos.fase ==2){ 
	CRUCE_pon_semAforo(SEM_C1 ,AMARILLO);
	pausa();
	pausa();
	pausa();
	}

	if(datos.fase ==3){ 
	CRUCE_pon_semAforo(SEM_C2,AMARILLO);
	pausa();
	pausa();
	pausa();
	}
}

int cambiarColorSem()
{
	int i;
	struct sembuf sops;

	datos.fase = 0;
	for(;;){

			if(datos.fase==1){
				ESPERA(sops,SEM_IPC_P1,datos.semid);
				CRUCE_pon_semAforo(SEM_P1,ROJO);
			}

			CRUCE_pon_semAforo(SEM_P2,VERDE);
			SENHAL(sops,SEM_IPC_P2,datos.semid);
			CRUCE_pon_semAforo(SEM_C1,VERDE);
			SENHAL(sops,SEM_IPC_C1,datos.semid); 			

			for(i=0; i<6; i++)
			{
				pausa();
			}
			

		datos.fase=2;

		//Segunda fase duracion 9 pausas
			ESPERA(sops,SEM_IPC_C1,datos.semid); 
			ambar();
			CRUCE_pon_semAforo(SEM_C1,ROJO);
			ESPERA(sops,SEM_IPC_P2,datos.semid);
			CRUCE_pon_semAforo(SEM_P2,ROJO);
			CRUCE_pon_semAforo(SEM_C2,VERDE);
			SENHAL(sops,SEM_IPC_C2,datos.semid);

			for(i=0; i<9; i++)
			{
				pausa();
			}

		datos.fase = 3;
		//Tercera fase duracion 12 pausas
			ESPERA(sops,SEM_IPC_C2,datos.semid);
			ambar();
			CRUCE_pon_semAforo(SEM_C2,ROJO);
			CRUCE_pon_semAforo(SEM_P1,VERDE);
			SENHAL(sops,SEM_IPC_P1,datos.semid); 

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

/*
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short  *array;
} arg;
*/

//Manejadora para la señal SIGINT

void terminar (int sig) {
	int i;
	pid_t self = getpid();

	if(datos.pidDelPadre == self){
		while(wait(NULL) > 0);

		CRUCE_fin();	
		
		//Eliminamos los semaforos
		if(datos.semid != -1){ 
			if(semctl(datos.semid,2,IPC_RMID)<0){
				perror("Semaforos no eliminados correctamente.\n");
				exit (1);
			}	
		}

		//Eliminamos la memoria compartida
		if(datos.memid != -1){
			if(shmctl(datos.memid,IPC_RMID,NULL)==-1){
				perror("Memoria no liberada correctamente.\n");
				exit(1);	
			}
		}
		
		if(datos.buzon !=-1){
			if(msgctl(datos.buzon,IPC_RMID,NULL)==-1){ 
				perror("Buzon no eliminado correctamente.\n");
				exit(1);
			}
		}
	}else{
		exit(0);
	}
	exit(0);
}


int main (int argc, char *argv[]){
	int numProc, velocidad, tipoProceso, flagCoche=0,flagLibCoche=0,flagCruce=0,flagCritico=0;
	char * zona;
	int argumentos = argc;
	struct posiciOn pos1, pos2, pos3,posAnt;
	struct sembuf sops;
	struct mensaje msg;
	int i=0,j=0,h=1;
	int msgTipoLib[55][22];
	int msgTipoLibP[50][17];
	//union semun arg;
	//arg.val=1;
	datos.fase=0;
	datos.pidDelPadre = getpid();
	datos.memid=-1;
	datos.semid=-1;
	datos.buzon=-1;

	//Comprobamos que los parametros introducidos son los correctos
	
	if((argumentos != 3)){
		printf("Error en el numero de parametros\n\n");
		ayuda();
		return 1;
	}

	

	if(!verify(argv[2]))
	{
		velocidad = atoi(argv[2]);
	}else{
		printf("Error. El parametro velocidad debe ser un número.\n\n");
		ayuda();
		return 2;
	}
		

	if(velocidad < 0){
		printf("Error. El parametro velocidad debe ser igual o mayor que cero.\n\n");
		ayuda();
		return 2;
	}

	if(!verify(argv[1]))
	{
		numProc = atoi(argv[1]);
	}else{
		printf("Error. El parametro de procesos debe ser un número.\n\n");
		ayuda();
		return 3;
	}
		
	if(numProc < 2 && numProc > 50 ){
		printf("Error, el numero de procesos tiene que ser mayor que 2 y menor a 50.\n\n");
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
	
	//Configuramos la señal SIGCLD para que exorcice INIT a los procesos hijos en estado zombie.
	ss.sa_handler=SIG_IGN;
	if(sigaction(SIGCLD,&ss,NULL)==-1){
		perror("Error al configurar SIGCLD.\n");
		return 4;
	}
	
	//Creamos los semaforos y la memoria
	datos.semid=semget(IPC_PRIVATE, 8, IPC_CREAT|0600);
	if(datos.semid==-1){
		perror("Error al crear los semaforos.\n");
		kill(getpid(),SIGTERM);
	}

	//arg.val=numProc-2;
	
	if(semctl(datos.semid,1,SETVAL,numProc-2) == -1)
	{
		perror("Error en la inicialización del semáforo de procesos.\n");
		kill(getpid(),SIGTERM);		
	}

	//arg.val=1;

	//Iniciamos los semáforos para controlar los Semaforos físicos
	for(i=2;i<6;i++){
		if(semctl(datos.semid,i,SETVAL,1) == -1)
		{
			fprintf(stderr,"Error en la inicialización del semáforo de Semaforos %d.\n",i);
			kill(getpid(),SIGTERM);		
		}
	}

	if(semctl(datos.semid,ZONA_CRITICA,SETVAL,1) == -1)
		{
			fprintf(stderr,"Error en la inicialización del semáforo de Zona crítica de Nacimiento de Peatones %d.\n",i);
			kill(getpid(),SIGTERM);		
		}

	if(semctl(datos.semid,CRUCE_COCHES,SETVAL,1) == -1)
		{
			fprintf(stderr,"Error en la inicialización del semáforo de Cruce Coche\n");
			kill(getpid(),SIGTERM);		
		}

	datos.memid=shmget(IPC_PRIVATE, 256, IPC_CREAT|0600); //La biblioteca usa 256 bytes 
	if(datos.memid==-1){
		perror("Error al crear la zona de memoria compartida.\n");
		kill(getpid(),SIGTERM);
	}

	zona=shmat(datos.memid, NULL, 0);
	if(zona==(char *)-1){
		perror("Error asociando zona de memoria compartida.\n");
		kill(getpid(),SIGTERM);
	}

	datos.buzon = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
	if(datos.buzon==-1)
	{
		perror("Error al crear el buzon.\n");
		kill(getpid(),SIGTERM);
	}
	
	//Inicializamos las posiciones para coches como liberadas
	for(i=0;i<55;i++){
		for(j=0;j<21;j++){
			msgTipoLib[i][j] = h;
			msg.tipo=msgTipoLib[i][j];

			if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
				perror("No se manda un mensaje liberar inicial coche");
				kill(getpid(),SIGTERM);
			}
			h++;
		}
	}
	
	//Inicializamos las posiciones para peatones como liberadas
	for(i=0;i<51;i++){
		for(j=0;j<18;j++){
			msgTipoLibP[i][j] = h;
			msg.tipo=msgTipoLibP[i][j];
			if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
				perror("No se manda un mensaje liberar inicial coche");
				kill(getpid(),SIGTERM);
			}
			h++;
		}
	}
	h=0;
	j=0;

	if(CRUCE_inicio(velocidad,numProc, datos.semid, zona)==-1){
		perror("Error al llamar a CRUCE_inicio.\n");
		kill(getpid(),SIGTERM);
	}

	//Inicializamos los semáforos en rojo para evitar choques por reparto de CPU
	CRUCE_pon_semAforo(SEM_P1,ROJO);
	CRUCE_pon_semAforo(SEM_P2,ROJO);
	CRUCE_pon_semAforo(SEM_C1,ROJO);
	CRUCE_pon_semAforo(SEM_C2,ROJO);
	ESPERA(sops,SEM_IPC_P1,datos.semid);
	ESPERA(sops,SEM_IPC_C2,datos.semid);
	ESPERA(sops,SEM_IPC_P2,datos.semid);
	ESPERA(sops,SEM_IPC_C1,datos.semid);

	//Creamos el proceso del gestor semafórico y lo iniciamos
	switch(fork())
	{
		case -1:	 
			perror("Error al crear el proceso Gestor Semafórico.");
			kill(getpid(),SIGTERM);
			break;
		case 0: 
			cambiarColorSem();						//Gestor Semafórico
			break;
	}

	//Bucle infito del proceso padre para creación de procesos coche y peatón
	for(;;){
		ESPERA(sops,1,datos.semid);
		tipoProceso=CRUCE_nuevo_proceso();

		if(tipoProceso==PEAToN){
			//Creamos un nuevo proceso para que gestione el peaton			
			switch (fork())
			{
			case -1:
				perror("Error al crear el proceso peaton.\n");
				kill(getpid(),SIGTERM);							//Llamamos a la manejadora
			case 0:
				//Proceso peaton
				//Entrar en seccion critica para evitar choques entre peatones en la poscion de salida
				ESPERA(sops,ZONA_CRITICA,datos.semid);
				pos1=CRUCE_inicio_peatOn_ext(&pos2);
				
				if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLibP[pos2.x][pos2.y],0)){
					perror("Error esperar peaton 1");
					kill(getpid(),SIGTERM);	
				} 
				
				posAnt=pos2;

				do{
					do{
						if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLibP[pos1.x][pos1.y],0)){
							perror("Error esperar peaton 2");
							kill(getpid(),SIGTERM);
						} 	
						
						pos3=CRUCE_avanzar_peatOn(pos1);
						
						msg.tipo=msgTipoLibP[posAnt.x][posAnt.y];
						if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
							perror("Error enviar peaton 1");
							kill(getpid(),SIGTERM);
						} 
						posAnt=pos1;						

						if((pos3.y==11)){

							if((pos3.x>=21) && (pos3.x<=27)){			//P1
								ESPERA(sops,SEM_IPC_P2,datos.semid); 
								pos1=pos3;
								do{
									if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLibP[pos1.x][pos1.y],0)){
										perror("Error esperar peaton 3");
										kill(getpid(),SIGTERM);
									}

									pos3=CRUCE_avanzar_peatOn(pos1);
									
									msg.tipo=msgTipoLibP[posAnt.x][posAnt.y];
									if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
										perror("Error enviar peaton 2");
										kill(getpid(),SIGTERM);
									}

									posAnt=pos1;				

									if(h==0){
										pausa();
										h++;
									}else
										h--;
									
									pos1=pos3;

								}while(pos3.y!=6);
								SENHAL(sops,SEM_IPC_P2,datos.semid); 
							}
							
						}
						
						if((pos3.x==30)){

							if((pos3.y>=13) && (pos3.y<=15)){		//P2
								ESPERA(sops,SEM_IPC_P1,datos.semid); 
								ESPERA(sops,CRUCE_COCHES,datos.semid); 
								pos1=pos3;

								do{
									if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLibP[pos1.x][pos1.y],0)){
										perror("Error esperar peaton 4");
										kill(getpid(),SIGTERM);
									}

									pos3=CRUCE_avanzar_peatOn(pos1);

									msg.tipo=msgTipoLibP[posAnt.x][posAnt.y];
									if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
										perror("Error esperar peaton 3.");
										kill(getpid(),SIGTERM);
									} 

									posAnt=pos1;

									if(h==0){
										pausa();
										h++;
									}else
										h--;
								
									pos1=pos3;
										

								}while(pos3.x!=41);

								SENHAL(sops,SEM_IPC_P1,datos.semid);
								SENHAL(sops,CRUCE_COCHES,datos.semid);
							}
									
						}

						if(h==0){
							pausa();
							h++;
						}else
							h--;

						pos1=pos3;
					}while(((pos1.x>=0 && pos1.x<=29) && pos1.y==16) || (pos1.x==0 && (pos1.y>=12 && pos1.y<=16)));//Condicion zona critrica
				
					if(flagCritico==0){
						SENHAL(sops,ZONA_CRITICA,datos.semid);
						flagCritico=1;
					}

				}while((pos3.y>=0) || (pos3.x>0));

				msg.tipo=msgTipoLibP[posAnt.x][posAnt.y];
				CRUCE_fin_peatOn();
				if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
						perror("Error enviar peaton 4");
						kill(getpid(),SIGTERM);
					}
				SENHAL(sops,1,datos.semid);
				return 0;
			}
			

		} else {
			//Creamos un nuevo proceso para que gestione el coche
			switch (fork())
			{
			case -1:
				perror("Error al crear el proceso coche.\n");
				kill(getpid(),SIGTERM);
			case 0:
				//Proceso coche
				pos1=CRUCE_inicio_coche();
		
				if(pos1.y==10)			//C2
				{
					do{ 
						if(flagCruce==0){
							if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLib[pos1.x+8+2][pos1.y],0)){
								perror("Error esperar coche 1");
								kill(getpid(),SIGTERM);
							} 
						}
						
						pos3=CRUCE_avanzar_coche(pos1); 						

						if(flagLibCoche==1 && flagCruce<6){ //Libera x=33 y=10 
							msg.tipo=msgTipoLib[posAnt.x+2][posAnt.y];
							if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
								perror("Error enviar coche 1");
								kill(getpid(),SIGTERM);
							}
						
						}else{
							flagLibCoche=1;
						}

						if(flagCruce>=1)
							flagCruce++;
							
						posAnt=pos1;

						if(j==0){	
							pausa_coche();
							j++;
						}else
							j--;

						pos1=pos3;

						if(pos3.x==13){					
							ESPERA(sops,SEM_IPC_C2,datos.semid);
						}else if(pos3.x==23){	
							ESPERA(sops,CRUCE_COCHES,datos.semid);
							flagCruce=1;	
						}else if(pos3.x==29){	
							SENHAL(sops,SEM_IPC_C2,datos.semid);
						}
						

					}while(pos1.y>0);					
				}
				
				if(pos1.x==33){					//C1 con x=33
					do{ 
						if(flagLibCoche==0){
							if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLib[pos1.x+2][pos1.y+6],0)){
								perror("Error esperar coche 2");
								kill(getpid(),SIGTERM);
							}
							posAnt=pos1;					
						}
					pos3=CRUCE_avanzar_coche(pos1);

					if(flagLibCoche==1){    
						msg.tipo=msgTipoLib[posAnt.x+2][posAnt.y+6];							
						if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1)
						{
							perror("Error enviar coche 2");
							kill(getpid(),SIGTERM);
						}
						flagLibCoche++;
					}else{
						flagLibCoche=2;
					}

					
					if((pos3.y==6) && (flagCoche==0)){ 
						ESPERA(sops,SEM_IPC_C1,datos.semid);
						ESPERA(sops,CRUCE_COCHES,datos.semid);
						flagCoche=1;
					}
					if((pos3.y==13)&&(flagCoche==1)){
						flagCoche=2;
						flagLibCoche=1;
					}

					if(j==0){
						pausa_coche();
						j++;
					}else
						j--;

					pos1=pos3;

					}while(pos3.y!=-2);
				}						
													
				CRUCE_fin_coche();
				SENHAL(sops,1,datos.semid); 
				if(flagCoche==2)
					SENHAL(sops,SEM_IPC_C1,datos.semid);
				
				SENHAL(sops,CRUCE_COCHES,datos.semid);
				return 0;	
			}
		}
	}
	return 0;
}
