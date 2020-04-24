#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include "mensaje.h"
#include <time.h>

#define MAX 3
#define TAM 30
#define MAX2 1024

char path[MAX2]= "/home/cristian/Documentos/TP1_CVelazquez_38091823/base.txt";
/* Variables globales */
long posPass = 0;
int32_t terminar = 0,i = 0,msgid;
key_t clave1;
char *comando;
char usser[TAM] = "";
bool logueado = false;

/* Declaración de funciones */
void handle1(void);
char *concate(char *palabra1, char *palabra2);
char *listar();
char *ingreso(struct msgbuf cliente);
void cambiarPass(char *newPass, long pos);


struct msgbuf child_msg = {MSGTYPE1, {0, 0, "", "", "", ""}};


int main() {
    printf("SOY AUTHSERVICE\n");
    if ((clave1 = ftok(".", 's')) < 0) {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }
    msgid = msgget(clave1, 0666);//Me conecto a la cola creada
    if (msgid < 0) {
        printf("Error al acceder la cola de mensajes \n");
        exit(EXIT_FAILURE);
    }


    while (1) {
        printf("SOY EL HIJO Y VOY A ESCUCHAR\n");
        if ((msgrcv(msgid, &child_msg, sizeof child_msg.msg, MSGTYPE1, 0)) <
            0) {//msgtyp es 0 para leer el primer mensaje de la cola
            perror("");
            exit(EXIT_FAILURE);
        }

        printf("Recibi en authservice %s\n", child_msg.msg.comandos);

        if (!strcmp("exit", child_msg.msg.comandos)) {//Si recibo exit termino
            printf("TERMINAR\n");
            strcpy(child_msg.msg.texto, "exit");
            terminar = 1;
            printf("Soy el hijo recibi exit\n");
        } else {

            strcpy(child_msg.msg.texto,
                   "Comando invalido");//CUando el cliente no escribio bien un comando es el mensaje por defecto
            if (logueado == false) {//Solo para inicial cuando intenta loguearse
                strcpy(child_msg.msg.texto, ingreso(child_msg));
            }

            if (strstr(child_msg.msg.comandos, "user passwd <") != NULL && logueado == true) {
                comando = strstr(child_msg.msg.comandos, "<");
                comando[0] = ':';
                comando[strlen(comando) - 2] = '\0';//borro los caracteres ':' y '>'
                printf("LEI EL COMANDO%s\n", comando);
                cambiarPass(comando, posPass);
                strcpy(child_msg.msg.texto, "Se ha cambiado la contraseña con exito");
            }
            if (strstr(child_msg.msg.comandos, "user ls") != NULL && logueado == true) {
                printf("Entro en el comando user ls\n");
                strcpy(child_msg.msg.texto, listar());
            }

        }

        child_msg.mtype = MSGTYPE2;
        printf("SOY EL HIJO Y ESTOY POR ENVIAR\n");

        if ((msgsnd(msgid, &child_msg, sizeof child_msg.msg, 0)) <
            0) {
            perror("ERROR al enviar");
            exit(EXIT_FAILURE);
        }

        printf("YA ENVIE\n");

        if (terminar == 1) {
            printf("SOY EL PROCESO HIJO, TERMINE\n");
            exit(0);
        }
        //   exit(0);
    }

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
 * Usado para concatenar el usuario y pass ingreasa por el usuario
 * @param usuario
 * @param password
 * @return puntero de chars con el resultado
 */
char *concate(char *palabra1, char *palabra2) {
    int32_t long1 = (int32_t) strlen(palabra1);
    int32_t long2 = (int32_t) strlen(palabra2);
    int32_t suma = long1 + long2;
    char *auxiliar = malloc(sizeof(suma));//Debo guardar su espacio para poderlo retornar

    for (int32_t j = 0; j <= suma - 1; j++) {
        if (j <= long1 - 1) {
            auxiliar[j] = palabra1[j];
        } else {

            auxiliar[j] = palabra2[j - long1];
        }
    }
    return auxiliar;
}

/**
 * Busca los usuarios bloqueados en la base de datos
 * @return cadena con nombre de usuario y ultima conexiòn
 */
char *listar() {
    FILE *archivo;
    char usuario[TAM];
    char buffer[MAX2];
    char *linea;
    char lista[MAX2] = "";
    bool find = false;

    if ((archivo = fopen("/home/cristian/Documentos/base.txt", "r")) == NULL) {
        perror("Error al intentar abrir el archivo");
        exit(EXIT_FAILURE);
    }
    while (fgets(buffer, MAX2, (FILE *) archivo)!=NULL) {
        if ((linea = strstr(buffer, "usuario:")) != NULL) {
            sscanf(linea, "%s ", usuario);
            if(fgets(buffer, MAX2, (FILE *) archivo)==NULL){
                return "Error en lista";
            }
            if ((linea = strstr(buffer, "intentos:1")) != NULL) {

                find = true;
            }
        }
        if ((linea = strstr(buffer, "ulticonex:")) != NULL && find == true) {
            strcat(lista, usuario);
            strcat(lista, "       ");
            strcat(lista, linea);
            find = false;
        }
    }
    linea = lista;
    printf("%s\n", lista);
    fclose(archivo);
    return linea;
}

/**
 * Realiza busqueda en la base, edentifica usuarios bloqueados, verifica usuario y password ingresado
 * @param cliente es una estructura con que contiene el campo a leer
 * @return cadena de caractares con la respuesta de la busqueda
 */
char *ingreso(struct msgbuf cliente) {
    char a[TAM] = "", buffer[MAX2] = "", intento[TAM] = "intentos:0";
    FILE *archivo;//Defino al archivo que voy a leer (base)
    char *linea, *aux;
    char pass[TAM];
    bool usuarioEncontrado = false;
    long posIntentos;
    time_t tiempo;
    char *timeFormat;
    const char delimiters[] = " ";
    int32_t j = 0;
    char *token = NULL;
    if ((archivo = fopen("/home/cristian/Documentos/base.txt", "r+")) == NULL) {
        perror("Error al intentar abrir el archivo");
        exit(EXIT_FAILURE);
    }

    aux = cliente.msg.comandos;
    aux[strlen(aux) - 1] = '\0';
    if (strlen(usser) == 0) {
        token = strtok(aux, delimiters);
        while (token != NULL) {
            if (j == 0) {
                strcpy(usser, token);
            } else {
                strcpy(pass, token);
            }
            j++;
            token = strtok(NULL, delimiters);
        }
        printf("USUARIO: %s\n", usser);
        printf("CONTRASEÑA %s\n", pass);
    } else {
        strcpy(pass, aux);
        printf("YA TENGO GUARDADO EL USUARIO\n");
    }
    printf("ANTES usser es %s\n", usser);
    aux = "usuario:";
    aux = concate(aux,usser);

    while ((fgets(buffer, MAX2, (FILE *) archivo) &&
            usuarioEncontrado == false)) {//busqueda por linea lo guardado en buffer
        //printf("Ingreso la palabra %s\n", aux);
        if ((linea = strstr(buffer, aux)) != NULL) {//Solo entro si existe una coincidencia
            sscanf(linea, "%s ", a);//Guardo en "a" el valor encontrado
            if (strcmp(a, aux) == 0) {//Hago todo sobre la misma linea
                usuarioEncontrado = true;//Si encuentra al usuario deja de buscar por linea
                printf("Usuario encontrado es %s\n", usser);
                if ( fgets(buffer, MAX2, (FILE *) archivo)==NULL){
                    printf("ERROR en base,no se encuentra la linea intentos\n");
                    return "Usuario no encontrado";
                }
                if ((strstr(buffer, intento)) == NULL) {
                    printf("Usuario bloqueado\n");
                    terminar = 1;
                    fclose(archivo);
                    return "Usuario bloqueado";
                    exit(-1);
                }
                posIntentos = ftell(archivo) - 2;//Guardo la posicion donde me encuentro del fichero
                //printf("posicion: %i\n",(int)posIntentos);
                if ( fgets(buffer, MAX2, (FILE *) archivo)==NULL){
                    printf("ERROR en base, no se encuentra la linea pass\n");
                    return "Usuario no encontrado";
                }
                i++;
                aux = concate("pass:", pass);
                printf("Ingreso la %s\n", aux);
                if ((linea = strstr(buffer, aux)) != NULL) {
                    sscanf(linea, "%s ", a);
                    if (strcmp(a, aux) == 0) {
                        int32_t value = (int) strlen(linea) -
                                        4;//Le resto el tamaño de pass, : lo agrego cuando modifico la pass
                        posPass = ftell(archivo) - value;//Guardo la posicion donde me encuentro del fichero
                        printf("Se ha logueado con exito!\n");
                        break;
                    } else return "Contraseña invalida";

                } else {
                    if (i == MAX) {
                        printf("Usuario bloqueado, supero el limite de intentos\n");
                        fseek(archivo, posIntentos,
                              SEEK_SET);//Seek es para decirle que arranco desde el inicio y le doy la posicion
                        if (fwrite("1", 1, 1, archivo) != 1) {//Modifico la posicion anterior
                            perror("No se pudo realizar la modificacion");
                            exit(EXIT_FAILURE);
                        }
                        fclose(archivo);
                        terminar = 1;
                        return "Usuario bloqueado";
                    }
                    return "Contraseña invalida";
                }
                // }
            }
        }
    }

    if (usuarioEncontrado == false) {
        printf("El usuario ingresado no existe\n");
        fclose(archivo);
        memset(usser, '\0', TAM);
        return "Usuario no encontrado";

    }
    if(fgets(buffer, MAX2, (FILE *) archivo)==0){
        printf("ERROR en base,no se encuentra la linea ultima conexion\n");
        return "Usuario no encontrado";
    }
    linea = strstr(buffer, "ulticonex:");
    //printf("La ultima conexion es%s\n", linea);
    long value = (int) strlen(linea) - 10;
    value = ftell(archivo) - value;//Guardo la posicion donde me encuentro del fichero
    fseek(archivo, value, SEEK_SET);//Seek es para decirle que arranco desde el inicio y le doy la posicion
    tiempo = time(NULL);
    timeFormat = ctime(&tiempo);
    if (timeFormat == NULL) {
        perror("Error en la conversión");
        exit(EXIT_FAILURE);
    }

    if (fwrite(timeFormat, 1, strlen(timeFormat), archivo) != strlen(timeFormat)) {//Modifico la posicion anterior
        perror("No se pudo realizar la modificacion");
        exit(EXIT_FAILURE);
    }
    fclose(archivo);
    logueado = true;
    child_msg.msg.logueado = 1;
    return "Se ha logueado con exito!!";
}
/**
 * Cambia la contraseña del usuario
 * @param nueva constraseña
 * @param la posición a escribir en el txt (base de datos)
 * @return cadena de caracteres para informar al cliente
 */
void cambiarPass(char *newPass, long pos) {
    FILE *archivo;
    if ((archivo = fopen("/home/cristian/Documentos/base.txt", "r+")) == NULL) {
        perror("Error al intentar abrir el archivo");
        exit(EXIT_FAILURE);
    }

    fseek(archivo, pos, SEEK_SET);//Seek es para decirle que arranco desde el inicio y le doy la posicion

    if (fwrite("           ", 1, strlen("           "), archivo) != 11) {//Limpio la pass
        perror("No se pudo realizar la modificacion");
        exit(EXIT_FAILURE);
    }
    fseek(archivo, pos, SEEK_SET);
    if (fwrite(newPass, 1, strlen(newPass), archivo) != strlen(newPass)) {//Modifico la posicion anterior
        perror("No se pudo realizar la modificacion");
        exit(EXIT_FAILURE);

    }
    fclose(archivo);
    printf("Su nueva contraseña es: %s\n", newPass);
}

