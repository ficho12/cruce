#include <stdio.h>
#include <windows.h>
#include "cruce2.h"
#include <stdlib.h>
#include <ctype.h>

void ayuda()
{
    printf("La practica se invocara especificando dos parámetros obligatorios desde la linea de ordenes.\n"
        "El primer parametro sera el numero maximo de procesos que puede haber en ejecucion simultanea.\n"
        "El segundo consistira en un valor entero mayor o igual que cero. Si es 1 o mayor, la practica funcionara\n"
        "tanto mas lenta cuanto mayor sea el parametro y no debera consumir CPU apreciablemente.Si es 0, ira a la\n"
        "maxima velocidad, aunque el consumo de CPU sí será mayor");
}

int verify(char* string)
{
    int x = 0;
    int len = strlen(string);

    while (x < len) {
        if (!isdigit(*(string + x)))
            return 1;

        ++x;
    }

    return 0;
}

struct datos {
    int fase;
    HINSTANCE biblioteca;
    HANDLE mutex[850];
    HANDLE semC1;
    HANDLE semC2;
    HANDLE semP1;
    HANDLE semP2;
    HANDLE semNumProc;
}datos;

DWORD WINAPI gestorSemaforico(LPVOID param);
DWORD WINAPI peaton(LPVOID param);
DWORD WINAPI coche(LPVOID param);


int (*inicio)(int, int); //Inicio cruce
int (*fin)(void); //Fin cruce
int (*inicioGestorSem)(void); //Inicio del gestor semaforico
int (*cambiaColor)(int, int);//Cambiar el color del semaforo
int (*nuevoProceso)(void); //Crear un coche o peaton
struct posiciOn(*inicioCoche)(void);
struct posiciOn(*avanzaCoche)(struct posiciOn);
int (*finCoche)(void);
struct posiciOn(*inicioPeaton)(void);
struct posiciOn(*avanzaPeaton)(struct posiciOn sgte);
int (*finPeaton)(void);
int (*pausa)(void);
int (*pausaCoche)(void);
int (*refrescar)(void);
void (*ponError)(const char* mensaje);


