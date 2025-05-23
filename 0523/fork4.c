#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid, child;
    int status;

    printf("[%d] 부모 프로세스 시작\n", getpid());

    // 자식 프로세스 생성
    pid = fork();
    if (pid < 0) {
        perror("fork 실패");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        // 자식 프로세스 코드 실행
        printf("[%d] 자식 프로세스 시작\n", getpid());
        exit(1);
    }

    // 부모 프로세스: 자식의 종료를 기다림
    child = wait(&status);
    if (child == -1) {
        perror("wait 실패");
        exit(EXIT_FAILURE);
    }

    printf("[%d] 자식 프로세스 %d 종료\n", getpid(), child);
    printf("\t종료 코드 %d\n", status >> 8);  // 또는 WEXITSTATUS(status) 사용 가능

    return EXIT_SUCCESS;
}