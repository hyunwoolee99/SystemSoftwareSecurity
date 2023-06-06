#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libssh/libssh.h>

int main() {
    ssh_session session;
    int rc;

    // SSH 세션 생성
    session = ssh_new();
    if (session == NULL) {
        fprintf(stderr, "Failed to create SSH session\n");
        exit(EXIT_FAILURE);
    }

    // SSH 연결 설정
    ssh_options_set(session, SSH_OPTIONS_HOST, "원격서버주소");
    ssh_options_set(session, SSH_OPTIONS_USER, "사용자이름");

    // SSH 연결
    rc = ssh_connect(session);
    if (rc != SSH_OK) {
        fprintf(stderr, "Failed to connect to remote server: %s\n", ssh_get_error(session));
        exit(EXIT_FAILURE);
    }

    // SSH 인증
    rc = ssh_userauth_password(session, NULL, "비밀번호");
    if (rc != SSH_AUTH_SUCCESS) {
        fprintf(stderr, "Failed to authenticate: %s\n", ssh_get_error(session));
        exit(EXIT_FAILURE);
    }

    // SSH 명령 실행
    ssh_channel channel;
    char buffer[1024];
    int nbytes;

    channel = ssh_channel_new(session);
    if (channel == NULL) {
        fprintf(stderr, "Failed to create SSH channel\n");
        exit(EXIT_FAILURE);
    }

    rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK) {
        fprintf(stderr, "Failed to open SSH channel\n");
        exit(EXIT_FAILURE);
    }

    rc = ssh_channel_request_exec(channel, "ls");
    if (rc != SSH_OK) {
        fprintf(stderr, "Failed to execute SSH command\n");
        exit(EXIT_FAILURE);
    }

    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    while (nbytes > 0) {
        if (fwrite(buffer, 1, nbytes, stdout) != nbytes) {
            fprintf(stderr, "Failed to write to stdout\n");
            exit(EXIT_FAILURE);
        }
        nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    }

    if (nbytes < 0) {
        fprintf(stderr, "Error reading SSH channel: %s\n", ssh_get_error(session));
        exit(EXIT_FAILURE);
    }

    // SSH 연결 종료
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(session);
    ssh_free(session);

    return 0;
}
