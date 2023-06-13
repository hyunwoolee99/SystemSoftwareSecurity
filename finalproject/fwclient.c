#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "192.168.214.24"
#define CLIENT_PORT 8080
#define SERVER_PORT 12345
#define MAXBUF 4096

void printBanner() {
	int fd;
	char buf[MAXBUF];
	ssize_t bytes;
	fd = open("banner.txt", O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "banner open failed.\n");
		return;
	}
	while((bytes = read(fd, buf, MAXBUF)) > 0) {
		buf[bytes] = '\0';
		write(1, buf, bytes);
		break;
	}
	close(fd);
	return;
}



int main() {
    int sockfd, server_sock, http_sock;
    struct sockaddr_in server_addr, http_addr;
    socklen_t http_addr_len;

    char buffer[MAXBUF];
    ssize_t bytes_received;

    printBanner();

    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
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
    connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(server_sock == -1) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    printf("connected\n");


    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (http_sock == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    http_addr.sin_family = AF_INET;
    http_addr.sin_addr.s_addr = INADDR_ANY;
    http_addr.sin_port = htons(CLIENT_PORT);

    // 소켓에 바인딩
    if (bind(sockfd, (struct sockaddr *)&http_addr, sizeof(http_addr)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // 클라이언트 연결 대기
    if (listen(sockfd, 5) == -1) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    //printf("Server listening on port %d\n", CLIENT_PORT);

    // 클라이언트 연결 수락
    http_addr_len = sizeof(http_addr);
    http_sock = accept(sockfd, (struct sockaddr *)&http_addr, &http_addr_len);
    if(http_sock == -1) {
            perror("Error accepting connection");
            exit(EXIT_FAILURE);
        }

    printf("Client connected: %s:%d\n", inet_ntoa(http_addr.sin_addr), ntohs(http_addr.sin_port));

    while((bytes_received = read(http_sock, buffer, MAXBUF)) > 0) {
        buffer[bytes_received] = '\0';
        printf("Received data from client: %s\n", buffer);
	printf("bytes_received:%d\n", bytes_received);
	printf("server_sock:%d\n", http_sock);
        if(send(server_sock, buffer, bytes_received, 0) < 0)
	{
		fprintf(stderr, "Send to server failed\n");
	}
	printf("Data send\n");
	break;
    }
    while((bytes_received = recv(server_sock, buffer, MAXBUF, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("Received data from server: %s\n", buffer);
        if(send(http_sock, buffer, bytes_received, 0) < 0)
        {
            fprintf(stderr, "Send failed\n");
        }
	break;
    }

    // 소켓 닫기
    close(sockfd);
    close(http_sock);
    return 0;
}


