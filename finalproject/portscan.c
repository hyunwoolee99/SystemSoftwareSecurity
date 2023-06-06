#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    char target_ip[] = "127.0.0.1"; // 대상 호스트 IP 주소
    int start_port = 1; // 스캔 시작 포트
    int end_port = 65535; // 스캔 종료 포트

    struct sockaddr_in target_addr;
    int sockfd;

    // 소켓 생성
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 스캔 대상 호스트 설정
    target_addr.sin_family = AF_INET;
    target_addr.sin_addr.s_addr = inet_addr(target_ip);

    // 포트 스캔
    for (int port = start_port; port <= end_port; port++) {
        target_addr.sin_port = htons(port);

        // 연결 시도
        if (connect(sockfd, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0) {
            // 연결 실패 - 포트 닫혀 있음
            //printf("Port %d is closed.\n", port);
        } else {
            // 연결 성공 - 포트 열려 있음
            printf("Port %d is open.\n", port);
            close(sockfd);
        }
    }
	printf("port scan finished.\n");
    return 0;
}
