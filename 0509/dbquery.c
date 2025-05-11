#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "student.h"

/* 학번을 입력받아 해당 학생의 레코드를 파일에서 읽어 출력한다. */
int main(int argc, char *argv[])
{
    int fd, id; 
    char c;
    struct student record;

    // 인자 검사
    if (argc < 2) {
        fprintf(stderr, "사용법 : %s file\n", argv[0]);
        exit(1);
    }

    // 파일 열기 (읽기 전용)
    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        perror(argv[1]);
        exit(2);
    } 

    do {
        printf("\n검색할 학생의 학번 입력: ");
        if (scanf("%d", &id) == 1) {
            // 해당 학번의 레코드 위치로 이동
            lseek(fd, (id - START_ID) * sizeof(record), SEEK_SET);

            // 레코드 읽기 및 유효성 확인
            if ((read(fd, (char *)&record, sizeof(record)) > 0) && (record.id != 0))
                printf("이름:%s\t 학번:%d\t 점수:%d\n", record.name, record.id, record.score);
            else
                printf("레코드 %d 없음\n", id);
        } else {
            printf("입력 오류\n");
        }

        // 계속 여부 확인
        printf("계속하겠습니까?(Y/N) ");
        scanf(" %c", &c);  // 공백을 줘야 버퍼 개행 제거됨

    } while (c == 'Y');

    close(fd);
    exit(0);
}
