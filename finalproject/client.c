#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "192.168.111.24"
#define CLIENT_PORT 8080
#define SERVER_PORT 12345
#define MAXBUF 4096