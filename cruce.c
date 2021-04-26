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

// struct para mensajes 
struct mensaje {
    long tipo;       /* message type, must be > 0 */
    char texto[1];    /* message data */
};

struct datos{
	int semid, memid;
	int buzon;
	int procesos;
	int argumentos;
	int fase;
	pid_t pidDelPadre;
	pid_t pidDeLosHijos[130];
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
	int i, recibir, msgReturn;

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

/*
struct posiciOn avanza(){

	if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLib[pos1.x][pos1.y],0)){
		fprintf(stderr,"Peaton %d ERROR ESPERAR A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
	} else {
		fprintf(stderr,"Peaton %d ESPERA BIEN A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
	}

	pos3=CRUCE_avanzar_peatOn(pos1);			

	fprintf(stderr, "Soy el peaton con PID %d.Avanzo pos1.x=%d, pos1.y=%d x=%d y=%d\n", getpid(),
	pos1.x,pos1.y,pos3.x,pos3.y);
	
	msg.tipo=msgTipoLib[posAnt.x][posAnt.y];
	if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
		fprintf(stderr,"Peaton %d NO LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);
	} else { 
		fprintf(stderr,"Peaton %d LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);	
	}
}*/


//Manejadora para la señal SIGINT

void terminar (int sig) {
	int i;
	pid_t self = getpid();

	if(datos.pidDelPadre != self){ 
		_exit(0);
	}
	
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
	int numProc, velocidad, buzon, flag=0,flag2=0,msgReturn, msgReturnLib;
	char * zona;
	int tipoProceso;
	datos.argumentos = argc;
	struct posiciOn pos1, pos2, pos3,posAnt;
	struct sembuf sops;
	struct mensaje msg;
	datos.fase=0;
	int i=0,j=0,h=6;
	int msgTipoRes[50][17];
	int msgTipoLib[50][17];
	int msgTipo3[50][17];

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

	if(numProc < 2 && numProc > 128 ){
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
	//Iniciamos los semáforos para controlar el numero de coches
	for(i=6;i<8;i++){
		if(semctl(datos.semid,i,SETVAL,1) == -1)
		{
			fprintf(stderr,"Error en la inicialización del semáforo de numCoches %d.\n",i);
			kill(getpid(),SIGTERM);		
		}
	}

	datos.memid=shmget(IPC_PRIVATE, 512, IPC_CREAT|0600); //La biblioteca usa 256 bytes 
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

	for(i=0;i<54;i++){
		for(j=0;j<17;j++){
			//msgTipoRes[i][j] = h;
			msgTipoLib[i][j] = h+1;
			//msgTipo3[i][j] = h+2;
			msg.tipo=msgTipoLib[i][j];
			strcpy(msg.texto,"1");
			if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
				fprintf(stderr,"No se manda un mensaje liberar a la posiocion %d %d\n",i,j);
			}
			h++;
		}
	}
	h=0;
		
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
	
	for(;;){
		ESPERA(sops,1,datos.semid);
		//tipoProceso=CRUCE_nuevo_proceso();
		//fprintf(stderr,"Soy el padre con PID %d con tipoProceso %d\n",getpid(),tipoProceso);
		tipoProceso=0;
		//Creamos un peaton y lo movemos para ver las posiciones por las que pasa


		if(tipoProceso==PEAToN){/*
			//Creamos un nuevo proceso para que gestione el peaton			
			switch (fork())
			{
			case -1:
				perror("Error al crear el proceso peaton.\n");
				kill(getpid(),SIGTERM);							//Llamamos a la manejadora
			case 0:
				//Proceso peaton
				pos1=CRUCE_inicio_peatOn_ext(&pos2);
									
				posAnt=pos1;

				fprintf(stderr, "Soy el peaton con PID %d.HOLA pos1.x=%d, pos1.y=%d pos2.x=%d pos2.y=%d\n", getpid(),
					pos1.x,pos1.y,pos2.x,pos2.y);

				do{

					if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLib[pos1.x][pos1.y],0)){
						fprintf(stderr,"Peaton %d ERROR ESPERAR A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
					} else {
						fprintf(stderr,"Peaton %d ESPERA BIEN A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
					}		
					
					pos3=CRUCE_avanzar_peatOn(pos1);
					
					fprintf(stderr, "Soy el peaton con PID %d.Avanzo pos1.x=%d, pos1.y=%d x=%d y=%d\n", getpid(),
					pos1.x,pos1.y,pos3.x,pos3.y);
					
					msg.tipo=msgTipoLib[posAnt.x][posAnt.y];
					strcpy(msg.texto,"1");
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
								if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLib[pos1.x][pos1.y],0)){
									fprintf(stderr,"Peaton %d ERROR ESPERAR A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
								} else {
									fprintf(stderr,"Peaton %d ESPERA BIEN A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
								}

								pos3=CRUCE_avanzar_peatOn(pos1);

								fprintf(stderr, "Soy el peaton con PID %d.Avanzo pos1.x=%d, pos1.y=%d x=%d y=%d\n", getpid(),
								pos1.x,pos1.y,pos3.x,pos3.y);

								
								msg.tipo=msgTipoLib[posAnt.x][posAnt.y];
								strcpy(msg.texto,"1");
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
								if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLib[pos1.x][pos1.y],0)){
									fprintf(stderr,"Peaton %d ERROR ESPERAR A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
								} else {
									fprintf(stderr,"Peaton %d ESPERA BIEN A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
								}

								pos3=CRUCE_avanzar_peatOn(pos1);

								fprintf(stderr, "Soy el peaton con PID %d.Avanzo pos1.x=%d, pos1.y=%d x=%d y=%d\n", getpid(),
								pos1.x,pos1.y,pos3.x,pos3.y);


								msg.tipo=msgTipoLib[posAnt.x][posAnt.y];
								strcpy(msg.texto,"1");
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
				
				}while((pos3.y>=0) || (pos3.x>0));

				msg.tipo=msgTipoLib[posAnt.x][posAnt.y];
				strcpy(msg.texto,"1");
					if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
						fprintf(stderr,"Peaton %d NO LIBERO x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
					} else { 
						fprintf(stderr,"Peaton %d LIBERO x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);	
					}
				
				fprintf(stderr,"Fin peaton.\n");
				CRUCE_fin_peatOn();
				SENHAL(sops,1,datos.semid); //Suma uno al semaforo
				kill(getpid(),SIGKILL);
				//return 0;
			}
		*/
		SENHAL(sops,1,datos.semid); //Suma uno al semaforo
		} else {
			//Creamos un nuevo proceso para que gestione el coche
			//ESPERA(sops,1,datos.semid);
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

				posAnt=pos1;
				
				if(pos1.y==10)			//C2
				{
					do{ 

					if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLib[pos1.x+8+3][pos1.y+3],0)){
					fprintf(stderr,"Coche %d ERROR ESPERAR A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
					} else {
						fprintf(stderr,"Coche %d ESPERA BIEN A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
					}

					
					pos3=CRUCE_avanzar_coche(pos1);
					
					if(flag2==1){
						msg.tipo=msgTipoLib[posAnt.x+3][posAnt.y+3];
						strcpy(msg.texto,"1");
						if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
							fprintf(stderr,"Coche %d NO LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);
						} else { 
							fprintf(stderr,"Coche %d LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);	
						}
					}else{
						flag2=1;
					}

					posAnt=pos1;
					
					
					fprintf(stderr, "Soy el coche con PID avanzo %d. x=%d, y=%d x=%d y=%d\n", getpid(),
					pos1.x,pos1.y,pos3.x,pos3.y);

					if((pos3.x==13) && (flag==0)){
						ESPERA(sops,SEM_IPC_C2,datos.semid); //Resta uno al semáforo
						fprintf(stderr, "Soy el coche con PID %d.Entro en el if Flag C2\n", getpid());
						//Se reserva la poscion del coche de atras y se comprueba si esta reservada la poscion del coche +2	


						flag=1;
						fprintf(stderr, "Soy el coche con PID %d.Pongo flag a 1\n", getpid());
						SENHAL(sops,SEM_IPC_C2,datos.semid); //Resta uno al semáforo
						//SENHAL(sops,7,datos.semid); //Suma uno al semaforo
					}

					if(j==0){
						pausa_coche();
						j++;
					}else
						j--;

					pos1=pos3;

				}while(pos3.y!=-2);

				}else{					//C1 con x=33
					do{ 

					if(-1==msgrcv(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),msgTipoLib[pos1.x+3][pos1.y+5+3],0)){
					fprintf(stderr,"Coche %d ERROR ESPERAR A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
					} else {
						fprintf(stderr,"Coche %d ESPERA BIEN A QUE SE LIBERE x=%d y=%d Errno: %d\n",getpid(),pos1.x,pos1.y,errno);
					}

					
					pos3=CRUCE_avanzar_coche(pos1);

					msg.tipo=msgTipoLib[posAnt.x+3][posAnt.y+3];
					strcpy(msg.texto,"1");
					if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
						fprintf(stderr,"Coche %d NO LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);
					} else { 
						fprintf(stderr,"Coche %d LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);	
					}

					posAnt=pos1;
					
					
					fprintf(stderr, "Soy el coche con PID avanzo %d. x=%d, y=%d x=%d y=%d\n", getpid(),
					pos1.x,pos1.y,pos3.x,pos3.y);
					
					if((pos3.y==6) && (flag==0)){
						ESPERA(sops,SEM_IPC_C1,datos.semid); //Resta uno al semáforo
						fprintf(stderr, "Soy el coche con PID %d.Entro en el if Flag C1\n", getpid());

						flag=1;
						fprintf(stderr, "Soy el coche con PID %d.Pongo flag a 1\n", getpid());
						SENHAL(sops,SEM_IPC_C1,datos.semid); //Resta uno al semáforo
						//SENHAL(sops,6,datos.semid); //Suma uno al semaforo
					}

					if(j==0){
						pausa_coche();
						j++;
					}else
						j--;

					pos1=pos3;

					}while(pos3.y!=-2);
				}

				msg.tipo=msgTipoLib[posAnt.x+3][posAnt.y+3];
				strcpy(msg.texto,"1");
				if(msgsnd(datos.buzon,&msg,sizeof(struct mensaje)- sizeof (long),0)==-1){
					fprintf(stderr,"Coche %d NO LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);
				} else { 
					fprintf(stderr,"Coche %d LIBERO x=%d y=%d Errno: %d\n",getpid(),posAnt.x,posAnt.y,errno);	
				}
				fprintf(stderr,"Fin coche %d.\n",getpid());
				CRUCE_fin_coche();
				SENHAL(sops,1,datos.semid); //Suma uno al semaforo
				kill(getpid(),SIGKILL);
				//return 0;	
			}
		}
	}
	return 0;
}
	
