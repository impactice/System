#include <stdio.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>  // 🔧 close() 함수 사용을 위한 헤더

int main(int argc, char *argv[])
{
    int fd;

    if (argc < 2) {
        fprintf(stderr, "사용법: %s 파일이름\n", argv[0]);
        exit(1);
    }

    if ((fd = open(argv[1], O_RDWR)) == -1) {
        perror("파일 열기 오류");
    } else {
        printf("파일 %s 열기 성공 : %d\n", argv[1], fd);
        close(fd);  // ✅ 성공했을 때만 close
    }

    exit(0);
}
