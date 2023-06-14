# System Software Security

## 201820671 이현우

### Final Project

개발환경 : Linux(Debian)

#### 구성파일

- README.md
- Makefile
- server.c
- client.c
- serverbanner.txt
- clientbanner.txt

#### 기본 설정
1. http 서버를 킨다.
- sudo systemctl start apache2
2. 80번 포트로 들어오는 접근을 차단하는 inbound 규칙을 방화벽에 추가한다.
- sudo iptables -A INPUT -p tcp --dport 80 -j DROP
- sudo iptables -I INPUT -p tcp -s 127.0.0.1 --dport 80 -j ACCEPT
3. /var/www/html 내부에 index.html을 제외한 다른 파일을 추가한다. (이하 example.html)

##### 참고
다음 세 명령어를 입력하거나 재부팅하면 설정을 원래대로 돌릴 수 있다.
- sudo service apache2 stop
- sudo iptables -D INPUT -p tcp --dport 80 -j DROP
- sudo iptables -D INPUT -p tcp -s 127.0.0.1 --dport 80 -j ACCEPT
---
#### 구현과정 (local port forwarding mode)
1. make 명령어로 server와 client 파일 생성
2. ./server -l
3. 다른 콘솔 창에서 ./client -l 실행 후 server의 IP주소 입력(localhost X)
4. 프로그램을 유지한 상태로 웹브라우저 실행
5. serverIP:80/example.html 접근 시도(실패)
6. localhost:8080/example.html 접근 시도(서버의 웹서비스 접근 성공)

#### 구현과정 (remote port forwarding mode)
1. make 명령어로 server와 client 파일 생성
2. ./client -r
3. 다른 콘솔 창에서 ./server -r 실행 후 client의 IP주소 입력(localhost X)
4. 프로그램을 유지한 상태로 웹브라우저 실행
5. serverIP:80/example.html 접근 시도(실패)
6. localhost:8080/example.html 접근 시도(서버의 웹서비스 접근 성공)
