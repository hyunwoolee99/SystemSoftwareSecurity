#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_PORT 12345
#define MAXBUF 4096
#define MAX_COMMAND_LEN 256
#define usage() fprintf(stderr, "Usage: %s {-l, -r}\n", argv[0]);

//local port forwarding을 할 지 remote port forwarding을 할 지 argv[1]를 통해 결정
typedef enum
{
	LOCAL_MODE,
	REMOTE_MODE,
} algo_t;

algo_t mode;

//handle client에서 사용할 자료형
typedef struct {
    int client_sock;
    int http_sock;
} thread_args_t;

//프로그램 종료 시 (socket) file descriptor 관리를 위한 배열
int fd_array[256];
int fd_count = 0;

//fd_array에 열려있는 (socket) file descriptor 번호를 저장
void save_fd(int fd) {
	fd_array[fd_count] = fd;
	fd_count++;
	return;
}

//fd_array에서 해당하는 (socket) file descriptor를 close, 이후 array에서 삭제
void close_fd(int fd) {
    for(int i = 0; i < fd_count; i++) {
        if(fd_array[i] == fd) {
            close(fd);
            fd_array[i] = fd_array[--fd_count];
            return;
        }
    }
}

void close_all_fd() {
	for(int i = 0; i < fd_count; i++) {
		close(fd_array[i]);
	}
	return;
}

void local_init(int *sockfd, struct sockaddr_in *server_addr) {
    // 소켓 생성
	*sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (*sockfd == -1) {
		perror("Error creating socket");
		exit(EXIT_FAILURE);
	}
	else {
        save_fd(*sockfd);
	}

	// 서버 주소 설정
	server_addr->sin_family = AF_INET;
	server_addr->sin_addr.s_addr = INADDR_ANY;
	server_addr->sin_port = htons(SERVER_PORT);

	// 소켓에 바인딩
	if (bind(*sockfd, (struct sockaddr *)server_addr, sizeof(*server_addr)) == -1) {
		perror("Error binding socket");
		exit(EXIT_FAILURE);
	}

	// 클라이언트 연결 대기
	if (listen(*sockfd, 5) == -1) {
		perror("Error listening for connections");
		exit(EXIT_FAILURE);
	}

	printf("Server listening on port %d\n", SERVER_PORT);
}

int local_accept(int *sockfd, int *new_sock, struct sockaddr_in *client_addr) {
    // 클라이언트 연결 수락
    socklen_t client_addr_len = sizeof(*client_addr);

    *new_sock = accept(*sockfd, (struct sockaddr *)client_addr, &client_addr_len);
    if (*new_sock == -1) {
        perror("Error accepting connection");
        free(new_sock);
        return EXIT_FAILURE;
    }
    printf("Client connected: %s:%d\n", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    return EXIT_SUCCESS;
}

int remote_init(int *sockfd, struct sockaddr_in *server_addr, char *client_ip) {
    // 소켓 생성
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd == -1) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }
    else {
        save_fd(*sockfd);
    }

    // 서버 주소 설정
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, client_ip, &(server_addr->sin_addr)) <= 0) {
        perror("Invalid server IP address");
        close_fd(*sockfd);
        return EXIT_FAILURE;
    }

    // 서버에 연결
    if(connect(*sockfd, (struct sockaddr *)server_addr, sizeof(*server_addr)) == -1) {
        perror("Error connecting to server");
        close_fd(*sockfd);
        return EXIT_FAILURE;
    }

    printf("connected\n");
}

int connect_http() {
    int http_sock;
    struct sockaddr_in server;
    http_sock = socket(AF_INET , SOCK_STREAM , 0);
	if (http_sock == -1)
	{
		printf("Could not create http socket");
		return EXIT_FAILURE;
	}
    else {
        save_fd(http_sock);
    }
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 80 );

	if (connect(http_sock , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("connect failed. Error");
		return EXIT_FAILURE;
	}
    return http_sock;
}

