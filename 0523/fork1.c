#include <stdio.h>
#include <unistd.h>

/* 자식 프로세스를 생성하는 프로그램 */
int main() {
    int pid;

    // 현재 프로세스 ID 출력
    printf("[%d] 프로세스 시작\n", getpid());

    // 새로운 프로세스 생성
    pid = fork();

    // 부모 및 자식 프로세스에서 출력
    printf("[%d] 프로세스 : fork() 반환값 %d\n", getpid(), pid);
    printf("[%d] 프로세스 : fork() 반환값 %d\n", getpid(), pid); 
    printf("[%d] 프로세스 : fork() 반환값 %d\n", getpid(), pid);
    
    return 0;
}