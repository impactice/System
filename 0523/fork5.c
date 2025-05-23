#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    int pid1, pid2, child, status;

    printf("[%d] 부모 프로세스 시작\n", getpid());

    /* 첫 번째 자식 프로세스 생성 */
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork 오류");
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0) {
        printf("[%d] 자식 프로세스[1] 시작\n", getpid());
        sleep(1);
        printf("[%d] 자식 프로세스[1] 종료\n", getpid());
        exit(1);
    }
    
    /* 두 번째 자식 프로세스 생성 */
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork 오류");
        exit(EXIT_FAILURE);
    }
    if (pid2 == 0) {
        printf("[%d] 자식 프로세스 #2 시작\n", getpid());
        sleep(2);
        printf("[%d] 자식 프로세스 #2 종료\n", getpid());
        exit(2);
    }

    /* 자식 프로세스 [1]의 종료를 기다림 */
    child = waitpid(pid1, &status, 0);
    if (child < 0) {
        perror("waitpid 오류");
        exit(EXIT_FAILURE);
    }
    printf("[%d] 자식 프로세스 #1 (%d) 종료\n", getpid(), child);
    printf("\t종료 코드: %d\n", status >> 8);

    return EXIT_SUCCESS;
}