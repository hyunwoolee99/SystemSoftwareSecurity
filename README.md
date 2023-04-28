# System Software Security

## 201820671 이현우

### Project 1

개발환경 : Linux

#### 구성파일

- Readme.md
- Makefile
  + 빠른 구현 및 실행을 위한 파일
- input (입력파일)
- result (출력파일)
  + 제출된 파일에서는 input 파일과 내용이 동일
  + make clean 시에 result 파일도 삭제
  + main 실행 시 새로 생성, result 파일이 이미 존재할 경우 기존 내용을 덮어씀
- printer.c
  + 프로그램 P에 해당하는 코드
- main.c
  + 메인 프로그램 코드
- printer (실행파일)
  + 프로그램 P에 해당
- main (실행파일)
  + 메인 프로그램

##### 참고사항

printer 프로그램을 단독으로 실행할 경우 EOF command(ctrl + D) 를 통해 종료한다

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
