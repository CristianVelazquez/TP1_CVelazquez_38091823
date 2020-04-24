#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "mensaje.h"
#include <openssl/md5.h>
#include <dirent.h>
#include <stdbool.h>
#define SIZE 512
#define TAM 1024

/* Variables globales */
__int32_t msgid;
char *comando;
char pathProgram[]="/home/cristian/Documentos/TP1_CVelazquez_38091823";
struct msgbuf child_msg = {MSGTYPE1, {0, 0, "", "", "", ""}};

/* Declaración de funciones */
void handle1(void) ;
ssize_t trySend();
char *calcularHash(char *dir);
char* info(char path[],char *nombre);


int main()
{
    char pathDescarga[TAM]="/home/cristian/Documentos/programas";
    bool enviarIso=false;
    printf("SOY EL HIJO EJECUTANDO DESDE FILESERVICE!!!! \n");


    key_t clave1; //Segun los parametros que use en ftok genero una clave unica
    if( (clave1 = ftok(".",'s'))< 0 ) {
        perror( "ERROR" );
        exit(EXIT_FAILURE);
    }

    //Creacion de la cola*
    msgid = msgget(clave1, 0666);//IPC_CREAT crea la cola sino existe, si existe falla. Tambien le otorgo perimos de lectura y escritura todos
    if ( msgid < 0 ) {
        printf("Error al crear la cola de mensajes \n");
        exit(1);
    }
    while(1){
        printf("Esperando mensaje en filesService \n");
        if( (msgrcv(msgid, &child_msg, sizeof child_msg.msg, 50, 0))<0){//msgtyp es 0 para leer el primer mensaje de la cola
            perror( "" );
            exit(EXIT_FAILURE);}


        child_msg.mtype = MSGTYPE2;
//enviando el mensaje


        if (!strcmp("exit", child_msg.msg.comandos)) {//Si recibo exit termino
            printf("Soy el hijo filesService, recibi exit\n");
            atexit(handle1);
            exit(EXIT_FAILURE);
        } else {
            strcpy(child_msg.msg.texto,"Comando invalido");//Si no entra a ningun comando
            if (strstr(child_msg.msg.comandos, "file ls")) {
                DIR *dir;//Puntero de tipo DIR para realizar la busqueda en la carpeta deseada
                struct dirent *ent;//Declaro la estructura para usar las funciones de readdir
/*Leo el directorio*/

                dir=opendir(pathProgram);
                char text[TAM];
                memset(text, 0, TAM);
/*Reviso si hubo un error*/
                if(dir==NULL){
                    perror("%s No se logro abrir el directorio");
                    exit(EXIT_FAILURE);
                }

/*Realizo la busqueda por archivo del directorio indicado anteriormente*/
                while ((ent = readdir (dir)) != NULL)
                {
                    /*directorio actual (.) y el anterior (..) */
                    if ( (strcmp(ent->d_name, ".")!=0) && (strcmp(ent->d_name, "..")!=0) )
                    {
                        //Le paso el nombre del archivo a la funcion

                        //printf("file_name: \"%s\"\n",ent->d_name);
                        strcat(text,info(pathProgram,ent->d_name));

                    }
                }
                printf("%s\n",text);
                strcpy(child_msg.msg.texto,text);
            }
            if (strstr(child_msg.msg.comandos, "file down <")) {
                comando = strstr(child_msg.msg.comandos, "<");
                printf("LEI EL COMANDO%s\n", comando);
                comando[0] = '/';
                //comando[strlen(comando) - 2] = '\0';//borro los caracteres ':' y '>'
                printf("LEI EL COMANDO%s\n", comando);
                strcat(pathDescarga,comando);
               // strcpy(child_msg.msg.texto,"Confirmación de transferencia de datos");
                strcpy(child_msg.msg.texto,calcularHash(pathDescarga));
                enviarIso=true;
            }
        }

        if( (msgsnd(msgid, &child_msg, sizeof child_msg.msg, 0))<0){
            perror( "" );
            exit(EXIT_FAILURE);
        }
        printf("que pasooo\n");
        if(enviarIso==true){
            printf("Soy el hijo listo para enviar\n");
            FILE *Archivo = fopen (pathDescarga, "rb");
            //DEBERÌA HACER UN FOPEN ARRIBA PARA CORROBAR SI ESTA BIEN EL PATH, SINO MANDO A COMANDO INVALIDO
            //char data[1024];
            char data[SIZE];
            memset(data,0, sizeof(data));
            size_t bytes;
            size_t cantidad=0;
            while (  (bytes=fread (child_msg.msg.data, sizeof(char), 512, Archivo))> 0){
                //strcpy(child_msg.msg.texto,data);
                //data[strlen(data) - 1] = '\0';
                //strcpy(child_msg.msg.data,data);

                // printf("TAMAÑO ES !!!!!!!!!!! %ld\n", bytes);
                //  printf("FILE SERVICE ENVIA %s\n", child_msg.msg.data);
                cantidad=cantidad+bytes;
                if(trySend()<0){
                    //Hacer la llamada para tomar el error
                }


                // memset(data, '\0', 512);
                //memset(child_msg.msg.data, '\0', 512);
            }
            printf("TAMAÑO ES !!!!!!!!!!! %ld\n", cantidad);

            child_msg.msg.termine=1;
            trySend();

            fclose(Archivo);

            /*  if(tryRecv()<0){
                  //llamo a funcion de error
              }*/

        }
    }

    return 0;
}

