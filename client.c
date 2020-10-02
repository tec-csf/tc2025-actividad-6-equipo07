/*
 *  Created by 
 *      Saul Montes De Oca
 *      Carlos de la Garza
 *  1/10/2020
 *
 *  4 semaphore sync
 * 
 *  Client side
*/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 

#define FALSE 0
#define TRUE 1
#define TCP_PORT 8000
#define EXIT 111
#define INTERMITENTE 222
#define ENROJO 333

pid_t nextPID;
int semID;
int suspended = FALSE;
int semEnRojo = TRUE;
char bufferOut[1000] = "--------------- Semaforo ahora en ROJO --------------\n";
ssize_t leidos;
int cliente;

// SIGALRM
void enRojo(int senial)
{
    if (!suspended)
    {
        // Si el proceso NO esta suspendido
        // actualizando estado
        semEnRojo = TRUE;
        // imprimiendo en consola
        printf("--------------- Semaforo %d ahora en ROJO -------------- \n", semID);
        // definiendo bufferOut para enviar a servidor
        sprintf(bufferOut, "--------------- Semaforo %d ahora en ROJO --------------\n", semID);
        write(cliente, &bufferOut, sizeof(bufferOut));
        // envianso senial a prox proceso
        kill(nextPID, SIGUSR1);
    }
}

// SIGUSR1
void enVerde(int senial)
{
    // imprimiendo en consola
    printf("--------------- Semaforo %d ahora en VERDE -------------- \n", semID);
    // actualizando estado
    semEnRojo = FALSE;
    // definiendo bufferOut para enviar a servidor
    sprintf(bufferOut, "--------------- Semaforo %d ahora en Verde --------------\n", semID);
    write(cliente, &bufferOut, sizeof(bufferOut));
    // Poniendo en verde por 30 segundos
    alarm(3);
}


