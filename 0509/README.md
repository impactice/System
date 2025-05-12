## 파일 디스크립터 복제
![image](https://github.com/user-attachments/assets/4150d590-3515-41d7-b044-d5be6d0c8177)

## dup.c 
```
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
int main()
{
  int fd1, fd2;

  if((fd1 = creat("myfile", 0600)) == -1)
    perror("myfile");
  write(fd1, "Hello! Linux", 12);
  fd2 = dup(fd1);
  write(fd2, "Bye! Linux", 10);
  exit(0);
}

```
```
$ dup
```
```
$ cat myfile
```
Hello! LinuxBye! Linux

## 파일 위치 포인터(file position pointer)
- 파일 위치 포인터는 파일 내에 읽거나 쓸 위치인 현재 파일 위치(current file position)를 가리킨다
![image](https://github.com/user-attachments/assets/a2492b79-46aa-46e3-94e5-2ca729cc6733)

## 파일 위치 포인터 이동: lseek()
- lseek() 시스템 호출
  - 임의의 위치로 파일 위치 포인터를 이동시킬 수 있다

```
#include <unistd.h>
off_t lseek (int fd, off_t offset, int whence );
```
이동에 성공하면 현재 위치를 리턴하고 실패하면 -1을 리턴한다

![image](https://github.com/user-attachments/assets/341dafd6-2bd8-4abc-a19a-eb9648475c29)

## 파일 위치 포인터 이동: 예
- 파일 위치 이동
  - lseek(fd, 0L, SEEK_SET); 파일 시작으로 이동(rewind)
  - lseek(fd, 100L, SEEK_SET); 파일 시작에서 100바이트 위치로
  - lseek(fd, 0L, SEEK_END); 파일 끝으로 이동(append)

- 레코드 단위로 이동
  - lseek(fd, n * sizeof(record), SEEK_SET); n+1번째 레코드 시작위치로
  - lseek(fd, sizeof(record), SEEK_CUR); 다음 레코드 시작위치로
  - lseek(fd, -sizeof(record), SEEK_CUR); 전 레코드 시작위치로 .

- 파일끝 이후로 이동
  - lseek(fd, sizeof(record), SEEK_END); 파일끝에서 한 레코드 다음 위치로

## 레코드 저장 예
- write(fd, &record1, sizeof(record));
- write(fd, &record2, sizeof(record));
- lseek(fd, sizeof(record), SEEK_END);
- write(fd, &record3, sizeof(record));


![image](https://github.com/user-attachments/assets/69525dee-4da4-41f1-b38f-264775f17f11)

## student.h
```
#define MAX 24
#define START_ID 1401001
struct student {
  char name[MAX];
  int id;
  int score;
};
```

## dbcreate.c
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "student.h"
/* 학생 정보를 입력받아 데이터베이스 파일에 저장한다. */
int main(int argc, char *argv[])
{
  int fd;
  struct student record;
  if (argc < 2) {
    fprintf(stderr, "사용법 : %s file\n", argv[0]);
    exit(1);
  }
  if ((fd = open(argv[1], O_WRONLY|O_CREAT|O_EXCL, 0640)) == -1) {perror(argv[1]);
    exit(2);
  }
  printf("%-9s %-8s %-4s\n", "학번", "이름", "점수");
  while (scanf("%d %s %d", &record.id, record.name, &record.score) == 3) {
    lseek(fd, (record.id - START_ID) * sizeof(record), SEEK_SET);
    write(fd, (char *) &record, sizeof(record) );
  }
  close(fd);
  exit(0);
}
```

## dbquery.c
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "student.h"
/* 학번을 입력받아 해당 학생의 레코드를 파일에서 읽어 출력한다. */
int main(int argc, char *argv[])
{
  int fd, id;
  struct student record;
  if (argc < 2) {
    fprintf(stderr, "사용법 : %s file\n", argv[0]);
    exit(1);
  }
  if ((fd = open(argv[1], O_RDONLY)) == -1) {perror(argv[1]);
    exit(2);
  }
  do {
    printf("\n검색할 학생의 학번 입력:");
    if (scanf("%d", &id) == 1) {
      lseek(fd, (id-START_ID)*sizeof(record), SEEK_SET);
      if ((read(fd, (char *) &record, sizeof(record)) > 0) && (record.id != 0))
        printf("이름:%s\t 학번:%d\t 점수:%d\n", record.name, record.id, record.score);
      else printf("레코드 %d 없음\n", id);
    } else printf(“입력 오류”);
    printf("계속하겠습니까?(Y/N)");
    scanf(" %c", &c);
  } while (c=='Y');
  close(fd);
  exit(0);
}
```

## 레코드 수정 과정
(1) 파일로부터 해당 레코드를 읽어서
(2) 이 레코드를 수정한 후에
(3) 수정된 레코드를 다시 파일 내의 원래 위치에 써야 한다 

![image](https://github.com/user-attachments/assets/f12e58f6-e276-4b98-beca-6b12adc850b8)

## dbupdate.c 
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "student.h"
/* 학번을 입력받아 해당 학생 레코드를 수정한다. */
int main(int argc, char *argv[])
{
  int fd, id;
  char c;
  struct student record;
  if (argc < 2) {
    fprintf(stderr, "사용법 : %s file\n", argv[0]);
    exit(1);
  }
  if ((fd = open(argv[1], O_RDWR)) == -1) {
    perror(argv[1]);
    exit(2);
  }
  do {
    printf("수정할 학생의 학번 입력: ");
    if (scanf("%d", &id) == 1) {
      lseek(fd, (long) (id-START_ID)*sizeof(record), SEEK_SET);
      if ((read(fd, (char *) &record, sizeof(record)) > 0) && (record.id != 0)) {
        printf("학번:%8d\t 이름:%4s\t 점수:%4d\n",
          record.id, record.name, record.score);
        printf("새로운 점수: ");
        scanf("%d", &record.score);
        lseek(fd, (long) -sizeof(record), SEEK_CUR);
        write(fd, (char *) &record, sizeof(record));
      } else printf("레코드 %d 없음\n", id);
    } else printf("입력오류\n");
    printf("계속하겠습니까?(Y/N)");
    scanf(" %c",&c);
  } while (c == 'Y');
  close(fd);
  exit(0);
}
```

## 핵심 개념
- 시스템 호출은 커널에 서비스를 요청하기 위한 프로그래밍 인터페이스로 응용 프로그램은 시스템 호출을 통해서 커널에 서비스를 요청할 수 있다
- 파일 디스크립터는 열린 파일을 나타낸다
- open() 시스템 호출을 파일을 열고 열린 파일의 파일 디스크립터를 반환한다
- read() 시스템 호출은 지정된 파일에서 원하는 만큼의 데이터를 읽고 write() 시스템 호출은 지정된 파일에 원하는 만큼의 데이터를 쓴다
- 파일 위치 포인터는 파일 내에 읽거나 쓸 위치인 현재 파일 위치를 가리킨다
- lseek() 시스템 호출은 지정된 파일의 현재 파일 위치를 원하는 위치로 이동시킨다

## 컴퓨터 시스템 구조
- 유닉스 커널(kernel)
  - 하드웨어를 운영 관리하여 다음과 같은 서비스를 제공
  - 파일 관리(File management)
  - 프로세스 관리(Process management)
  - 메모리 관리(Memory management)
  - 통신 관리(Communication management)
  - 주변장치 관리(Device management)

![image](https://github.com/user-attachments/assets/ffe37c0a-a1dc-4f3d-82a5-456deb05b830)

## 시스템 호출 
- 시스템 호출은 커널에 서비스 요청을 위한 프로그래밍 인터페이스 응용 프로그램은 시스템 호출을 통해서 커널에 서비스를 요청한다

![image](https://github.com/user-attachments/assets/7161eda4-56e0-4da6-90a2-148bc2eb8761)

## 시스템 호출 과정
![image](https://github.com/user-attachments/assets/83248b7a-658c-48e2-b82e-f820545ddee3)

## 시스템 호출 요약
![image](https://github.com/user-attachments/assets/03447e2e-de51-4775-adce-821b76b51f79)

## 유닉스에서 파일
- 연속된 바이트의 나열
- 특별한 다른 포맷을 정하지 않음
- 디스크 파일뿐만 아니라 외부 장치에 대한 인터페이스

![image](https://github.com/user-attachments/assets/4115ee1d-e5fa-4de4-94dc-f872cb9cffec)

## 파일 열기: open()
- 파일을 사용하기 위해서는 먼저 open() 시스템 호출을 이용하여 파일을 열어야 한다

```
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int open (const char *path, int oflag, [ mode_t mode ]);
```
파일 열기에 성공하면 파일 디스크립터를, 실패하면 -1을 리턴
- 파일 디스크립터는 열린 파일을 나타내는 번호이다

- oflag
  - O_RDONLY
      읽기 모드, read() 호출은 사용 가능
  - O_WRONLY
      쓰기 모드, write() 호출은 사용 가능
  - O_RDWR
      읽기/쓰기 모드, read(), write() 호출 사용 가능
  - O_APPEND
      데이터를 쓰면 파일끝에 첨부된다.
  - O_CREAT
      해당 파일이 없는 경우에 생성하며 mode는 생성할 파일의 사용권한을 나타낸다
  - O_TRUNC
      파일이 이미 있는 경우 내용을 지운다
  - O_EXCL
      O_CREAT와 함께 사용되며 해당 파일이 이미 있으면 오류
  - O_NONBLOCK
      넌블로킹 모드로 입출력 하도록 한다
  - O_SYNC
      write() 시스템 호출을 하면 디스크에 물리적으로 쓴 후 반환된다

## 파일 열기 예 
- fd = open("account",O_RDONLY);
- fd = open(argv[1], O_RDWR);
- fd = open(argv[1], O_RDWR | O_CREAT, 0600);
- fd = open("tmpfile", O_WRONLY|O_CREAT|O_TRUNC, 0600);
- fd = open("/sys/log", O_WRONLY|O_APPEND|O_CREAT, 0600);
- if ((fd = open("tmpfile", O_WRONLY|O_CREAT|O_EXCL, 0666))==-1)

## fopen.c 
```
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(int argc, char *argv[])
{
  int fd;
  if ((fd = open(argv[1], O_RDWR)) == -1)
    printf("파일 열기 오류\n");
  else printf("파일 %s 열기 성공 : %d\n", argv[1], fd);

  close(fd);
  exit(0);
} 
```

## 파일 생성: creat()
- creat() 시스템 호출
  - path가 나타내는 파일을 생성하고 쓰기 전용으로 연다
  - 생성된 파일의 사용권한은 mode로 정한다
  - 기존 파일이 있는 경우에는 그 내용을 삭제하고 연다
  - 다음 시스템 호출과 동일 
  ```
  open(path, WRONLY | O_CREAT | O_TRUNC, mode);
  ```

```
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int creat (const char *path, mode_t mode );
```
파일 생성에 성공하면 파일 디스크립터를, 실패하면 -1을 리턴

## 파일 닫기: close() 
- close() 시스템 호출은 fd가 나타내는 파일을 닫는다
```
#include <unistd.h>
int close( int fd );
```
fd가 나타내는 파일을 닫는다 
성공하면 0, 실패하면 -1을 리턴한다

## 데이터 읽기: read()
- read() 시스템 호출
  - fd가 나타내는 파일에서
  - nbytes 만큼의 데이터를 읽고
  - 읽은 데이터는 buf에 저장한다
```
#include <unistd.h>
ssize_t read ( int fd, void *buf, size_t nbytes );
```
파일 읽기에 성공하면 읽은 바이트 수, 파일 끝을 만나면 0, 실패하면 -1을 리턴

## fsize.c
```
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#define BUFSIZE 512
/* 파일 크기를 계산 한다 */
int main(int argc, char *argv[])
{
  char buffer[BUFSIZE];
  int fd;
  ssize_t nread;
  long total = 0;
  if ((fd = open(argv[1], O_RDONLY)) == -1)
    perror(argv[1]);
/* 파일의 끝에 도달할 때까지 반복해서 읽으면서 파일 크기 계산 */
  while( (nread = read(fd, buffer, BUFSIZE)) > 0)
  total += nread;
  close(fd);
  printf ("%s 파일 크기 : %ld 바이트 \n", argv[1], total);
  exit(0);
}
```

## 데이터 쓰기: write() 
- write() 시스템 호출
  - buf에 있는 nbytes 만큼의 데이터를 fd가 나타내는 파일에 쓴다
```
#include <unistd.h>
ssize_t write (int fd, void *buf, size_t nbytes);

```
파일에 쓰기를 성공하면 실제 쓰여진 바이트 수를 리턴하고, 실패하면 -1을 리턴

## copy.c
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
/* 파일 복사 프로그램 */
main(int argc, char *argv[])
{
  int fd1, fd2, n;
  char buf[BUFSIZ];
  if (argc != 3) {
    fprintf(stderr,"사용법: %s file1 file2\n", argv[0]);
    exit(1);
  }
  if ((fd1 = open(argv[1], O_RDONLY)) == -
    1) {
    perror(argv[1]);
    exit(2);
  }
  if ((fd2 =open(argv[2], O_WRONLY |
    O_CREAT|O_TRUNC 0644)) == -1) {
    perror(argv[2]);
    exit(3);
  }

  while ((n = read(fd1, buf, BUFSIZ)) > 0)
    write(fd2, buf, n); // 읽은 내용을 쓴다.
  exit(0);
}
```

## 파일 디스크립터 복제 
- dup()/dup2() 호출은 기존의 파일 디스크립터를 복제한다
```
#include <unistd.h>
int dup(int oldfd);
```
oldfd에 대한 복제본인 새로운 파일 디스크립터를 생성하여 반환한다
실패하면 –1을 반환한다
```
int dup2(int oldfd, int newfd);
```
oldfd을 newfd에 복제하고 복제된 새로운 파일 디스크립터를 반환한다
실패하면 –1을 반환한다 

- oldfd와 복제된 새로운 디스크립터는 하나의 파일을 공유한다

![image](https://github.com/user-attachments/assets/2a6489a3-4622-402c-ac9e-ea4c6a3257da)

## dup.c
```
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
int main()
{
  int fd1, fd2;

  if((fd1 = creat("myfile", 0600)) == -1)
    perror("myfile");
  write(fd1, "Hello! Linux", 12);
  fd2 = dup(fd1);
  write(fd2, "Bye! Linux", 10);
  exit(0);
}
```

```
$ dup
$ cat myfile
```
Hello! LinuxBye! Linux

## 파일 위치 포인터(file position pointer)
- 파일 위치 포인터는 파일 내에 읽거나 쓸 위치인 현재 파일 위치(current file position)를 가리킨다

![image](https://github.com/user-attachments/assets/5b70e056-6afb-446d-8c8e-67946bd24ef3)

## 파일 위치 포인터 이동: lseek()
- lseek() 시스템 호출
  - 임의의 위치로 파일 위치 포인터를 이동시킬 수 있다
```
#include <unistd.h>
off_t lseek (int fd, off_t offset, int whence );
```
이동에 성공하면 현재 위치를 리턴하고 실패하면 -1을 리턴한다 

![image](https://github.com/user-attachments/assets/3f19af86-7d91-45ff-af22-d87dd84f0080)

## 파일 위치 포인터 이동: 예 
- 파일 위치 이동
  - lseek(fd, 0L, SEEK_SET); 파일 시작으로 이동(rewind)
  - lseek(fd, 100L, SEEK_SET); 파일 시작에서 100바이트 위치로
  - lseek(fd, 0L, SEEK_END); 파일 끝으로 이동(append)

- 레코드 단위로 이동
  - lseek(fd, n * sizeof(record), SEEK_SET); n+1번째 레코드 시작위치로
  - lseek(fd, sizeof(record), SEEK_CUR); 다음 레코드 시작위치로
  - lseek(fd, -sizeof(record), SEEK_CUR); 전 레코드 시작위치로 .

- 파일끝 이후로 이동
  - lseek(fd, sizeof(record), SEEK_END); 파일끝에서 한 레코드 다음 위치로

## 레코드 저장 예
write(fd, &record1, sizeof(record));
write(fd, &record2, sizeof(record));
lseek(fd, sizeof(record), SEEK_END);
write(fd, &record3, sizeof(record));

![image](https://github.com/user-attachments/assets/c5cb57d4-de4f-491c-b597-9904c0802da7)

## student.h
```
#define MAX 24
#define START_ID 1401001
struct student {
  char name[MAX];
  int id;
  int score;
};
```

## dbcreate.c 
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "student.h"
/* 학생 정보를 입력받아 데이터베이스 파일에 저장한다. */
int main(int argc, char *argv[])
{
  int fd;
  struct student record;
  if (argc < 2) {
    fprintf(stderr, "사용법 : %s file\n", argv[0]);
    exit(1);
  }
  if ((fd = open(argv[1], O_WRONLY|O_CREAT|O_EXCL, 0640)) == -1) {
    perror(argv[1]);
    exit(2);
  }
  printf("%-9s %-8s %-4s\n", "학번", "이름", "점수");
  while (scanf("%d %s %d", &record.id, record.name, &record.score) == 3) {
    lseek(fd, (record.id - START_ID) * sizeof(record), SEEK_SET);
    write(fd, (char *) &record, sizeof(record) );
  }
  close(fd);
  exit(0);
}
```

## dbquery.c 
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "student.h"
/* 학번을 입력받아 해당 학생의 레코드를 파일에서 읽어 출력한다. */
int main(int argc, char *argv[])
{
  int fd, id;
  struct student record;
  if (argc < 2) {
    fprintf(stderr, "사용법 : %s file\n", argv[0]);
    exit(1);
  }
  if ((fd = open(argv[1], O_RDONLY)) == -1) {
    perror(argv[1]);
    exit(2);
  }
  do {
    printf("\n검색할 학생의 학번 입력:");
    if (scanf("%d", &id) == 1) {
      lseek(fd, (id-START_ID)*sizeof(record), SEEK_SET);
      if ((read(fd, (char *) &record, sizeof(record)) > 0) && (record.id != 0))
        printf("이름:%s\t 학번:%d\t 점수:%d\n", record.name, record.id, record.score);
      else printf("레코드 %d 없음\n", id);
    } else printf(“입력 오류”);
    printf("계속하겠습니까?(Y/N)");
    scanf(" %c", &c);
  } while (c=='Y');
  close(fd);
  exit(0);
}
```

## 레코드 수정 과정
(1) 파일로부터 해당 레코드를 읽어서
(2) 이 레코드를 수정한 후에
(3) 수정된 레코드를 다시 파일 내의 원래 위치에 써야 한다

![image](https://github.com/user-attachments/assets/9d28655e-f136-4334-8853-4f94e47939a6)


## dbupdate.c
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "student.h"
/* 학번을 입력받아 해당 학생 레코드를 수정한다. */
int main(int argc, char *argv[])
{
  int fd, id;
  char c;
  struct student record;
  if (argc < 2) {
    fprintf(stderr, "사용법 : %s file\n", argv[0]);
    exit(1);
  }
  if ((fd = open(argv[1], O_RDWR)) == -1) {
    perror(argv[1]);
    exit(2);
  }
  do {
    printf("수정할 학생의 학번 입력: ");
    if (scanf("%d", &id) == 1) {
      lseek(fd, (long) (id-START_ID)*sizeof(record), SEEK_SET);
      if ((read(fd, (char *) &record, sizeof(record)) > 0) && (record.id != 0)) {
        printf("학번:%8d\t 이름:%4s\t 점수:%4d\n", record.id, record.name, record.score);
        printf("새로운 점수: ");
        scanf("%d", &record.score);
        lseek(fd, (long) -sizeof(record), SEEK_CUR);
        write(fd, (char *) &record, sizeof(record));
      } else printf("레코드 %d 없음\n", id);
    } else printf("입력오류\n");
    printf("계속하겠습니까?(Y/N)");
    scanf(" %c",&c);
  } while (c == 'Y');
  close(fd);
  exit(0);
}
```

## 핵심 개념 
- 시스템 호출은 커널에 서비스를 요청하기 위한 프로그래밍 인터페이스로 응용 프로그램은 시스템 호출을 통해서 커널에 서비스를 요청할 수 있다
- 파일 디스크립터는 열린 파일을 나타낸다
- open() 시스템 호출을 파일을 열고 열린 파일의 파일 디스크립터를 반환한다
- read() 시스템 호출은 지정된 파일에서 원하는 만큼의 데이터를 읽고 write() 시스템 호출은 지정된 파일에 원하는 만큼의 데이터를 쓴다
- 파일 위치 포인터는 파일 내에 읽거나 쓸 위치인 현재 파일 위치를 가리킨다
- lseek() 시스템 호출은 지정된 파일의 현재 파일 위치를 원하는 위치로 이동시킨다

## 파일 시스템 구조 
![image](https://github.com/user-attachments/assets/f85d6912-5146-49ed-a3c1-5c7293a31502)

- 부트 블록(Boot block)
  - 파일 시스템 시작부에 위치하고 보통 첫 번째 섹터를 차지
  - 부트스트랩 코드가 저장되는 블록

- 슈퍼 블록(Super block)
  - 전체 파일 시스템에 대한 정보를 저장
    - 총 블록 수, 사용 가능한 i-노드 개수, 사용 가능한 블록 비트 맵, 블록의 크기, 사용 중인 블록 수, 사용 가능한 블록 수 등

- i-리스트(i-list)
  - 각 파일을 나타내는 모든 i-노드들의 리스트
  - 한 블록은 약 40개 정도의 i-노드를 포함 

- 데이터 블록(Data block)
  - 파일의 내용(데이터)을 저장하기 위한 블록들

## i-노드(i-Node)
- 한 파일은 하나의 i-노드를 갖는다.

- 파일에 대한 모든 정보를 가지고 있음
  - 파일 타입: 일반 파일, 디렉터리, 블록 장치, 문자 장치 등
  - 파일 크기
  - 사용권한
  - 파일 소유자 및 그룹
  - 접근 및 갱신 시간
  - 데이터 블록에 대한 포인터(주소) 등

## 블록 포인터
![image](https://github.com/user-attachments/assets/1cc7cef0-76c9-42e0-a564-b4c460ad1a86)

- 데이터 블록에 대한 포인터
  - 파일의 내용을 저장하기 위해 할당된 데이터 블록의 주소

- 하나의 i-노드 내의 블록 포인터
  - 직접 블록 포인터 10개
  - 간접 블록 포인터 1개
  - 이중 간접 블록 포인터 1개

## 파일 입출력 구현
- 파일 입출력 구현을 위한 커널 내 자료구조
  - 파일 디스크립터 배열(Fd array)
  - 열린 파일 테이블(Open File Table)
  - 동적 i-노드 테이블(Active i-node table)

## 파일을 위한 커널 자료구조
- fd = open(“file”, O_RDONLY);

![image](https://github.com/user-attachments/assets/27ca7b1d-8b58-4284-9bd0-6b16b0fd8096)

## 파일 디스크립터 배열(Fd Array)
- 프로세스 당 하나씩 갖는다

- 파일 디스크립터 배열
  - 열린 파일 테이블의 엔트리를 가리킨다.

- 파일 디스크립터
  - 파일 디스크립터 배열의 인덱스
  - 열린 파일을 나타내는 번호

## 열린 파일 테이블(Open File Table)
- 파일 테이블 (file table)
  - 커널 자료구조
  - 열려진 모든 파일 목록
  - 열려진 파일  파일 테이블의 항목

- 파일 테이블 항목 (file table entry)
  - 파일 상태 플래그 (read, write, append, sync, nonblocking,…)
  - 파일의 현재 위치 (current file offset)
  - i-node에 대한 포인터

## 동적 i-노드 테이블(Active i-node Table)
- 동적 i-노드 테이블
  - 커널 내의 자료 구조
  - Open 된 파일들의 i-node를 저장하는 테이블

- i-노드
  - 하드 디스크에 저장되어 있는 파일에 대한 자료구조
  - 한 파일에 하나의 i-node
  - 하나의 파일에 대한 정보 저장
    - 소유자, 크기
    - 파일이 위치한 장치
    - 파일 내용 디스크 블럭에 대한 포인터

- i-node table vs. i-node

## 파일을 위한 커널 자료구조
- fd = open(“file”, O_RDONLY); //두 번 open

![image](https://github.com/user-attachments/assets/bfe69dba-bd37-405f-aa94-b12d6376d3d8)

- fd = dup(3); 혹은 fd = dup2(3,4);
![image](https://github.com/user-attachments/assets/8950c135-af89-4ef5-a34d-a40c19918a50)

## 파일 상태(file status)
- 파일 상태
  - 파일에 대한 모든 정보
  - 블록수, 파일 타입, 사용 권한, 링크수, 파일 소유자의 사용자 ID,
  - 그룹 ID, 파일 크기, 최종 수정 시간 등

- 예
![image](https://github.com/user-attachments/assets/96c687f1-2b1a-4ff1-8326-58a049277b81)

## 상태 정보 : stat()
- 파일 하나당 하나의 i-노드가 있으며 i-노드 내에 파일에 대한 모든 상태 정보가 저장되어 있다

```
#include <sys/types.h>
#include <sys/stat.h>
int stat (const char *filename, struct stat *buf);
int fstat (int fd, struct stat *buf);
int lstat (const char *filename, struct stat *buf);
```
파일의 상태 정보를 가져와서 stat 구조체 buf에 저장한다 성공하면 0, 실패하면 -1을 리턴한다

## stat 구조체 
```
struct stat {
mode_t st_mode; // 파일 타입과 사용권한
ino_t st_ino; // i-노드 번호
dev_t st_dev; // 장치 번호
dev_t st_rdev; // 특수 파일 장치 번호
nlink_t st_nlink; // 링크 수
uid_t st_uid; // 소유자의 사용자 ID
gid_t st_gid; // 소유자의 그룹 ID
off_t st_size; // 파일 크기
time_t st_atime; // 최종 접근 시간
time_t st_mtime; // 최종 수정 시간
time_t st_ctime; // 최종 상태 변경 시간
long st_blksize; // 최적 블록 크기
long st_blocks; // 파일의 블록 수
};
```

## 파일 타입 
|파일 타입| 설명|
|일반 파일| 데이터를 갖고 있는 텍스트 파일 또는 이진 화일|
|디렉터리 파일| 파일의 이름들과 파일 정보에 대한 포인터를 포함하는 파일|
|문자 장치 파일| 문자 단위로 데이터를 전송하는 장치를 나타내는 파일|
|블록 장치 파일| 블록 단위로 데이터를 전송하는 장치를 나타내는 파일|
|FIFO 파일| 프로세스 간 통신에 사용되는 파일로 이름 있는 파이프|
|소켓| 네트워크를 통한 프로세스 간 통신에 사용되는 파일|
|심볼릭 링크| 다른 파일을 가리키는 포인터 역할을 하는 파일|


