#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define CLIENT_PORT 8080
#define SERVER_PORT 12345
#define MAXBUF 4096
#define MAX_COMMAND_LEN 16
#define usage() fprintf(stderr, "Usage: %s {-l, -r}\n", argv[0]);

typedef enum
{
	LOCAL_MODE,
	REMOTE_MODE,
} algo_t;

algo_t mode;

int fd[2];

void close_all_fd() {
    if(fd[0] != 0) close(fd[0]);
    if(fd[1] != 0) close(fd[1]);
}

int local_init(int *sockfd, char *server_ip) {
    struct sockaddr_in server_addr;
    // 소켓 생성
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd == -1) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }
    else {
        fd[0] = *sockfd;
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, server_ip, &(server_addr.sin_addr)) <= 0) {
        perror("Invalid server IP address");
        close(*sockfd);
        fd[0] = 0;
        return EXIT_FAILURE;
    }

    // 서버에 연결
    if(connect(*sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to server");
        close(*sockfd);
        fd[0] = 0;
        return EXIT_FAILURE;
    }

    printf("connected\n");
}

void remote_init(int *sockfd, struct sockaddr_in *server_addr) {
    // 소켓 생성
	*sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (*sockfd == -1) {
		perror("Error creating socket");
		exit(EXIT_FAILURE);
	}
	else {
        fd[0] = *sockfd;
	}

	// 서버 주소 설정
	server_addr->sin_family = AF_INET;
	server_addr->sin_addr.s_addr = INADDR_ANY;
	server_addr->sin_port = htons(CLIENT_PORT);

	// 소켓에 바인딩
	if (bind(*sockfd, (struct sockaddr *)server_addr, sizeof(*server_addr)) == -1) {
		perror("Error binding socket");
        close(fd[0]);
		exit(EXIT_FAILURE);
	}

	// 클라이언트 연결 대기
	if (listen(*sockfd, 5) == -1) {
		perror("Error listening for connections");
        close(fd[0]);
		exit(EXIT_FAILURE);
	}

	printf("Client listening on port %d\n", CLIENT_PORT);
}

void remote_accept(int *sockfd, struct sockaddr_in *server_addr) {
    // 클라이언트 연결 수락
    socklen_t client_addr_len = sizeof(*server_addr);

    *sockfd = accept(*sockfd, (struct sockaddr *)server_addr, &client_addr_len);
    if (*sockfd == -1) {
        perror("Error accepting connection");
        close(fd[0]);
        exit(EXIT_FAILURE);
    }
    printf("Client connected: %s:%d\n", inet_ntoa(server_addr->sin_addr), ntohs(server_addr->sin_port));
}

int connect_http() {
    int http_sock;
    struct sockaddr_in server;
    http_sock = socket(AF_INET , SOCK_STREAM , 0);
	if (http_sock == -1)
	{
		printf("Could not create http socket");
		exit(EXIT_FAILURE);
	}
    else {
        fd[1] = http_sock;
    }
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 80 );

	if (connect(http_sock , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("connect failed. Error");
        close(fd[1]);
		exit(EXIT_FAILURE);
	}
    return http_sock;
}

//client_socket을 받아 server의 http service에 forwarding
void port_forward(int *sockfd) {
    int server_sock = *sockfd;
    int http_sock;
    char buffer[MAXBUF];
    char plaintext[MAXBUF];
	ssize_t bytes_received;
    unsigned char shared_key[128];

    http_sock = connect_http();

    shared_key = DH_key_exchange(*server_sock);

    while(1) {
        while((bytes_received = read(http_sock, buffer, MAXBUF)) > 0) {
            buffer[bytes_received] = '\0';
            printf("Received data from client: %s\n", buffer);
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
    }
}

void printBanner() {
	int _fd;
	char buf[MAXBUF];
	ssize_t bytes;
	_fd = open("banner.txt", O_RDONLY);
	if(_fd < 0) {
		fprintf(stderr, "banner open failed.\n");
		return;
	}
	while((bytes = read(_fd, buf, MAXBUF)) > 0) {
		buf[bytes] = '\0';
		write(1, buf, bytes);
		break;
	}
	close(_fd);
	return;
}

void sigint_handler(int sig_num) {
    close_all_fd();
	fprintf(stderr, "\nProgram terminated.\n");
	exit(0);
}

static void __print_prompt(void)
{
	char *prompt = "Insert IP address:";
	fprintf(stderr, "%s ", prompt);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    char address[MAX_COMMAND_LEN] = { '\0' };

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
        __print_prompt();

        if (!fgets(address, sizeof(address), stdin)) return 0;

		if (!strcmp(address, "q\n") || !strcmp(address, "exit\n")) {
            fprintf(stderr, "Exit program..\n");
            return 0;
        }
        else {
            char *pos;
            if ((pos = strchr(address, '\n')) != NULL) *pos = '\0';
        }
        local_init(&sockfd, address);
        port_forward(&sockfd);

    }
    else if (mode == REMOTE_MODE) {
        remote_init(&sockfd, &server_addr);
        remote_accept(&sockfd, &server_addr);
        port_forward(&sockfd);
    }

    return 0;
}
