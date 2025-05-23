#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 부모 프로세스가 두 개의 자식 프로세스를 생성한다.
int main() {
    int pid1, pid2;

    // 첫 번째 fork: 자식 프로세스 1 생성
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0) {
        printf("[Child 1] : Hello, world! pid = %d\n", getpid());
        exit(EXIT_SUCCESS);
    }

    // 두 번째 fork: 자식 프로세스 2 생성
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid2 == 0) {
        printf("[Child 2] : Hello, world! pid = %d\n", getpid());
        exit(EXIT_SUCCESS);
    }

    // 부모 프로세스 실행
    printf("[PARENT] : Hello, world! pid = %d\n", getpid());

    return 0;
}