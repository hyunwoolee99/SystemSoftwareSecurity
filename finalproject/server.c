#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/aes.h>
#include <openssl/rand.h>

#define SERVER_PORT 12345
#define CLIENT_PORT 8080
#define MAXBUF 4096
#define MAX_COMMAND_LEN 256
#define usage() fprintf(stderr, "Usage: %s {-l, -r}\n", argv[0]);

pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    pthread_mutex_lock(&fd_mutex);
	fd_array[fd_count] = fd;
	fd_count++;
    pthread_mutex_unlock(&fd_mutex);
	return;
}

//fd_array에서 해당하는 (socket) file descriptor를 close, 이후 array에서 삭제
void close_fd(int fd) {
    pthread_mutex_lock(&fd_mutex);
    for(int i = 0; i < fd_count; i++) {
        if(fd_array[i] == fd) {
            close(fd);
            fd_array[i] = fd_array[--fd_count];
            return;
        }
    pthread_mutex_unlock(&fd_mutex);
    }
}

void close_all_fd() {
	for(int i = 0; i < fd_count; i++) {
		close(fd_array[i]);
	}
	return;
}

void encrypt(const unsigned char *plaintext, int plaintext_len, const unsigned char *key, unsigned char *ciphertext) {
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 128, &aes_key);
    AES_encrypt(plaintext, ciphertext, &aes_key);
}

void decrypt(const unsigned char *ciphertext, int ciphertext_len, const unsigned char *key, unsigned char *plaintext) {
    AES_KEY aes_key;
    AES_set_decrypt_key(key, 128, &aes_key);
    AES_decrypt(ciphertext, plaintext, &aes_key);
}

unsigned char *DH_key_exchange(int *sock) {
    DH *dh_params = DH_new();
    DH_generate_parameters_ex(dh_params, 2048, DH_GENERATOR_5, NULL);
    DH *server_dh = DHparams_dup(dh_params);
    if (DH_generate_key(server_dh) != 1) {
        perror("DH error");
            exit(EXIT_FAILURE);
    }

    int pub_key_size = BN_num_bytes(server_dh->pub_key);
    unsigned char *pub_key_serialized = malloc(pub_key_size);
    BN_bn2bin(server_dh->pub_key, pub_key_serialized);

    if (send(*sock, &pub_key_size, sizeof(pub_key_size), 0) == -1 ||
        send(*sock, pub_key_serialized, pub_key_size, 0) == -1) {
            perror("DH error");
            exit(EXIT_FAILURE);
    }

    if (recv(*sock, &pub_key_size, sizeof(pub_key_size), 0) == -1) {
        perror("DH error");
            exit(EXIT_FAILURE);
    }
    unsigned char *client_pub_key_serialized = malloc(pub_key_size);
    if (recv(*sock, client_pub_key_serialized, pub_key_size, 0) == -1) {
        perror("DH error");
            exit(EXIT_FAILURE);
    }

    BIGNUM *client_pub_key = BN_new();
    BN_bin2bn(client_pub_key_serialized, pub_key_size, client_pub_key);

    unsigned char *shared_key = malloc(DH_size(server_dh));
    DH_compute_key(shared_key, client_pub_key, server_dh);

    // Don't forget to free the allocated memory
    free(pub_key_serialized);
    free(client_pub_key_serialized);
    BN_free(client_pub_key);
    DH_free(server_dh);
    DH_free(dh_params);

    return shared_key;
}

void append_history(struct sockaddr_in *address) {
    int fd = open("connection_history.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Unable to open file");
        return;
    }
    //fd에 ip 주소와 포트번호를 "xxx.xxx.xxx.xxx:x" 형태로 저장
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address->sin_addr), ip, INET_ADDRSTRLEN);
    int port = ntohs(address->sin_port);

    char buffer[INET_ADDRSTRLEN + 10];
    snprintf(buffer, sizeof(buffer), "%s:%d\n", ip, port);

    // Write to the file
    if (write(fd, buffer, strlen(buffer)) == -1) {
        perror("Error writing to file");
    }

    close(fd);
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
    append_history(client_addr);

    return EXIT_SUCCESS;
}

int remote_init(int *sockfd, struct sockaddr_in *client_addr, char *client_ip) {
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
    client_addr->sin_family = AF_INET;
    client_addr->sin_port = htons(CLIENT_PORT);
    if (inet_pton(AF_INET, client_ip, &(client_addr->sin_addr)) <= 0) {
        perror("Invalid server IP address");
        close_fd(*sockfd);
        return EXIT_FAILURE;
    }

    // 서버에 연결
    if(connect(*sockfd, (struct sockaddr *)client_addr, sizeof(*client_addr)) == -1) {
        perror("Error connecting to server");
        close_fd(*sockfd);
        return EXIT_FAILURE;
    }

    printf("connected\n");
    append_history(client_addr);
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
	fprintf(stderr, "\nProgram Terminated.\n");
	exit(0);
}

static void __print_prompt(void)
{
	char *prompt = "Insert IP address:";
	fprintf(stderr, "%s ", prompt);
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
    char plaintext[MAXBUF];
	ssize_t bytes_received;
    unsigned char shared_key[128];
    
    shared_key = DH_key_exchange(*sock);

    while(1) {
	    while ((bytes_received = recv(sock, buffer, MAXBUF, 0)) > 0) {
		    buffer[bytes_received] = '\0';
    		printf("Received data from client: %s\n", buffer);
            decrypt(buffer, bytes_received, shared_key, plaintext);
	    	if(send(http, plaintext, bytes_received, 0) < 0)
    		{
    			fprintf(stderr, "Send failed");
    		}
    	}

	    while ((bytes_received = read(http, plaintext, MAXBUF)) > 0) {
	    	plaintext[bytes_received] = '\0';
	    	printf("Received data from http: %s\n", plaintext);
            encrypt(plaintext, bytes_received, shared_key, buffer);
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
        while(1){
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
        while (1) {
		    __print_prompt();

		    if (!fgets(command, sizeof(command), stdin)) break;

		    if (!strcmp(command, "q\n") || !strcmp(command, "exit\n")) {
                fprintf(stderr, "Exit program..\n");
                break;
            }
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