#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <openssl/md5.h>

#define TAM 1024
#define SIZE 512

/* Variables globales */
ssize_t original;
FILE *Archivo;
char *token = NULL;
int32_t sockfd,a = 0, newsockfd;
struct sockaddr_in serv_addr;
struct hostent *server;
char path[]="/dev",buffer[TAM];


/* Declaración de funciones */
void handle1(void);
void HandlerSingal(int32_t signum);
void detectBoot(char *bootInd);
void conectar(__int32_t test);
void tablaMbr(char* pathLocal);
void tryToDownload(char * pathLocal);

int main(int argc, char *argv[]) {
    struct sigaction mysignal;
    uint16_t puerto;
    int32_t sigAction = 0,terminar = 0;
    char *etapas[] = {"usuario y contraseña: ", "comandos: user ls,user passwd <new>, file ls, file down <name>sd*:", "contrasena: "};
    mysignal.sa_handler = HandlerSingal;//Le paso mi handler a la estructura de sigaction
    if ((sigemptyset(&mysignal.sa_mask)) == -1) {//inicializa y vacía la señal
        perror("Error en sigaction");
        exit(EXIT_FAILURE);
    }
    mysignal.sa_flags = 0;// Lo hace re-ejecutable

    sigAction = sigaction(SIGINT, &mysignal, NULL);//Para capturar y manejar señales
    if (sigAction == -1) {
        perror("Error en sigaction");
        exit(EXIT_FAILURE);
    }

    if (argc < 3) {
        fprintf(stderr, "Uso %s host puerto\n", argv[0]);
        exit(0);
    }

    puerto = (uint16_t) atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR apertura de socket");
        exit(1);
    }
    newsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (newsockfd < 0) {
        perror("ERROR apertura de socket");
        exit(1);
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Error, no existe el host\n");
        exit(0);
    }
    memset((char *) &serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, (size_t) server->h_length);
    serv_addr.sin_port = htons(puerto);
    conectar(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));

    printf("-----------------Cliente-----------------\n");
    printf("Debe ingresar el usuario y contraseña separados por un espacio\n");
    printf("para acceder al servidor\n");
    while (1) {
        char pathLocal [10];
        char* device="";
        char sd[10];
        printf("\nIngrese su %s", etapas[a]);
        memset(buffer, '\0', TAM);
        if (fgets(buffer, TAM - 1, stdin) == 0) {
              printf("Interrupción\n");
        }
        // Verificando si se escribió: exit
        if (!strcmp("exit\n", buffer)) {
            strncpy(buffer, "exit", sizeof("exit"));
            terminar = 1;
            fprintf(stderr, "Exiting\n");

        }
        if(strstr(buffer, "file down <") != NULL){
            if(strstr(buffer, "[") != NULL){
            device=strstr(buffer, "[");
            strcpy(sd, device);
            sd[0] = '/';
            const char delimiter[] = "]";
            if((token = strtok(sd, delimiter))==NULL){
                strcpy(buffer, "Comando incompleto");
            }
            strcpy(sd, token);
            //long num =(int) strlen(sd);
            //device[strlen(device) - 1] = '\0';
            const char delimiters[] = ">";
            token = strtok(buffer, delimiters);
            strcpy(buffer, token);
           /* device = strstr(buffer, ">");
            char *buffer2=strstr(buffer, ">");
            unsigned long borrar= strlen(buffer2);
            buffer[strlen(buffer) - borrar] = '\0';
            device[0] = '/';
            //device[strlen(device) - 1] = '\0';*/
            printf("%s sd ingresado ",sd);
            printf("%s lo que envio",buffer);
            }else strcpy(buffer, "Comando incompleto");
        }

        if (send(sockfd, (void *) buffer, TAM, 0) < 0) {
            perror("No se pudo enviar el mensaje\n");
            exit(1);
        }

        memset(buffer, '\0', TAM);

        if (recv(sockfd, buffer, TAM, 0) < 0) {
            perror("No se pudo recibir el mensaje\n");
            exit(1);
        }
        printf("Respuesta del servidor: %s\n", buffer);

        if (!strcmp("Usuario bloqueado", buffer)) {
            printf("Supero la maxima cantidad de intentos\n");
            exit(0);

        }
        if (terminar) {
            printf("Finalizando ejecución\n");
            exit(0);
        }

        if (a != 2) {
            a++;
        }

        if (!strcmp("Usuario no encontrado", buffer)) {

            a = 0;
        } else {
            a = 1;//Posibilidad de ingresar un comando, sino sera contraseña invalida
        }
        if (!strcmp("Contraseña invalida", buffer)) {

            a = 2;
        }
        if (!strcmp("Confirmación de transferencia de datos", buffer)) {
            strcpy(pathLocal, path);
            //const char delimiter[] = ":";
            //token = strtok(buffer, delimiter);
            //original=atoi(token);
            //printf("Tamaño a descargar %ld\n",original);
            //strcat(pathLocal,path);
            strcat(pathLocal,sd);
            printf("-----------------Cliente-----------------\n");
            printf("Nueva conexion para trasferencia de archivos\n");
            printf("Path de escritura %s\n",pathLocal);
            serv_addr.sin_port = htons(8888);
            printf("Loading..........\n");
            tryToDownload(pathLocal);

            printf("No retire el pendrive hasta que termine de parpadear\n");
        }
    }

    return 0;
}
/**
 * Cierro posible archivo abierto y enviò exit al servidor
 */

