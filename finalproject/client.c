#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "10.168.18.189"
#define SERVER_PORT 12345

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // 소켓 생성
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0) {
        perror("Invalid server IP address");
        exit(EXIT_FAILURE);
    }

    // 서버에 연결
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    // 서버로 데이터 전송
    char message[] = "Hello, server!";
    if (send(sockfd, message, strlen(message), 0) == -1) {
        perror("Error sending data to server");
        exit(EXIT_FAILURE);
    }

    printf("Data sent to server: %s\n", message);

    // 소켓 닫기
    close(sockfd);

    return 0;
}

