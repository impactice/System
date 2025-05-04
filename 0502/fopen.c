#include <stdio.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>  

int main(int argc, char *argv[])
{
    int fd1, fd2, fd3;

    if (argc < 4) {
        fprintf(stderr, "사용법: %s 파일1 파일2 파일3\n", argv[0]);
        exit(1);
    }

    if ((fd1 = open(argv[1], O_RDWR)) == -1)
        perror("파일1 열기 실패");
    else
        printf("파일 %s 열기 성공: %d\n", argv[1], fd1);

    if ((fd2 = open(argv[2], O_RDWR)) == -1)
        perror("파일2 열기 실패");
    else
        printf("파일 %s 열기 성공: %d\n", argv[2], fd2);

    if ((fd3 = open(argv[3], O_RDWR)) == -1)
        perror("파일3 열기 실패");
    else
        printf("파일 %s 열기 성공: %d\n", argv[3], fd3);

    // 성공한 파일들만 닫기
    if (fd1 != -1) close(fd1);
    if (fd2 != -1) close(fd2);
    if (fd3 != -1) close(fd3);

    return 0;
}
