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
#define ZonaCritica 6

// struct para mensajes 
struct mensaje {
    long tipo;       /* message type, must be > 0 */
};

struct datos{
	int semid, memid;
	int buzon;
	int procesos;
	int argumentos;
	int fase;
	pid_t pidDelPadre;
	int * myZona;
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
	int i, recibir;

	struct mensaje msg;
	struct sembuf sops;

	//ESPERA(sops,SEM_IPC_C1,datos.semid); //Resta uno al semáforo
	//ESPERA(sops,SEM_IPC_P2,datos.semid); //Resta uno al semáforo
	//ESPERA(sops,SEM_IPC_C2,datos.semid); //Resta uno al semáforo
	//CRUCE_pon_semAforo(SEM_C2,ROJO);
	datos.fase = 0;
	for(;;){

			if(datos.fase==1){
				ESPERA(sops,SEM_IPC_P1,datos.semid); //Resta uno al semáforo
				CRUCE_pon_semAforo(SEM_P1,ROJO);
			}

			CRUCE_pon_semAforo(SEM_P2,VERDE);
			SENHAL(sops,SEM_IPC_P2,datos.semid); //Resta uno al semáforo
			CRUCE_pon_semAforo(SEM_C1,VERDE);
			SENHAL(sops,SEM_IPC_C1,datos.semid); //Resta uno al semáforo			

			for(i=0; i<6; i++)
			{
				pausa();
			}
			

		datos.fase=2;

		//Segunda fase duracion 9 pausas
			ESPERA(sops,SEM_IPC_C1,datos.semid); //Resta uno al semáforo
			ambar();
			CRUCE_pon_semAforo(SEM_C1,ROJO);
			ESPERA(sops,SEM_IPC_P2,datos.semid); //Resta uno al semáforo
			CRUCE_pon_semAforo(SEM_P2,ROJO);
			CRUCE_pon_semAforo(SEM_C2,VERDE);
			SENHAL(sops,SEM_IPC_C2,datos.semid); //Resta uno al semáforo

			for(i=0; i<9; i++)
			{
				pausa();
			}

		datos.fase = 3;
		//Tercera fase duracion 12 pausas
			ESPERA(sops,SEM_IPC_C2,datos.semid); //Resta uno al semáforo
			ambar();
			CRUCE_pon_semAforo(SEM_C2,ROJO);
			CRUCE_pon_semAforo(SEM_P1,VERDE);

			SENHAL(sops,SEM_IPC_P1,datos.semid); //Suma uno al semaforo

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
unsigned concatenar(unsigned x, unsigned y) {
    unsigned pow = 10;
    while(y >= pow)
        pow *= 10;
    return x * pow + y;        
}*/

//Manejadora para la señal SIGINT

void terminar (int sig) {
	int i;
	pid_t self = getpid();

	if(datos.pidDelPadre == self){
		while(wait(NULL) > 0)
		{

		};
		/*if(wait(NULL)==-1){
			perror("Error en la espera por la muerte del hijo");
		}*/
		CRUCE_fin();	
		//while ((wpid = wait(&status)) > 0); 
		
		//Eliminamos los semaforos
		if(datos.semid != -1){ 
			if(semctl(datos.semid,2,IPC_RMID)<0){
				perror("Semaforos no eliminados correctamente.\n");
				//MAndar otra vez o se finaliza 
				exit (0);
			} else {
				perror("Semaforos eliminados correctamente.\n");
			}
		}else {
				perror("Semaforos no creados.\n");
		}

		//Eliminamos la memoria compartida
		if(datos.memid != -1){
			if(shmctl(datos.memid,IPC_RMID,NULL)==-1){
				perror("Memoria no liberada correctamente.\n");
			}else {
				perror("Memoria eliminada correctamente.\n");
			}
		}else {
				perror("Memoria no eliminada.\n");
		}
		
		if(datos.buzon !=-1){
			if(msgctl(datos.buzon,IPC_RMID,NULL)==-1){
				perror("Buzon no eliminado correctamente.\n");
			}else {
				perror("Buzon eliminado correctamente.\n");
			}
		}else {
				perror("Buzon no eliminado.\n");
		}
		printf("Programa acabado correctamente.\n"); //Quitarlo antes de entregar	
	}else{
		exit(1);
	}
	exit(0);
}


int main (int argc, char *argv[]){
	int numProc, velocidad, buzon, tipoProceso, flag=0,flag2=0,flagCruce=0,flagCritico=0;
	char * zona;
	datos.argumentos = argc;
	struct posiciOn pos1, pos2, pos3,posAnt;
	struct sembuf sops;
	struct mensaje msg;
	datos.fase=0;
	int i=0,j=0,h=1;
	int msgTipoLib[55][22];
	int msgTipoLibP[50][17];

	//Comprobamos que los parametros introducidos son los correctos
	/*
	if((datos.argumentos != 3)){
		printf("Error en el numero de parametros\n\n");
		ayuda();
		return 1;
	}
	*/

	datos.pidDelPadre = getpid();

	if(!verify(argv[1]))
		velocidad = atoi(argv[1]);

	if(velocidad < 0){
		printf("Error. El parametro velocidad debe ser igual o mayor que cero.\n\n");
		ayuda();
		return 3;
	}

	if(!verify(argv[2]))
		numProc = atoi(argv[2]);

	//fprintf(stderr,"Numproc es %d\n",numProc);

	if(numProc < 2 && numProc > 50 ){
		printf("Error, el numero de procesos tiene que ser mayor que 2 y menor a 128.\n\n");
		ayuda();
		return 2;
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
	
	ss.sa_handler=SIG_IGN;
	if(sigaction(SIGCLD,&ss,NULL)==-1){
		perror("Error al configurar SIGCLD.\n");
		return 1;
	}
	
	//Creamos los semaforos y la memoria
	datos.semid=semget(IPC_PRIVATE, 8, IPC_CREAT|0600);
	if(datos.semid==-1){
		perror("Error al crear los semaforos.\n");
		kill(getpid(),SIGTERM);							//Llamar manejadora
	}
	
	if(semctl(datos.semid,1,SETVAL,numProc-2) == -1)
	{
		perror("Error en la inicialización del semáforo de procesos.\n");
		kill(getpid(),SIGTERM);		
	}
	//Iniciamos los semáforos para controlar los Semaforos físicos
	for(i=2;i<6;i++){
		if(semctl(datos.semid,i,SETVAL,1) == -1)
		{
			fprintf(stderr,"Error en la inicialización del semáforo de Semaforos %d.\n",i);
			kill(getpid(),SIGTERM);		
		}
	}

	if(semctl(datos.semid,6,SETVAL,1) == -1)
		{
			fprintf(stderr,"Error en la inicialización del semáforo de Zona crítica de Nacimiento de Peatones %d.\n",i);
			kill(getpid(),SIGTERM);		
		}

	if(semctl(datos.semid,7,SETVAL,1) == -1)
		{
			fprintf(stderr,"Error en la inicialización del semáforo de Cruce Coche\n");
			kill(getpid(),SIGTERM);		
		}

	datos.memid=shmget(IPC_PRIVATE, 256, IPC_CREAT|0600); //La biblioteca usa 256 bytes 
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

	for(i=0;i<55;i++){
		for(j=0;j<21;j++){
			msgTipoLib[i][j] = h;
			msg.tipo=msgTipoLib[i][j];

			if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
				fprintf(stderr,"No se manda un mensaje liberar a la posiocion %d %d\n",i,j);
			}
			h++;
		}
	}
	for(i=0;i<51;i++){
		for(j=0;j<18;j++){
			msgTipoLibP[i][j] = h;
			msg.tipo=msgTipoLibP[i][j];
			if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
				fprintf(stderr,"No se manda un mensaje liberar a la posiocion %d %d\n",i,j);
			}
			h++;
		}
	}
	h=0;
	j=0;	
	if(CRUCE_inicio(velocidad, numProc, datos.semid, zona)==-1){
		perror("Error al llamar a CRUCE_inicio.\n");
		kill(getpid(),SIGTERM);							//Llamamos a la manejadora
	}

	CRUCE_pon_semAforo(SEM_P1,ROJO);
	CRUCE_pon_semAforo(SEM_P2,ROJO);
	CRUCE_pon_semAforo(SEM_C1,ROJO);
	CRUCE_pon_semAforo(SEM_C2,ROJO);
	ESPERA(sops,SEM_IPC_P1,datos.semid); //Resta uno al semáforo
	ESPERA(sops,SEM_IPC_C2,datos.semid); //Resta uno al semáforo
	ESPERA(sops,SEM_IPC_P2,datos.semid); //Resta uno al semáforo
	ESPERA(sops,SEM_IPC_C1,datos.semid); //Resta uno al semáforo

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

	int a=0;	
	
	for(;;){
		ESPERA(sops,1,datos.semid);
		tipoProceso=CRUCE_nuevo_proceso();
		//fprintf(stderr,"Soy el padre con PID %d con tipoProceso %d\n",getpid(),tipoProceso);
		tipoProceso=0;
		//Creamos un peaton y lo movemos para ver las posiciones por las que pasa


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
				ESPERA(sops,6,datos.semid);
				pos1=CRUCE_inicio_peatOn_ext(&pos2);
				
				if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLibP[pos2.x][pos2.y],0)){
					fprintf(stderr,"Peaton %d ERROR ESPERAR A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos2.x,pos2.y,errno);
				} else {
					fprintf(stderr,"Peaton %d ESPERA BIEN A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos2.x,pos2.y,errno);
				}	
				
				posAnt=pos2;

				fprintf(stderr, "Soy el peaton con PID %d.HOLA pos1.x=%d, pos1.y=%d pos2.x=%d pos2.y=%d\n", getpid(),
					pos1.x,pos1.y,pos2.x,pos2.y);

				do{
					do{
						if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLibP[pos1.x][pos1.y],0)){
							fprintf(stderr,"Peaton %d ERROR ESPERAR A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
						} else {
							fprintf(stderr,"Peaton %d ESPERA BIEN A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
						}		
						
						pos3=CRUCE_avanzar_peatOn(pos1);
						
						fprintf(stderr, "Soy el peaton con PID %d.Avanzo pos1.x=%d, pos1.y=%d x=%d y=%d\n", getpid(),
						pos1.x,pos1.y,pos3.x,pos3.y);
						
						msg.tipo=msgTipoLibP[posAnt.x][posAnt.y];
						if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
							fprintf(stderr,"Peaton %d NO LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);
						} else { 
							fprintf(stderr,"Peaton %d LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);	
						}

						posAnt=pos1;
						

						if((pos3.y==11)){

							fprintf(stderr, "Soy el peaton con PID %d.Entro en el if Flag P2\n", getpid());

							if((pos3.x>=21) && (pos3.x<=27)){
								ESPERA(sops,SEM_IPC_P2,datos.semid); //Resta uno al semáforo
								fprintf(stderr, "Soy el peaton con PID %d.Entro en el if MSGRCV P2\n", getpid());
								pos1=pos3;
								do{
									if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLibP[pos1.x][pos1.y],0)){
										fprintf(stderr,"Peaton %d ERROR ESPERAR A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
									} else {
										fprintf(stderr,"Peaton %d ESPERA BIEN A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
									}

									pos3=CRUCE_avanzar_peatOn(pos1);

									fprintf(stderr, "Soy el peaton con PID %d.Avanzo pos1.x=%d, pos1.y=%d x=%d y=%d\n", getpid(),
									pos1.x,pos1.y,pos3.x,pos3.y);

									
									msg.tipo=msgTipoLibP[posAnt.x][posAnt.y];
									//strcpy(msg.texto,"1");
									if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
										fprintf(stderr,"Peaton %d NO LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);
									} else { 
										fprintf(stderr,"Peaton %d LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);	
									}

									posAnt=pos1;				

									if(h==0){
										pausa();
										h++;
									}else
										h--;
									
									pos1=pos3;

								}while(pos3.y!=6);
								fprintf(stderr, "Soy el peaton con PID %d.Pongo flag a 1\n", getpid());
								SENHAL(sops,SEM_IPC_P2,datos.semid); //Suma uno al semáforo
							}
							
						}
						
						if((pos3.x==30)){

							fprintf(stderr, "Soy el peaton con PID %d.Entro en el if Flag P1\n", getpid());


							if((pos3.y>=13) && (pos3.y<=15)){
								ESPERA(sops,SEM_IPC_P1,datos.semid); 
								fprintf(stderr, "Soy el peaton con PID %d.Entro en el if MSGRCV P1\n", getpid());
								pos1=pos3;

								do{
									if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLibP[pos1.x][pos1.y],0)){
										fprintf(stderr,"Peaton %d ERROR ESPERAR A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
									} else {
										fprintf(stderr,"Peaton %d ESPERA BIEN A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
									}

									pos3=CRUCE_avanzar_peatOn(pos1);

									fprintf(stderr, "Soy el peaton con PID %d.Avanzo pos1.x=%d, pos1.y=%d x=%d y=%d\n", getpid(),
									pos1.x,pos1.y,pos3.x,pos3.y);


									msg.tipo=msgTipoLibP[posAnt.x][posAnt.y];
									if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
										fprintf(stderr,"Peaton %d NO LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);
									} else { 
										fprintf(stderr,"Peaton %d LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);	
									}

									posAnt=pos1;

									if(h==0){
										pausa();
										h++;
									}else
										h--;
								
									pos1=pos3;
										

								}while(pos3.x!=43);
								
								fprintf(stderr, "Soy el peaton con PID %d.Pongo flag a 1\n", getpid());
								SENHAL(sops,SEM_IPC_P1,datos.semid);
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
						SENHAL(sops,ZonaCritica,datos.semid);
						fprintf(stderr,"Peaton %d entro en el flag CRITICO\n",getpid());
						flagCritico=1;
					}

				}while((pos3.y>=0) || (pos3.x>0));

				msg.tipo=msgTipoLibP[posAnt.x][posAnt.y];
				
					if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
						fprintf(stderr,"Peaton %d NO LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);
					} else { 
						fprintf(stderr,"Peaton %d LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);	
					}
				
				fprintf(stderr,"Fin peaton.\n");
				CRUCE_fin_peatOn();
				SENHAL(sops,1,datos.semid); //Suma uno al semaforo
				//kill(getpid(),SIGKILL);
				return 0;
			}

		} else {
			//Creamos un nuevo proceso para que gestione el coche
			//ESPERA(sops,1,datos.semid);
//sleep(1);
//if(a++==2){pause();}
			switch (fork())
			{
			case -1:
				perror("Error al crear el proceso coche.\n");
				kill(getpid(),SIGTERM);							//Llamamos a la manejadora
			case 0:
				//Proceso coche
				pos1=CRUCE_inicio_coche();
				
				fprintf(stderr, "Soy el coche con PID %d. pos1.x=%d, pos1.y=%d\n", getpid(),
					pos1.x,pos1.y);

				//posAnt=pos1;
		
				if(pos1.y==10)			//C2
				{
					do{ 
						if(flagCruce==0){
							fprintf(stderr,"Coche %d ESPERO EN 1 POR x=%d y=%d tipo %d\n",getpid(),pos1.x+8,pos1.y,msgTipoLib[pos1.x+8+2][pos1.y]);
							if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLib[pos1.x+8+2][pos1.y],0)){
								fprintf(stderr,"Coche %d ERROR ESPERAR A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
							} 
						}
						fprintf(stderr, "Coche %d PASA 1\n", getpid());
						
						pos3=CRUCE_avanzar_coche(pos1); 

						

						if(flag2==1 && flagCruce<8){
							msg.tipo=msgTipoLib[posAnt.x+2][posAnt.y];
							fprintf(stderr,"Coche %d LIBERO 1 x=%d y=%d tipo: %ld tipoLib: %d\n",getpid(),posAnt.x,posAnt.y,msg.tipo,msgTipoLib[posAnt.x+2][posAnt.y]);	
							if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1)
								fprintf(stderr,"Coche %d NO LIBERO x=%d y=%d tipo: %ld\n",getpid(),posAnt.x,posAnt.y,msg.tipo);
						}else{
							flag2=1;
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
							//Se reserva la poscion del coche de atras y se comprueba si esta reservada la poscion del coche +2	
						}else if(pos3.x==25){	
							ESPERA(sops,7,datos.semid);
							flagCruce=1;	
						}else if(pos3.x==29){	
							SENHAL(sops,SEM_IPC_C2,datos.semid);
						}
						

					}while(pos1.y!=13); //Revisar pos1.x!=33 && 
					SENHAL(sops,7,datos.semid);
					//fprintf(stderr,"Pos1 =33\n");
					/*msg.tipo=msgTipoLib[pos1.x+2+2][pos1.y];
						fprintf(stderr,"Coche %d LIBERO 2 x=%d y=%d tipo: %ld tipoLib: %d\n",getpid(),posAnt.x,posAnt.y,msg.tipo,msgTipoLib[pos1.x+2+2][pos1.y]);	
						if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1)
							fprintf(stderr,"Coche %d NO LIBERO x=%d y=%d tipo: %ld\n",getpid(),posAnt.x,posAnt.y,msg.tipo);

					msg.tipo=msgTipoLib[pos1.x+2+4][posAnt.y];
						fprintf(stderr,"Coche %d LIBERO 2 x=%d y=%d tipo: %ld tipoLib: %d\n",getpid(),posAnt.x,posAnt.y,msg.tipo,msgTipoLib[pos1.x+2+4][pos1.y]);	
						if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1)
							fprintf(stderr,"Coche %d NO LIBERO x=%d y=%d tipo: %ld\n",getpid(),posAnt.x,posAnt.y,msg.tipo);

					msg.tipo=msgTipoLib[pos1.x+2+6][pos1.y];
						fprintf(stderr,"Coche %d LIBERO 2 x=%d y=%d tipo: %ld tipoLib: %d\n",getpid(),posAnt.x,posAnt.y,msg.tipo,msgTipoLib[pos1.x+2+6][pos1.y]);	
						if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1)
							fprintf(stderr,"Coche %d NO LIBERO x=%d y=%d tipo: %ld\n",getpid(),posAnt.x,posAnt.y,msg.tipo);*/

				}
				
				if(pos1.x==33 || pos1.x==29){					//C1 con x=33
					do{ 
						if(pos1.y<22){
						fprintf(stderr,"Coche %d ESPERO EN 3 POR x=%d y=%d tipo %d\n",getpid(),pos1.x,pos1.y+6,msgTipoLib[pos1.x+2][pos1.y+6]);

					if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLib[pos1.x+2][pos1.y+6],0)){

					}

					fprintf(stderr, "Coche %d PASA 3\n", getpid());
						}
					pos3=CRUCE_avanzar_coche(pos1);

					if(flag2==1){
						msg.tipo=msgTipoLib[posAnt.x+2][posAnt.y];
						
						fprintf(stderr,"Coche %d LIBERO 3 x=%d y=%d tipo: %ld\n",getpid(),posAnt.x,posAnt.y,msg.tipo);							
						if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
					} 
					}else{
						flag2=1;
					}

					posAnt=pos1;					
					
					if((pos3.y==6) && (flag==0)){
						ESPERA(sops,SEM_IPC_C1,datos.semid);

						flag=1;
					}
					if((pos3.y==11)&&(flag==1)){
						SENHAL(sops,SEM_IPC_C1,datos.semid);
						flag=2;
					}

					if(j==0){
						pausa_coche();
						j++;
					}else
						j--;

					pos1=pos3;

					}while(pos3.y!=-2);
				}
				for(i=0; i<7;i++){
					msg.tipo=msgTipoLib[posAnt.x+2][posAnt.y+i];

					fprintf(stderr,"Coche %d LIBERO 4 x=%d y=%d tipo: %ld tipoLib %d\n",getpid(),posAnt.x,posAnt.y,msg.tipo,msgTipoLib[posAnt.x+2][posAnt.y+i]);					
					if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
						fprintf(stderr,"Coche %d NO LIBERO 4 x=%d y=%d tipo: %ld\n",getpid(),posAnt.x+2,posAnt.y+i,msg.tipo);
					}
				}
				fprintf(stderr,"Fin coche %d.\n",getpid());
				CRUCE_fin_coche();
				SENHAL(sops,1,datos.semid); //Suma uno al semaforo
				//kill(getpid(),SIGKILL);
				return 0;	
			}
		}
	}
	
	return 0;
}
	