void handle1(void) {
    if (Archivo != NULL) {
        fclose(Archivo);
    }
    if (send(sockfd, "exit", TAM, 0) < 0) {
        perror("No se pudo enviar el mensaje\n");
    }
}

/**
 * Atrapo la señal que genera el crtl c, envío al servidor exit y preparo al programa para finalizar
 * @param valor que general ctrl c
 */
void HandlerSingal(int32_t signum) {
    fprintf(stderr, "Recibi Ctrl-C (%d)\n", signum);
    strncpy(buffer, "exit", sizeof("exit"));
    if (send(sockfd, (void *) buffer, TAM, 0) < 0) {
        perror("No se pudo enviar el mensaje\n");
    }
    if (Archivo != NULL) {
        fclose(Archivo);
    }

    exit(1);
}

/**
 * Determina si es booteable e imprimo una respuesta
 * @param primer caracter de los 64 bytes
 */
void detectBoot(char *bootInd){
    if(!strcmp(bootInd, "8")){
        printf("Particion booteable\n");
    }else printf("Particion no booteable\n");
}

/**
 * Verificación de la conexión
 * @param valor retornado de la función connect
 */

void conectar(__int32_t test) {
    if (test< 0) {
        perror("No se pudo conectar");
        exit(1);
    }
}

/**
 * Se leen los archivos escritos y se obtienen los últimos 64 bytes para obtemer informaciòn del MBR
 * @param direcciòn del dispostivo a leer
 */
