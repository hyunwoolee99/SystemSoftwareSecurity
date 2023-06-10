#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAXBUF 4096

int main(int argc, char* argv[]) {
    int serverSocket, clientSocket;
    struct sockaddr_in localAddr;
    char buffer[MAXBUF];
    int n;

    // 1. local server socket 생성
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket < 0) {
        perror("Cannot open socket");
        exit(EXIT_FAILURE);
    }

    // 2. localAddr 구조체 초기화 및 설정
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(8000);  // 사용하려는 포트번호

    // 3. local server socket을 local 주소에 bind
    if(bind(serverSocket, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 4. local server socket listen 시작
    if(listen(serverSocket, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    while(1) {
        // 5. 클라이언트의 연결 요청 수락
        clientSocket = accept(serverSocket, NULL, NULL);
        if(clientSocket < 0) {
            perror("Accept failed");
            continue;
        }

        // 6. 클라이언트의 요청을 받아 응답 처리
        while((n = read(clientSocket, buffer, MAXBUF)) > 0) {
            printf("Received a message: %s\n", buffer);
	    char response[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!";
            send(clientSocket, response, sizeof(response) - 1, 0);
        }

        // 7. 소켓 close
        close(clientSocket);
    }

    close(serverSocket);

    return 0;
}