int main(int argc, char* argv[]) {
    int argumentos, velocidad, numProc, i = 0, z=0;
    int pos1, pos2, pos3, posAnt;
    DWORD retorno;
    //HANDLE gestorSemaforico;

    // Esta variable puntero a función se puede asociar a una función que devuelva un int
    // y reciba como parámetro 2 int
    // Los punteros a función pueden declararse como variables globales, ya que se usarán dentro
    // de las llamadas DWORD WINAPI. Puedo definirlas dentro de una estructura para tener
    // una única variable global que contenga todo.
    



    // Comprobación de parámetros
    argumentos = argc;
    /*if((argumentos != 3)){
        printf("Error en el numero de parametros\n\n");
        ayuda();
        return 1;
    }*/

    if (!verify(argv[1])) {
        velocidad = atoi(argv[1]);
    }
    else {
        printf("Error. El parametro velocidad debe ser un numero positivo.\n\n");
        ayuda();
        return 2;
    }
    if (velocidad < 0) {
        printf("Error. El parametro velocidad debe ser igual o mayor que cero.\n\n");
        ayuda();
        return 2;
    }

    if (!verify(argv[2])) {
        numProc = atoi(argv[2]);
    }
    else {
        printf("Error. El parametro numero de procesos debe ser un numero positivo.\n\n");
        ayuda();
        return 3;
    }

    if (numProc < 2 && numProc > 50) {
        printf("Error, el numero de procesos tiene que ser mayor que 2 y menor a 128.\n\n");
        ayuda();
        return 3;
    }

    DWORD idHijo[50];
    HANDLE handleHilos[52];
    // Cargamos la datos.biblioteca
    datos.biblioteca = LoadLibrary("cruce2.dll");
    if (datos.biblioteca == NULL) {
        PERROR("Error al cargar la datos.biblioteca");
        return 10;
    }

    // Para cada función de la datos.biblioteca que queramos usar hay que llamar a GetProcAddress
    // para asociar un puntero a función al códugo de la función en la datos.biblioteca
    inicio = (int(*)(int, int))GetProcAddress(datos.biblioteca, "CRUCE_inicio");
    if (inicio == NULL) {
        PERROR("Error al cargar CRUCE_inicio");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    fin = (int(*)())GetProcAddress(datos.biblioteca, "CRUCE_fin");
    if (fin == NULL) {
        PERROR("Error al cargar CRUCE_fin");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    inicioGestorSem = (int(*)())GetProcAddress(datos.biblioteca, "CRUCE_gestor_inicio");
    if (inicioGestorSem == NULL) {
        PERROR("Error al cargar CRUCE_gestor_inicio");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    nuevoProceso = (int(*)())GetProcAddress(datos.biblioteca, "CRUCE_nuevo_proceso");
    if (nuevoProceso == NULL) {
        PERROR("Error al cargar CRUCE_nuevo_proceso");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    inicioCoche = (struct posiciOn(*)(void))GetProcAddress(datos.biblioteca, "CRUCE_inicio_coche");
    if (inicioCoche == NULL) {
        PERROR("Error al cargar CRUCE_inicio_coche");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    avanzaCoche = (struct posiciOn(*)(struct posiciOn))GetProcAddress(datos.biblioteca, "CRUCE_avanzar_coche");
    if (avanzaCoche == NULL) {
        PERROR("Error al cargar CRUCE_avanzar_coche");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    finCoche = (int(*)())GetProcAddress(datos.biblioteca, "CRUCE_fin_coche");
    if (finCoche == NULL) {
        PERROR("Error al cargar CRUCE_fin_coche");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    inicioPeaton = (struct posiciOn(*)(void))GetProcAddress(datos.biblioteca, "CRUCE_nuevo_inicio_peatOn");
    if (inicioPeaton == NULL) {
        PERROR("Error al cargar CRUCE_nuevo_inicio_peatOn");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    avanzaPeaton = (struct posiciOn(*)(struct posiciOn))GetProcAddress(datos.biblioteca, "CRUCE_avanzar_peatOn");
    if (avanzaPeaton == NULL) {
        PERROR("Error al cargar CRUCE_avanzar_peatOn");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    finPeaton = (int(*)())GetProcAddress(datos.biblioteca, "CRUCE_fin_peatOn");
    if (finPeaton == NULL) {
        PERROR("Error al cargar CRUCE_fin_peatOn");
        FreeLibrary(datos.biblioteca);
        return 11;
    }


    pausa = (int(*)())GetProcAddress(datos.biblioteca, "pausa");
    if (pausa == NULL) {
        PERROR("Error al cargar pausa");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    pausaCoche = (int(*)())GetProcAddress(datos.biblioteca, "pausa_coche");
    if (pausaCoche == NULL) {
        PERROR("Error al cargar pausa_coche");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    refrescar = (int(*)(void))GetProcAddress(datos.biblioteca, "refrescar");
    if (refrescar == NULL) {
        PERROR("Error al cargar refrescar");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    ponError = (void (*)(const char* mensaje))GetProcAddress(datos.biblioteca, "pon_error");
    if (ponError == NULL) {
        PERROR("Error al cargar pon_error");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    cambiaColor = (int(*)(int, int))GetProcAddress(datos.biblioteca, "CRUCE_pon_semAforo");
    if (cambiaColor == NULL) {
        PERROR("Error al cargar CRUCE_pon_semAforo");
        FreeLibrary(datos.biblioteca);
        return 11;
    }

    datos.semC1 = CreateSemaphore(NULL, 0, 1, NULL);
    if (datos.semC1 == NULL) {
        PERROR("Error al crear el semáforo C1");
        return 1;
    }

    datos.semC2 = CreateSemaphore(NULL, 0, 1, NULL);
    if (datos.semC2 == NULL) {
        PERROR("Error al crear el semáforo C2");
        return 1;
    }

    datos.semP1 = CreateSemaphore(NULL, 0, 1, NULL);
    if (datos.semP1 == NULL) {
        PERROR("Error al crear el semáforo P1");
        return 1;
    }

    datos.semP2 = CreateSemaphore(NULL, 0, 1, NULL);
    if (datos.semP2 == NULL) {
        PERROR("Error al crear el semáforo P2");
        return 1;
    }

    datos.semNumProc = CreateSemaphore(NULL, 2, numProc, NULL);
    if (datos.semNumProc == NULL) {
        PERROR("Error al crear el semáforo semNumProc");
        return 1;
    }

    // Crear todos los recursos de sincronía que se necesiten en la práctica

    if (inicio(velocidad, numProc) == -1) {
        fprintf(stderr, "Error en la lamada a cruce inicio.\n");
        FreeLibrary(datos.biblioteca);
        return 12;
    }

    handleHilos[z] = CreateThread(NULL, 0, gestorSemaforico, NULL, 0, &idHijo[z]);
    if (handleHilos[z] == NULL) {
        PERROR("Error al crear el hilo");
        return 13;
    }
    z++;

    


    int h = 0;
    for (i = 0; i < 51; i++) {
        for (int j = 0; j < 18; j++) {
            datos.mutex[h]= CreateMutex(NULL, FALSE, "Pepito");
            h++;
        }
    }

    for(;;){
        int tipo;
        tipo = nuevoProceso;
        if(tipo==PEAToN){       //Proceso peaton
            
            handleHilos[z] = CreateThread(NULL, 0, peaton, NULL, 0, &idHijo[z]);
            if (handleHilos[i] == NULL) {
                PERROR("Error al crear el hilo");
                return 13;
            }
            z++;

        }else{      //Proceso coche

            handleHilos[z] = CreateThread(NULL, 0, coche, NULL, 0, &idHijo[z]);
            if (handleHilos[i] == NULL) {
                PERROR("Error al crear el hilo");
                return 13;
            }
            z++;

        }

    }

    retorno = WaitForMultipleObjects(1, handleHilos, TRUE, INFINITE);
    if (retorno == WAIT_FAILED) {
        PERROR("Error al esperar por la muerte de los hilos");
        return 14;
    }

    printf("Los hilos han terminado\n");

    FreeLibrary(datos.biblioteca);
    return 0;
}

void ambar(){

    cambiaColor = (int(*)(int, int))GetProcAddress(datos.biblioteca, "CRUCE_pon_semAforo");
    if (cambiaColor == NULL) {
        PERROR("Error al cargar CRUCE_pon_semAforo");
        FreeLibrary(datos.biblioteca);
        //return 11;
    }
    
    if(datos.fase==2){
        cambiaColor(SEM_C1, AMARILLO);
        pausa();
        pausa();
        pausa();
    }
    if(datos.fase==3){
        cambiaColor(SEM_C2, AMARILLO);
        pausa();
        pausa();
        pausa();
    }
}

DWORD WINAPI gestorSemaforico(LPVOID param) {
    int j;
    bool ret;
    DWORD err;

    datos.fase = 0;
    cambiaColor(SEM_C1, ROJO);
    cambiaColor(SEM_C2, ROJO);
    cambiaColor(SEM_P1, ROJO);
    cambiaColor(SEM_P2, ROJO);

    for  (;;)  {
        if(datos.fase==1){
            err = WaitForSingleObject(datos.semP1, INFINITE);
            switch(err) {
                case WAIT_ABANDONED:                PERROR("Error en el wait semP1 WAIT_ABANDONED");
                case WAIT_OBJECT_0:                 PERROR("Error en el wait semP1 WAIT_OBJECT_0");
                case WAIT_TIMEOUT:                  PERROR("Error en el wait semP1 WAIT_TIMEOUT");
                case WAIT_FAILED:                   PERROR("Error en el wait semP1 WAIT_FAILED");
            }
            cambiaColor(SEM_P1, ROJO);
        }
        cambiaColor(SEM_P2, VERDE);
        ret = ReleaseSemaphore(datos.semP2,1,0);//Comprobar errores
        if(ret== FALSE){
            PERROR("Error al hacer el signal en P2");
        }

        cambiaColor(SEM_C1, VERDE);
        ret = ReleaseSemaphore(datos.semC1,1,0);
        if(ret== FALSE){
            PERROR("Error al hacer el signal en C1");
        }

        for(j=0; j<6; j++){
            pausa();
        }

        datos.fase=2;
        //Segunda fase duracion 9 pausas
        err = WaitForSingleObject(datos.semC1, INFINITE);
        switch (err) {
            case WAIT_ABANDONED:                PERROR("Error en el wait semP1 WAIT_ABANDONED");
            case WAIT_OBJECT_0:                 PERROR("Error en el wait semP1 WAIT_OBJECT_0");
            case WAIT_TIMEOUT:                  PERROR("Error en el wait semP1 WAIT_TIMEOUT");
            case WAIT_FAILED:                   PERROR("Error en el wait semP1 WAIT_FAILED");
        }
        ambar();
        cambiaColor(SEM_C1, ROJO);
        err = WaitForSingleObject(datos.semP2, INFINITE);
        switch (err) {
            case WAIT_ABANDONED:                PERROR("Error en el wait semP1 WAIT_ABANDONED");
            case WAIT_OBJECT_0:                 PERROR("Error en el wait semP1 WAIT_OBJECT_0");
            case WAIT_TIMEOUT:                  PERROR("Error en el wait semP1 WAIT_TIMEOUT");
            case WAIT_FAILED:                   PERROR("Error en el wait semP1 WAIT_FAILED");
        }
        cambiaColor(SEM_P2, ROJO);

        cambiaColor(SEM_C2, VERDE);
        ret = ReleaseSemaphore(datos.semC2,1,0);
        if(ret == FALSE){
            PERROR("Error al hacer el signal en C2");
        }

         for(j=0; j<9; j++){
            pausa();
        }

        datos.fase=3;
        //Tercera fase duracion 12 pausas
        err = WaitForSingleObject(datos.semC2, INFINITE);
        switch (err) {
            case WAIT_ABANDONED:                PERROR("Error en el wait semP1 WAIT_ABANDONED");
            case WAIT_OBJECT_0:                 PERROR("Error en el wait semP1 WAIT_OBJECT_0");
            case WAIT_TIMEOUT:                  PERROR("Error en el wait semP1 WAIT_TIMEOUT");
            case WAIT_FAILED:                   PERROR("Error en el wait semP1 WAIT_FAILED");
        }
        ambar();
        cambiaColor(SEM_C2, ROJO);

        cambiaColor(SEM_P1, VERDE);
        ret=ReleaseSemaphore(datos.semP1,1,0);
        if(ret == FALSE){
            PERROR("Error al hacer el signal en P1");
        }

         for(j=0; j<12; j++){
            pausa();
        }
        datos.fase=1;

    }

    return 0;
}

/*DWORD WINAPI peaton(LPVOID param) {
    pos1 = inicioPeaton(&pos2);
    posAnt = pos2;
    do{
        do{
            pos3 = avazarPeaton(pos1);
            //Aqui mensaje libera
            posAnt = pos1;
            if(pos3.y==11){
                if((pos3.x>=21) && (pos3.x<=27)){
                    //Esperas P2
                    pos1 = pos3;
                    do{
                        pos3 = avazarPeaton(pos1);
                        //Mesaje libera
                        posAnt = pos1;
                        pausa();
                        pos1 = pos3;
                    } while (pos3.y != 6);
                //Señal P2
                }
            }
    }
}

DWORD WINAPI coche(LPVOID param) {

}*/