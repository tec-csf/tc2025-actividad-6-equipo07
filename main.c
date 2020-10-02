/*
 *  Created by 
 *      Saul Montes De Oca
 *      Carlos de la Garza
 *  1/10/2020
 *
 *  4 semaphore sync
 * 
 *  Server side
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 

#define EXIT 111
#define INTERMITENTE 222
#define ENROJO 333
#define TCP_PORT 8000

int cliente;
ssize_t leidos;
// id para identificar a que cliente se conecta cada socket
int semId;

void manejadorCtrlC(int senial)
{
    // enviar mensaje de ponerse en rojo
    printf("Enviando a semaforos ponerse en rojo\n");
    int signalToSend = ENROJO;
    write(semId, &signalToSend, sizeof(int));
}

void manejadorCtrlZ(int senial)
{
    // enviar mensaje de ponerse en intermitente
    // printf("Enviando a semaforos ponerse en intermitente\n");
    int signalToSend = INTERMITENTE;
    write(semId, &signalToSend, sizeof(int));
}

int main(int argc, char const *argv[])
{
    // Definiendo manejador de SIGUSR1
    signal(SIGINT, manejadorCtrlC);
    // Definiendo manejador de SIGALRM
    signal(SIGTSTP, manejadorCtrlZ);

    // *************** socket code ****************
    struct sockaddr_in direccion;
    char buffer[1000];
    
    int servidor, cliente;
    
    socklen_t escritos;
    int continuar = 1;
    pid_t pid;
    
    if (argc != 2) {
        printf("Use: %s IP_Servidor \n", argv[0]);
        exit(-1);
    }
    
    // Crear el socket
    servidor = socket(PF_INET, SOCK_STREAM, 0);
    
    // Enlace con el socket
    inet_aton(argv[1], &direccion.sin_addr);
    direccion.sin_port = htons(TCP_PORT);
    direccion.sin_family = AF_INET;
    
    bind(servidor, (struct sockaddr *) &direccion, sizeof(direccion));
    
    // Escuhar
    listen(servidor, 10);
    
    escritos = sizeof(direccion);
    // **********************   ****************

    int i= 0;
    // Aceptar conexiones
    while (continuar)
    {
        cliente = accept(servidor, (struct sockaddr *) &direccion, &escritos);
        // se guarda el cliente en i
        i = cliente;
        printf("Aceptando conexiones en %s:%d \n",
               inet_ntoa(direccion.sin_addr),
               ntohs(direccion.sin_port));
        
        pid = fork();
        
        if (pid == 0) continuar = 0;
        
    }
    
    if (pid == 0) {

        close(servidor);
        
        // se guarda el cliente (en i) en variable global semId
        semId = i;
        if (cliente >= 0) {
            
            // While para ecuchar en socket
            while (leidos = read(semId, &buffer, sizeof(buffer)))
            {
               write(fileno(stdout), &buffer, leidos);
            }
        }
        
        close(cliente);
    }
    
    else if (pid > 0)
    {
        while (wait(NULL) != -1);
        
        // Cerrar sockets
        close(servidor);
        
    }

    return 0;
}