void tablaMbr(char* pathLocal){
    char mbr[SIZE];
    unsigned char buffer2[SIZE];
    char aux[SIZE]="";;
    Archivo = fopen(pathLocal,"rb");
    if(Archivo==NULL){
        perror("No se encontro la dirección del archivo descargado");
        return;
    }
    if((fread (buffer2,sizeof(char), 510, Archivo))>0){
        for(int32_t i = 0; i < 510; i++) {//para no imprimir los dos ultimos bytes
            //sprintf(mbr,"0x%x ",buffer2[i]);
            if(i>445){//Solo obtengo los 64 bytes que me interesan
                printf(" %02x ",buffer2[i]);//imprimo en hexa
                sprintf(aux, "%02x", buffer2[i]);
                strcat(mbr, aux);
            }
        }
    }
    fclose(Archivo);
    //printf(" \ncompruebo %lu\n", strlen(mbr));
    char bootInd[8];
//char idTypes[8];
//char sector[8];
//char partSize[8];
//int32_t pos1=7,pos2=10;//pos3,pos4;
//char *tmp="";
    int32_t inc=0;
    for(int32_t i = 0; i < 4; i++) {
        for(int32_t j = 0; j < 128; j++) {//para no imprimir los dos ultimos bytes
            //sprintf(mbr,"0x%x ",buffer2[i]);
            if(j==0+inc){//Solo obtengo los 64 bytes que me interesan
                //bootInd= atoi(mbr);
                sprintf(bootInd, "%c", mbr[j]);
                printf("Valor %s ",bootInd);
            }
            /*  if(j>pos1 && j <(pos2)){
                  sprintf(aux, "%c", mbr[j]);
                  strcat(tmp, aux);
              }

              if(j>9+inc && j <16+inc){
                  sprintf(aux, "%c", mbr[j]);
                  strcat(tmp, aux);
              }
              strcpy(sector, tmp);
              printf(" %s  ",sector);
              tmp="";
              if(j>15+inc && j <23+inc){
                  sprintf(aux, "%c", mbr[j]);
                  strcat(tmp, aux);
              }
              strcpy(partSize, tmp);
              printf(" %s  ",partSize);
              tmp="";*/
        }
        // sprintf(idTypes, "%s", tmp);

        //  printf(" %s  ",idTypes);

        // tmp="";
        // pos1=pos1+31;
        // pos2=pos2+31;
        inc=inc+31;
        detectBoot(bootInd);
    }
}

/**
 * Comprueba si se puede acceder al path ingresado por el usuario
 * Primero se realiza la conexión con el nuevo socket
 * Luego realiza la escritura e imprime el hash calculado
 * Por ultimo llama a la funciòn tablaMbr()
 * @param dirección para la escritura
 */
void tryToDownload(char * pathLocal){
    MD5_CTX md5;
    unsigned char c[MD5_DIGEST_LENGTH];

    printf("Conexión exitosa\n");
    Archivo = fopen(pathLocal,"rb");
    if(Archivo==NULL){
        perror("Error, revise la ubicación de su dispositivo");
        atexit(handle1);
        exit(EXIT_FAILURE);
    }
    fclose(Archivo);

    sleep(5);
    if(connect(newsockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))<0){
        perror("No se pudo conectar");
        //return;
        exit(EXIT_FAILURE);
    }
    Archivo = fopen(pathLocal,"wb");

    if (Archivo == NULL) {
        perror("error ");
        printf (" No se puede acceder al path %s\n", path);
        exit(EXIT_FAILURE);
    }
    unsigned char buffer2[SIZE];
    ssize_t bytes=0;
    ssize_t totalbytes=0;
    original=286261248;
    MD5_Init (&md5);//Le paso la estructura
    while(totalbytes<original){
        bytes=recv(newsockfd, buffer2, SIZE, 0);
        totalbytes=totalbytes+bytes;
        if(totalbytes>original){
            printf("SUPERE EL MAXIMO\n");
            bytes=SIZE-(totalbytes-original);
        }
        MD5_Update (&md5, buffer2, (size_t)bytes);
        fwrite(buffer2, 1, (size_t)bytes,Archivo);
    }
    MD5_Final (c,&md5);//Esto para el hash resultante
    fclose(Archivo);
    close(newsockfd);
    printf(" \nDatos\n");
    for(int32_t i = 0; i < MD5_DIGEST_LENGTH; i++) {
        printf("%02x", c[i]);//Imprimo al menos dos caracteres y pongo un cero si hay menos de dos;
    }
    printf(" hash del archivo recibido\n");
    printf("Total de bytes: %ld\n\n",totalbytes );
    tablaMbr(pathLocal);;
    close(newsockfd);
}