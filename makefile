# Directorios
CLIENT_SOURCE=cliente
SERVER_SOURCE=servidor
BINARY_DIR=bin

#Binarios
CLIENT=client
SERVER=server
SERVER_FS=fileserv
SERVER_AUTH=auth
DOXYGEN = doxygen

# Opt de compilacion 
CC=gcc
CFLAGS=-std=gnu11  -Wall -Werror -pedantic -Wextra -Wconversion -O1

all: client server

client : $(CLIENT_SOURCE)/main.c
	mkdir -p $(BINARY_DIR)
	@echo Building $@
	$(CC) $(CFLAGS)  -o $(BINARY_DIR)/$(CLIENT).o -c $(CLIENT_SOURCE)/main.c -lssl -lcrypto
	$(CC) $(CFLAGS)  -o $(BINARY_DIR)/$(CLIENT)   $(BINARY_DIR)/$(CLIENT).o -lssl -lcrypto

server : $(SERVER_SOURCE)/server.c $(SERVER_SOURCE)/mensaje.h $(SERVER_SOURCE)/fileserv.c $(SERVER_SOURCE)/auth.c
	mkdir -p $(BINARY_DIR)
	$(CC) $(CFLAGS)  -o $(BINARY_DIR)/$(SERVER).o -c $(SERVER_SOURCE)/server.c -lrt
	$(CC) $(CFLAGS)  -o $(BINARY_DIR)/$(SERVER)   $(BINARY_DIR)/$(SERVER).o -lrt
	$(CC) $(CFLAGS)  -o $(BINARY_DIR)/$(SERVER_FS).o -c $(SERVER_SOURCE)/fileserv.c -lrt -lssl -lcrypto
	$(CC) $(CFLAGS)  -o $(BINARY_DIR)/$(SERVER_FS)   $(BINARY_DIR)/$(SERVER_FS).o -lrt -lssl -lcrypto
	$(CC) $(CFLAGS)  -o $(BINARY_DIR)/$(SERVER_AUTH).o -c $(SERVER_SOURCE)/auth.c -lrt
	$(CC) $(CFLAGS)  -o $(BINARY_DIR)/$(SERVER_AUTH)   $(BINARY_DIR)/$(SERVER_AUTH).o -lrt

.PHONY: clean
clean:
	rm  -Rf $(BINARY_DIR)

