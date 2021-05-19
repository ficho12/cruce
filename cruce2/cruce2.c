#include <stdio.h>
#include <windows.h>
#include "cruce2.h"
#include <stdlib.h>
#include <ctype.h>

#define SEM_IPC_C1 2
#define SEM_IPC_C2 3
#define SEM_IPC_P1 4
#define SEM_IPC_P2 5
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
struct datos{
    int fase;
}
DWORD WINAPI gestorSemaforico(LPVOID param);


int main(int argc, char* argv[]) {
    DWORD argumentos, velocidad, tmp, i=0;
    DWORD retorno;
    //HANDLE gestorSemaforico;

    HINSTANCE biblioteca;
    // Esta variable puntero a función se puede asociar a una función que devuelva un int
    // y reciba como parámetro 2 int
    // Los punteros a función pueden declararse como variables globales, ya que se usarán dentro
    // de las llamadas DWORD WINAPI. Puedo definirlas dentro de una estructura para tener
    // una única variable global que contenga todo.
    int (*inicio) (int, int);

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
        tmp = atoi(argv[2]);
    }
    else {
        printf("Error. El parametro numero de procesos debe ser un numero positivo.\n\n");
        ayuda();
        return 3;
    }
    
    DWORD const numProc = tmp;
    
    if (numProc < 2 && numProc > 50) {
        printf("Error, el numero de procesos tiene que ser mayor que 2 y menor a 128.\n\n");
        ayuda();
        return 3;
    }

    DWORD idHijo[50];
    HANDLE handleHilos[70];
    // Cargamos la biblioteca
    biblioteca = LoadLibrary("cruce2.dll");
    if (biblioteca == NULL) {
        PERROR("Error al cargar la biblioteca");
        return 10;
    }

    // Para cada función de la biblioteca que queramos usar hay que llamar a GetProcAddress
    // para asociar un puntero a función al códugo de la función en la biblioteca
    inicio = (int(*)(int, int))GetProcAddress(biblioteca, "CRUCE_inicio");
    if (inicio == NULL) {
        PERROR("Error al cargar CRUCE_inicio");
        FreeLibrary(biblioteca);
        return 11;
    }

    // Crear todos los recursos de sincronía que se necesiten en la práctica

    if (inicio(velocidad, numProc) == -1) {
        fprintf(stderr, "Error en la lamada a cruce inicio.\n");
        FreeLibrary(biblioteca);
        return 12;
    }

    handleHilos[i] = CreateThread(NULL, 0, gestorSemaforico, NULL, 0, &idHijo[i]);
    if (handleHilos[i] == NULL) {
        PERROR("Error al crear el hilo");
        return 13;
    }

    retorno = WaitForMultipleObjects(numProc, handleHilos, TRUE, INFINITE);
    if (retorno == WAIT_FAILED) {
        PERROR("Error al esperar por la muerte de los hilos");
        return 14;
    }

    printf("Los hilos han terminado\n");

    FreeLibrary(biblioteca);
    return 0;
}
/*void ambar(){
    if(datos.fase==2){
        CRUCE_pon_semAforo(SEM_C1, AMARILLO);
        pausa();
        pausa();
        pausa();
    }
    if(datos.fase==3){
        CRUCE_pon_semAforo(SEM_C2, AMARILLO);
        pausa();
        pausa();
        pausa();
    }
}*/

DWORD WINAPI gestorSemaforico(LPVOID param) {
    //int j;
    Sleep(10);
    printf("Soy el gestor Semaforico %ld \n", GetCurrentThreadId());
    Sleep(10);
    /*datos.fase=0
    for  (;;)  {
        if(datos.fase==1){
            //WAIT (SEM_IPC_C1)
            CRUCE_pon_semAforo(SEM_P1, ROJO);
        }
        CRUCE_pon_semAforo(SEM_P2, VERDE);
        //SEÑAL SEM_IPC_P2
        CRUCE_pon_semAforo(SEM_C1, VERDE);
        //SEÑAL SEM_IPC_C1

        for(j=0; j<6; j++){
            pausa();
        }

        datos.fase=2;
        //Segunda fase duracion 9 pausas
        //ESPERA SEM_IPC_C1
        //ambar
        CRUCE_pon_semAforo(SEM_C1, ROJO);
        //ESPERA SEM_IPC_C2
        CRUCE_pon_semAforo(SEM_P2, ROJO);
        CRUCE_pon_semAforo(SEM_C2, VERDE);
        //SEÑAL SEM_IPC_C2

         for(j=0; j<9; j++){
            pausa();
        }

        datos.fase=3;
        //Tercera fase duracion 12 pausas
        //ESPERA SEM_IPC_C2
        //ambar
        CRUCE_pon_semAforo(SEM_C2, ROJO);
        CRUCE_pon_semAforo(SEM_P1, VERDE);
        //SEÑAL SEM_IPC_C1

         for(j=0; j<12; j++){
            pausa();
        }
        datos.fase=1; 

    }*/

    return 0;
}


void funcion() {

}