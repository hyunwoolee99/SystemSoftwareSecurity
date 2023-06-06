#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char message[1000] , server_reply[5000];
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP for www.google.com
    server.sin_family = AF_INET;
    server.sin_port = htons( 80 );
 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");
     
    //Send HTTP request
    char *httpRequest = "GET /leehw_xss.html HTTP/1.1\r\nHost: www\r\n\r\n";
    if( send(sock , httpRequest , strlen(httpRequest) , 0) < 0)
    {
        puts("Send failed");
        return 1;
    }
     
    //Receive a reply from the server
    if( recv(sock , server_reply , 5000 , 0) < 0)
    {
        puts("recv failed");
    }
     
    puts("Server reply :\n");
    puts(server_reply);
     
    close(sock);
    return 0;
}
