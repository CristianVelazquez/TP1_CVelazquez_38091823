#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint-gcc.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include "mensaje.h"

#define SIZE 1024

/* Variables globales */
int32_t msgid, newsockfd2,pidHijo1,pidHijo2;
int32_t tcpsocket, newsockfd,data;
struct msgbuf child_msg = {MSGTYPE1, {0, 0, "", "", "", ""}};

/* Declaración de funciones */
void HandlerSingal(int32_t signum);

int main() {
    pid_t pid1, pid2;
    struct sockaddr_in *server;
    struct sockaddr_in *server2;
    struct sockaddr_in *cliente;
    char mensaje[SIZE];
    int32_t lenght_client;
    int32_t sigAction = 0;

    struct sigaction signal;
    signal.sa_handler = HandlerSingal;//Le paso mi handler a la estructura de sigaction
    if ((sigemptyset(&signal.sa_mask)) == -1) {//inicializa y vacía la señal
        perror("Error en sigaction");
        exit(EXIT_FAILURE);
    }
    signal.sa_flags = 0;// Lo hace re-ejecutable

    sigAction = sigaction(SIGINT, &signal, NULL);//Para capturar y manejar señales
    if (sigAction == -1) {
        perror("Error en sigaction");
        exit(EXIT_FAILURE);
    }


    key_t clave1; //Segun los parametros que use en ftok genero una clave unica
    if ((clave1 = ftok(".", 's')) < 0) {
        perror("ERROR al obtener el token");
        exit(EXIT_FAILURE);
    }


    tcpsocket = socket(AF_INET, SOCK_STREAM, 0);
    data =socket(AF_INET, SOCK_STREAM, 0); ///////////////////////////////

    if (tcpsocket < 0) {//devuelve un valor positivo si la llamada fue exitosa
        perror(" Error al crear el socket ");
        exit(EXIT_FAILURE);
    }

    if (data < 0) {//devuelve un valor positivo si la llamada fue exitosa////////////////////
        perror(" Error al crear el socket ");
        exit(EXIT_FAILURE);
    }

    server = calloc(1, sizeof(struct sockaddr_in));//vamos a alocar una sola estructura y limpiamos
    server->sin_family = AF_INET;
    server->sin_port = htons(4444);//guardo el puerto
    server->sin_addr.s_addr = INADDR_ANY; //Para escuchar en varias ips

    server2 = calloc(1, sizeof(struct sockaddr_in));//vamos a alocar una sola estructura y limpiamos///////////////
    server2->sin_family = AF_INET;
    server2->sin_port = htons(8888);//guardo el puerto
    server2->sin_addr.s_addr = INADDR_ANY; //Para escuchar en varias ips

    if (bind(tcpsocket, (struct sockaddr *) server, sizeof(struct sockaddr)) < 0) {
        perror("No se pudo asociar una direccion al socket");
        exit(EXIT_FAILURE);
    }
    if (bind(data, (struct sockaddr *) server2, sizeof(struct sockaddr)) < 0) {/////////////
        perror("No se pudo asociar una direccion al socket");
        exit(EXIT_FAILURE);
    }

    printf("Proceso: %d - socket disponible: %d\n", getpid(), ntohs(server->sin_port));

    listen(tcpsocket, 1);//Acepto un solo cliente

    cliente = calloc(1, sizeof(struct sockaddr_in)); //estructura donde guardo los datos del cliente
    lenght_client = (int32_t) sizeof(struct sockaddr_in);//largo del cliente


    while (1) {
        printf("Usted esta intentado conectarse al servidor\n");

        newsockfd = accept(tcpsocket, (struct sockaddr *) cliente,
                           (socklen_t *) &lenght_client);//casteo los parametros,aca acepto la conexion del cliente
        if (newsockfd < 0) {
            perror("accept");
            exit(1);
        }

        msgid = msgget(clave1, IPC_CREAT |
                               0666);//IPC_CREAT crea la cola sino existe, si existe falla. Tambien le otorgo perimos de lectura y escritura todos
        if (msgid < 0) {
            printf("Error al crear la cola de mensajes \n");
            exit(1);
        }
        char *args[] = {"/home/cristian/Documentos/TP1_CVelazquez_38091823/bin/auth",
                        "/home/cristian/Documentos/TP1_CVelazquez_38091823/bin/fileserv", NULL};

        pid1 = fork();

        if (pid1 < 0) {
            perror("fork");
            exit(1);
        }
        if (pid1 == 0) {
            pidHijo1=getpid();
            printf("PROCESO hijo %d encargado del authService\n", pidHijo1);
            execv(args[0], args);
        }

        pid2 = fork();

        if (pid2 < 0) {
            perror("fork");
            exit(1);
        }


        if (pid2 == 0) {
            pidHijo2=getpid();
            printf("PROCESO hijo %d. encarcado del filesService\n", pidHijo2);
            execv(args[1], args);
        }

        while (1) {

            if (recv(newsockfd, mensaje, sizeof(mensaje), 0) < 0) {
                perror("No se pudo recibir el mensaje\n");
                exit(1);
            }
            printf("PROCESO %d. ", getpid());
            printf("Recibí: %s\n", mensaje);

            strcpy(child_msg.msg.comandos, mensaje);
            if (strstr(mensaje, "file") != NULL && child_msg.msg.logueado == 1) {
                child_msg.mtype = 50;

            } else {
                child_msg.mtype = MSGTYPE1;
            }

            if ((msgsnd(msgid, &child_msg, sizeof child_msg.msg, 0)) <
                0) { ////////////////ANDA PERO E CLION DEBO VER LO rlt para que funcione
                perror("");
                exit(EXIT_FAILURE);
            }
            printf("Se enviaron mensajes \n");

            // if( (msgrcv(msgid, &parent_msg, TAMHIJO, MSGTYPE2, 0))<0){//msgtyp es 0 para leer el primer mensaje de la cola
            if ((msgrcv(msgid, &child_msg, sizeof child_msg.msg, MSGTYPE2, 0)) <
                0) {//msgtyp es 0 para leer el primer mensaje de la cola
                perror("");
                exit(EXIT_FAILURE);
            }
            listen(data, 1);
            printf("Mensaje a enviar al cliente: %s\n", child_msg.msg.texto);

            //bin

            if (send(newsockfd, child_msg.msg.texto, strlen(child_msg.msg.texto), 0) < 0) {
                perror("No se pudo enviar el mensaje\n");
                exit(1);
            }

            if (!strcmp("Confirmación de transferencia de datos", child_msg.msg.texto)) {
                printf("Esperando la nueva conexion\n");
                //close(newsockfd);
                newsockfd2 = accept(data, (struct sockaddr *) cliente,
                                    (socklen_t *) &lenght_client);//casteo los parametros,aca acepto la conexion del cliente
                if (data < 0) {
                    perror("accept");
                    exit(1);
                }
                printf("Soy el proceso padre listo para enviar\n");
                // while (1){
                ssize_t cantidad=0;
                ssize_t bytesrecv;
                while (1){
                    msgrcv(msgid, &child_msg, sizeof(child_msg.msg), MSGTYPE2, 0);
                    //  printf("Servidor recibe %s\n",child_msg.msg.data);
                    if(child_msg.msg.termine!=0){
                        break;
                        //msgrcv(msgid, &child_msg, sizeof child_msg.msg, MSGTYPE2, 0);
                    }
                    bytesrecv=sizeof(child_msg.msg.data);
                    //send(newsockfd2, child_msg.msg.texto, strlen(child_msg.msg.texto), 0)  ;
                    send(newsockfd2, child_msg.msg.data, sizeof(child_msg.msg.data), 0)  ;
                    // perror("No se pudo enviar el mensaje\n");
                    //exit(1);
                    cantidad=cantidad+bytesrecv;
                }
                printf("TAMAÑO ES !!!!!!!!!!! %ld\n", cantidad);
                /* if (recv(newsockfd2, mensaje, sizeof(mensaje), 0) < 0) {
                     perror("No se pudo recibir el mensaje\n");
                     exit(1);
                 }*/
                close(newsockfd2);
                //   }
            }
            // Verificación de si hay que terminar
            if (!strcmp("exit", mensaje) || !strcmp("Usuario bloqueado", child_msg.msg.texto)) {
                child_msg.mtype = 50;
                strcpy(child_msg.msg.comandos, "exit");
                if ((msgsnd(msgid, &child_msg, sizeof child_msg.msg, 0)) <
                    0) { ////////////////ANDA PERO E CLION DEBO VER LO rlt para que funcione
                    perror("");
                    exit(EXIT_FAILURE);
                }
                wait(NULL);
                wait(NULL);
                printf("PROCESO %d. Termino la ejecución del cliente\n\n", getpid());
                // inicio=false;
                close(newsockfd);
                break;
            }

        }

    }

    close(tcpsocket);
    return 0;
}

/**
 * Atrapo a la señal generada por ctrl C
 * Borro la cola de mensajes, mato los hijos porque ctrl c puede provocar un cierre forzoso
 * Cierro todos los sockets y finalizo el proceso sv
 * @param signum numero de la señal
 */
void HandlerSingal(__int32_t signum) {
    msgctl(msgid, IPC_RMID, NULL) ;
    fprintf(stderr, " Cierro sevidor porque recibi %d\n", signum);
    kill(pidHijo1,SIGKILL);
    kill(pidHijo2,SIGKILL);
    wait(NULL);
    wait(NULL);
    close(newsockfd2);
    close(newsockfd);
    close(tcpsocket);
    exit(-1);
}
