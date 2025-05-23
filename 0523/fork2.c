#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>  // fork()와 getpid()를 사용하기 위한 헤더 추가

/* 부모 프로세스가 자식 프로세스를 생성하고 서로 다른 메시지를 출력하는 프로그램 */
int main() {
    int pid;

    // 프로세스 생성
    pid = fork();

    if (pid == 0) { // 자식 프로세스
        printf("[Child] : Hello, world! PID = %d\n", getpid());
    } else { // 부모 프로세스
        printf("[Parent] : Hello, world! PID = %d\n", getpid());
    }

    return 0;
}