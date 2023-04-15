# System Software Security

## 201820671 이현우

### Project 1

개발환경 : Linux

#### 구성파일

- Readme.md
- (Makefile)
- input (입력파일)
- printer.c
- main.c
- main (실행파일)
- printer (실행파일)
---
#### 구현과정(별개 컴파일X)

1. main 실행
2. 실행결과로 result 파일 생성
---
#### 구현과정(별개 컴파일O, use Makefile)

1. make clean (실행파일과 result 파일이 삭제됨)
2. make (c파일을 각각 컴파일하여 main파일과 printer파일 생성)
3. main 실행
4. 실행결과로 result 파일 생성
---
#### 구현과정(별개 컴파일O, not use Makefile)
***파일명 중요***
1. printer.c 파일을 컴파일하여 printer 실행파일 생성 (cc -o printer printer.c)
2. main.c 파일을 컴파일하여 main 실행파일 생성 (cc -o main main.c)
3. main 실행
4. 실행결과로 result 파일 생성
