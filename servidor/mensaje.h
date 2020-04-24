#define MSGTYPE1   22
#define MSGTYPE2   23
struct mensaje {
    int logueado;
    int termine;
    char texto[1024];
    char exito[30];
    unsigned char data[512];
    char comandos[50];
};
struct msgbuf {/* Estructura para la cola*/
    long mtype;
    struct mensaje msg;
};


