#include <stdio.h>
#include <stdlib.h>
#include <libssh/libssh.h>

int main() {
    ssh_bind sshbind;
    ssh_session session;

    // SSH 바인딩 생성
    sshbind = ssh_bind_new();
    if (sshbind == NULL) {
        fprintf(stderr, "Failed to create SSH binding\n");
        exit(EXIT_FAILURE);
    }

    // SSH 바인딩 포트 설정
    int port = 22;
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT_STR, "22");

    // SSH 바인딩 수신 대기
    int rc = ssh_bind_listen(sshbind);
    if (rc != SSH_OK) {
        fprintf(stderr, "Failed to listen for SSH connections\n");
        exit(EXIT_FAILURE);
    }

    // SSH 연결 대기
    session = ssh_new();
    if (session == NULL) {
        fprintf(stderr, "Failed to create SSH session\n");
        exit(EXIT_FAILURE);
    }

    rc = ssh_bind_accept(sshbind, session);
    if (rc != SSH_OK) {
        fprintf(stderr, "Failed to accept SSH connection\n");
        exit(EXIT_FAILURE);
    }

    // SSH 인증
    rc = ssh_handle_key_exchange(session);
    if (rc != SSH_OK) {
        fprintf(stderr, "Failed to perform SSH key exchange\n");
        exit(EXIT_FAILURE);
    }

    // SSH 채널 생성
    ssh_channel channel;
    channel = ssh_channel_new(session);
    if (channel == NULL) {
        fprintf(stderr, "Failed to create SSH channel\n");
        exit(EXIT_FAILURE);
    }

    // SSH 채널 열기
    rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK) {
        fprintf(stderr, "Failed to open SSH channel\n");
        exit(EXIT_FAILURE);
    }

    // SSH 명령 수신 및 실행
    char buffer[1024];
    int nbytes;

    while (1) {
        nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
        if (nbytes > 0) {
            // 수신한 명령을 처리하는 로직을 작성합니다.
            // 여기서는 수신한 명령을 그대로 실행하여 결과를 클라이언트에게 반환합니다.
            rc = ssh_channel_write(channel, buffer, nbytes);
            if (rc < 0) {
                fprintf(stderr, "Failed to write SSH channel\n");
                exit(EXIT_FAILURE);
            }
        } else if (nbytes < 0) {
            fprintf(stderr, "Error reading SSH channel\n");
            exit(EXIT_FAILURE);
        } else {
            break;  // 클라이언트 연결 종료
        }
    }

    // SSH 연결 종료
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(session);
    ssh_free(session);
    ssh_bind_free(sshbind);

    return 0;
}
