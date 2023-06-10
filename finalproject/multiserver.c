#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_PORT 12345
#define MAXBUF 4096

// 새로운 클라이언트를 처리하기 위한 스레드 함수를 정의합니다.
void *handle_client(void *client_sock) {
    int sock = *(int *)client_sock;
    int http_sock;
    char buffer[MAXBUF];
    ssize_t bytes_received;
    struct sockaddr_in server;

    http_sock = socket(AF_INET , SOCK_STREAM , 0);
    if (http_sock == -1)
    {
        printf("Could not create http socket");
	return NULL;
    }
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 80 );
    //Connect to remote server
    if (connect(http_sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return NULL;
    }

    // 클라이언트로부터 데이터 수신
    while ((bytes_received = recv(sock, buffer, MAXBUF, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("Received data from client: %s\n", buffer);
        if(send(http_sock, buffer, bytes_received, 0) < 0)
        {
            fprintf(stderr, "Send failed");
        }
	break;
    }

    while ((bytes_received = read(http_sock, buffer, MAXBUF)) > 0) {
        buffer[bytes_received] = '\0';
        printf("Received data from http: %s\n", buffer);
	if(send(sock, buffer, bytes_received, 0) < 0)
        {
		fprintf(stderr, "Send failed");
        }
	break;
    }

    // 소켓 닫기
    close(sock);
    free(client_sock);

    return NULL;
}

int main() {
    int sockfd, *new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    
    // 소켓 생성
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // 소켓에 바인딩
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // 클라이언트 연결 대기
    if (listen(sockfd, 5) == -1) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", SERVER_PORT);

    // 클라이언트 연결 수락
    client_addr_len = sizeof(client_addr);
    while ((new_sock = malloc(sizeof(int)), *new_sock = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len))) {
        if (*new_sock == -1) {
            perror("Error accepting connection");
            continue;
        }

        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_t sniffer_thread;
        if (pthread_create(&sniffer_thread, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Error creating thread");
            return EXIT_FAILURE;
        }
    }

    if (*new_sock < 0) {
        perror("Error accepting connection");
        return EXIT_FAILURE;
    }

    close(sockfd);

    return 0;
}