void sigint_handler(int sig_num) {
    close_all_fd();
	fprintf(stderr, "\nProgram terminated.\n");
	exit(0);
}

static const char *__color_start = "^[[0;31;40m";
static const char *__color_end = "^[[0m";

static void __print_prompt(void)
{
	char *prompt = "Insert IP address:";
	fprintf(stderr, "%s%s%s ", __color_start, prompt, __color_end);
}

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

// 새로운 클라이언트를 처리하기 위한 스레드 함수를 정의합니다.
void *handle_client(void *args) {
    thread_args_t *sockets = (thread_args_t *)args;
	int sock = sockets->client_sock;
    int http = sockets->http_sock;
	char buffer[MAXBUF];
	ssize_t bytes_received;

	// 클라이언트로부터 데이터 수신
    while(true) {
	    while ((bytes_received = recv(sock, buffer, MAXBUF, 0)) > 0) {
		    buffer[bytes_received] = '\0';
    		printf("Received data from client: %s\n", buffer);
	    	if(send(http, buffer, bytes_received, 0) < 0)
    		{
    			fprintf(stderr, "Send failed");
    		}
    	}

	    while ((bytes_received = read(http, buffer, MAXBUF)) > 0) {
	    	buffer[bytes_received] = '\0';
	    	printf("Received data from http: %s\n", buffer);
	    	if(send(sock, buffer, bytes_received, 0) < 0)
	    	{
	    		fprintf(stderr, "Send failed");
	    	}
	    }
    }

	close_fd(sock);
    close_fd(http);
	free(&sock);

	return NULL;
}

//client_socket을 받아 server의 http service에 forwarding
void port_forward(int *client_sock) {
    int http_sock;

    http_sock = connect_http();

    pthread_t client_thread;
    thread_args_t *args = malloc(sizeof(*args));
    args->client_sock = *client_sock;
    args->http_sock = http_sock;
    if (pthread_create(&client_thread, NULL, handle_client, (void *)args) < 0) {
        perror("Error creating thread");
        close_fd(*client_sock);
        free(client_sock);
        close_fd(http_sock);
    }
}

int main(int argc, char *argv[]) {
    int sockfd, *new_sock;
	struct sockaddr_in server_addr, client_addr;
    printBanner();
    if (argc != 2) {
		usage();
		return EXIT_FAILURE;
	}

	if (strcmp(argv[1], "-l") == 0) {
		mode = LOCAL_MODE;
	}
	else if (strcmp(argv[1], "-r") == 0) {
		mode = REMOTE_MODE;
	}
	else {
		usage();
		return EXIT_FAILURE;
	}

    //signal handle
	if (signal(SIGINT, sigint_handler) == SIG_ERR) {
		printf("Unable to set SIGINT handler. Exiting now...\n");
		return 1;
	}

    if (mode == LOCAL_MODE) {
        local_init(&sockfd, &server_addr);
        while(true){
            new_sock = malloc(sizeof(int));
            if (new_sock == NULL) {
                perror("Memory allocation failed");
                continue;
            }

            if(local_accept(&sockfd, new_sock, &client_addr) == EXIT_FAILURE) continue;

            port_forward(new_sock);
        }
    }
    else if (mode == REMOTE_MODE) {
        char command[MAX_COMMAND_LEN] = { '\0' };
        while (true) {
		    __print_prompt();

		    if (!fgets(command, sizeof(command), stdin)) break;

		    if (!strcmp(command, "q\n") || !strcmp(command, "exit\n")) break;
            else {
                char *pos;
                if ((pos = strchr(command, '\n')) != NULL) *pos = '\0';
            }

            new_sock = malloc(sizeof(int));
            if (new_sock == NULL) {
                perror("Memory allocation failed");
                continue;
            }
            if(remote_init(new_sock, &server_addr, command) == EXIT_FAILURE) continue;

            port_forward(new_sock);
	    }
    }

    close_all_fd();
    return 0;
}