/**
 * Borra la cola de mensajes
 */
void handle1(void) {
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("Eliminando cola : ");
    }
    printf("Se borro la cola\n");
}

/**
 * Indico la id donde voy a mensdar el mensaje y Obtengo un valor para usar en controles
 * @return valor que obtengo al mandar el mensaje
 */
ssize_t trySend() {
    child_msg.mtype = MSGTYPE2;
    return msgsnd(msgid, &child_msg, sizeof child_msg.msg, 0);
}
/**
 * Obtengo un valor para usar en controles
 * @return valor que obtengo recibir el mensaje
 */
ssize_t tryRecv(){
    return msgrcv(msgid, &child_msg, sizeof child_msg.msg, 50, 0); //DESPUES VEO EL MANEJO DE LOS ERRORES AQUI!!!!!!!!!!!!!veo que hago con lo que revuelve

}

/**
 * Calculo el hash y el tamaño del archivo a transferir
 * @param direcciñon donde se encuentra el archivo
 * @return cadena con respuesta
 */
char *calcularHash(char *dir){
    FILE * Archivo;
    MD5_CTX md5;
    size_t bytes=0;
    int32_t bytesTotales = 0;
    unsigned char c[MD5_DIGEST_LENGTH];
    char data[SIZE];
    // char texto[SIZE];
    Archivo = fopen (dir, "rb");
    if (Archivo == NULL) {
        printf ("%s No se pudo abrir el achivo\n", dir);
        return "Error en servidor, no se realizo el comando";
    }
    long sizeTotal=0;
    /*Aca configuro el md5*/
    MD5_Init (&md5);//Le paso la estructura
    do{
        bytes=fread (data, 1, 1, Archivo) ;
        MD5_Update (&md5, data, bytes); //Esto lo hago para hacerlo por partes porque son muchos datos que debo leer
        bytesTotales=bytesTotales+(int)bytes;
        sizeTotal++;
    } while ( bytes>0);

    MD5_Final (c,&md5);//Esto para el hash resultante

    /* for(int i = 0; i < MD5_DIGEST_LENGTH; i++) {
         printf("%02x", c[i]);//Imprimo al menos dos caracteres y pongo un cero si hay menos de dos;
     }*/
    // printf(" hash del archivo recibido\n");
    fclose (Archivo);
    printf("Total de bytes: %ld\n\n",sizeTotal );
/*char * tamTotal="";
    strcat(texto,"Tamaño");
    char* variable= (char *) (sizeTotal + '0');
    strcat(texto,variable);
    tamTotal=texto;
    printf("%s\n\n",tamTotal );*/
    return "Confirmación de transferencia de datos";

}
/**
 * Calculo el hash y tamaño de los archivos disponibles
 * @param direcciòn de la caparte con los archivos
 * @param nombre de archivo
 * @return cadena con la información
 */
char* info(char path[],char *nombre){
    unsigned char c[MD5_DIGEST_LENGTH];// CAMBIARLR LUEGO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    char direc[TAM]="";
    char resultado[TAM]="";
    char aux[TAM]="";
    char cantString[TAM]="";
    int32_t i;
    // float conver;
    char *salida=NULL;
    strcat(direc, path);
    strcat(direc, nombre);//EL resultado es la dirrecion de la carpeta con el nombre del archivo
    size_t bytes;
    FILE *Archivo = fopen (direc, "rb");
    MD5_CTX md5;
    // char data[1024];
    char data[SIZE];
    int32_t cantidad=0;//Variable para contar el tamaño del archivo


/*Reviso si hubo un error al abrir el archivo*/
    if (Archivo == NULL) {
        printf ("%s No se pudo abrir el achivo\n", direc);
        exit(EXIT_FAILURE);
    }
/*Aca configuro el md5*/
    MD5_Init (&md5);//Le paso la estructura
    do
    {
        //while ( fread (data, 1, 1, Archivo) != 0){
        //MD5_Update (&md5, data, strlen(data));
        bytes=fread (data, 1, 512, Archivo) ;
        MD5_Update (&md5, data, bytes); //Esto lo hago para hacerlo por partes porque son muchos datos que debo leer
        //cantidad++;
        cantidad=cantidad+(int)bytes;
    } while ( bytes>0);
    MD5_Final (c,&md5);//Esto para el hash resultante
    for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(aux, "%02x", c[i]); //Imprimo al menos dos caracteres y pongo un cero si hay menos de dos
        //printf("%02x", c[i]);//Imprimo al menos dos caracteres y pongo un cero si hay menos de dos
        strcat(resultado, aux);
    }

//    conver=(float)cantidad/1000000;
    memset(aux, 0, TAM);
    strcat(aux,"\n");
    strcat(aux,"Nombre: ");
    strcat(aux,nombre);
    //sprintf(cantString,"  Tamaño es %f (MB)", conver);
    sprintf(cantString,"  Tamaño es %d (bytes)", cantidad);
    strcat(aux,cantString);
    strcat(aux,"   Hash MD5 es:");
    strcat(aux,resultado);
    strcat(aux,"\n");
    //printf ("%s\n",aux);
    fclose (Archivo);
    salida=aux;
    return salida;
}