int main(int argc, const char * argv[])
{
    // **************************** socket code **************************
    struct sockaddr_in direccion;
    int buffer;
    
    ssize_t escritos;
    
    if (argc != 2) {
        printf("Use: %s IP_Servidor \n", argv[0]);
        exit(-1);
    }

    // ************************** Shared memory *********************
    // ftok to generate unique key 
    key_t key = ftok("shmfile", 65); 
  
    // shmget returns an identifier in shmid 
    int shmid = shmget(key , 5 * sizeof(pid_t), 0666|IPC_CREAT); 

    // Checking for error
    if(shmid == -1)
    {
        printf("ERROR al crear shm \n");
        exit(1);    
    }
  
    // shmat to attach to shared memory 
    pid_t *pidShMem = (pid_t*) shmat(shmid, (void*)0, 0); 
    // ******************************* ------ **************************


    // Definiendo manejador de SIGUSR1
    signal(SIGUSR1, enVerde);
    // Definiendo manejador de SIGALRM
    signal(SIGALRM, enRojo);

    // definiendo 3 semaforos hijos
    int children = 3;

    // Processes
    pid_t pid;
    pid_t *pidChildProcesses = (pid_t *) malloc((children-1) * sizeof(pid_t));
    pid_t *aux = pidChildProcesses;

    int i = 0;
    while (i < children)
    {
        pid = fork();

        // Codigo para los 3 semaforos hijos
        if (pid == 0)
        {
            // Escribiendo id del proceso en su respectivo lugar en shared memory
            pid_t *auxPid = pidShMem + (i+1); 
            *auxPid = getpid();
            // obtniendo pid del proceso a cual mandar señal SIGUSR1
            nextPID = *(pidShMem + i);
            // definiendo semId
            semID = i + 2;

            // Crear el socket
            cliente = socket(PF_INET, SOCK_STREAM, 0);
            
            // Establecer conexión
            inet_aton(argv[1], &direccion.sin_addr);
            direccion.sin_port = htons(TCP_PORT);
            direccion.sin_family = AF_INET;


            escritos = connect(cliente, (struct sockaddr *) &direccion, sizeof(direccion));

            if (escritos == 0)
            {   
                // Escribir datos en el socket
                while ((leidos = read(cliente, &buffer, sizeof(int)))) 
                {
                    //Salir
                    if (buffer == EXIT)
                    {
                        //detach from shared memory  
                        shmdt(pidShMem); 
                        
                        // destroy the shared memory 
                        shmctl(shmid,IPC_RMID,NULL); 
                        exit(0);
                    } 
                    //Poner semaforo en INTERMITENTE
                    else if(buffer == INTERMITENTE)
                    {
                        // Si ya estaba previamente suspendido en Intermintente
                        if (suspended)
                        {
                            // Quitar suspencion
                            printf("********  Continuar ********\n");
                            // definiendo bufferOut para enviar a servidor
                            sprintf(bufferOut, "******** Semaforo %d CONTINUA ********\n", semID);
                            write(cliente, &bufferOut, sizeof(bufferOut));
                            suspended = FALSE;
                            // Si el semaforo estaba en verde antes de la suspension
                            if (!semEnRojo)
                            {
                                raise(SIGUSR1);// regresar a ponerlo en verde con raise(SIGUSR1)
                            }
                            //sino se queda en estado rojo
                        } 
                        // Si no estaba previamente suspendido
                        else
                        {
                            //suspenderlo e imprimir estado
                            printf("******** Semaforos en INTERMITENTE ********\n");
                            // definiendo bufferOut para enviar a servidor
                            sprintf(bufferOut, "******** Semaforo %d en INTERMITENTE ********\n", semID);
                            write(cliente, &bufferOut, sizeof(bufferOut));
                            suspended = TRUE;
                            alarm(0);
                        }

                    } 
                    // Ponerlo en rojo
                    else if(buffer == ENROJO)
                    {
                        // Si ya estaba previamente suspendido en Intermintente
                        if (suspended)
                        {
                            // Quitar suspencion 
                            printf("******** Continuar ********\n");
                            // definiendo bufferOut para enviar a servidor
                            sprintf(bufferOut, "******** Semaforo %d CONTINUA ********\n", semID);
                            write(cliente, &bufferOut, sizeof(bufferOut));
                            suspended = FALSE;
                            // Si el semaforo estaba en verde antes de la suspension
                            if (!semEnRojo)
                            {
                                raise(SIGUSR1);// regresar a ponerlo en verde con raise(SIGUSR1)
                            }
                            //sino sequeda en estado rojo
                        } 
                        // Si no estaba previamente suspendido
                        else
                        {
                            //Poner semaforo en ENROJO
                            printf("******** Semaforos en ROJO ********\n");
                            // definiendo bufferOut para enviar a servidor
                            sprintf(bufferOut, "******** Semaforo %d en ROJO ********\n", semID);
                            write(cliente, &bufferOut, sizeof(bufferOut));
                            suspended = TRUE;
                            alarm(0);
                        }
                    }                        
                }
            }
        }
        // Error en fork()
        else if(pid == -1)
        {
            // Error al crear hijo
            printf("Hubo un error al crear proceso hijo. Numero de procesos hijos creados hasta ahora: %d\n", i + 1);
            break;
        }
        // codigo para parent process
        else
        {
            // para primera iteracion escribimos el pid padre en shm
            if(i == 0)
            {
                // Se escribe en el primer lugar de Shared memory
                pid_t *auxPid = pidShMem; 
                *auxPid = getpid();
                // escribiendo el pgrp por si se usa en el futuro
                auxPid = pidShMem + 4; 
                *auxPid = getpgrp();
            }
            // Guardando los pids de child processes para esperarlos despues
            *aux = pid;
        }
        i++;
        aux++;
    }

    // Proceso de semaforo para el padre
    sleep(1);// se espera un poco para visualizar mejor en terminal

    // Obteniendo pid del proceso a enviar SIGUSR1 el cual se encuentra en la ultima posicion de shared memory
    nextPID = *(pidShMem + 3);
    // Crear el socket
    cliente = socket(PF_INET, SOCK_STREAM, 0);
    
    // Establecer conexión
    inet_aton(argv[1], &direccion.sin_addr);
    direccion.sin_port = htons(TCP_PORT);
    direccion.sin_family = AF_INET;


    escritos = connect(cliente, (struct sockaddr *) &direccion, sizeof(direccion));
    // asignando semId
    semID = 1;

    // Levantandolo como primer semaforo en verde
    raise(SIGUSR1);

    if (escritos == 0) {
        // Escribir datos en el socket
        while ((leidos = read(cliente, &buffer, sizeof(int)))) {                        
            //Salir
            if (buffer == EXIT)
            {
                //detach from shared memory  
                shmdt(pidShMem); 
                
                // destroy the shared memory 
                shmctl(shmid,IPC_RMID,NULL); 
                exit(0);
            } 
            //Poner semaforo en INTERMITENTE
            else if(buffer == INTERMITENTE)
            {
                // Si ya estaba previamente suspendido en Intermintente
                if (suspended)
                {
                    // Quitar suspencion
                    printf("******** Continuar ********\n");
                    // definiendo bufferOut para enviar a servidor
                    sprintf(bufferOut, "******** Semaforo %d CONTINUA ********\n", semID);
                    write(cliente, &bufferOut, sizeof(bufferOut));

                    suspended = FALSE;
                    // Si el semaforo estaba en verde antes de la suspension
                    if (!semEnRojo)
                    {
                        raise(SIGUSR1);// regresar a ponerlo en verde con raise(SIGUSR1)
                    }
                    //sino se queda en estado rojo
                } 
                // Si no estaba previamente suspendido
                else
                {
                    //suspenderlo
                    printf("******** Semaforos en INTERMITENTE ********\n");
                    // definiendo bufferOut para enviar a servidor
                    sprintf(bufferOut, "******** Semaforo %d en INTERMITENTE ********\n", semID);
                    write(cliente, &bufferOut, sizeof(bufferOut));
                    suspended = TRUE;
                    alarm(0);
                }

            } 
            // Ponerlo en rojo
            else if(buffer == ENROJO)
            {
                // Si ya estaba previamente suspendido en Intermintente
                if (suspended)
                {
                    // Quitar suspencion 
                    printf("******** Continuar ********\n");
                    // definiendo bufferOut para enviar a servidor
                    sprintf(bufferOut, "******** Semaforo %d CONTINUA ********\n", semID);
                    write(cliente, &bufferOut, sizeof(bufferOut));

                    suspended = FALSE;
                    // Si el semaforo estaba en verde antes de la suspension
                    if (!semEnRojo)
                    {
                        raise(SIGUSR1);// regresar a ponerlo en verde con raise(SIGUSR1)
                    }
                    //sino sequeda en estado rojo
                } 
                // Si no estaba previamente suspendido
                else
                {
                    //Poner semaforo en ENROJO
                    printf("******** Semaforos en ROJO ********\n");
                    // definiendo bufferOut para enviar a servidor
                    sprintf(bufferOut, "******** Semaforo %d en ROJO ********\n", semID);
                    write(cliente, &bufferOut, sizeof(bufferOut));
                    suspended = TRUE;
                    alarm(0);
                }
            }                        
        }
    }

    // Proceso padre espera a todos sus hijos a que terminen
    aux = pidChildProcesses;
    pid_t *final = pidChildProcesses + (children-1);
    for (; aux < final; ++aux)
    {
        waitpid(*aux, NULL, 0);
    }
    
    //detach shared memory  
    shmdt(pidShMem); 
    
    // destroy shared memory 
    shmctl(shmid,IPC_RMID,NULL); 
    // free memory
    free(pidChildProcesses);

    return 0;
}
