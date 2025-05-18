# 파일 시스템 

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
![image](https://github.com/user-attachments/assets/bccc9eac-8851-488c-a6a8-8cb773731339)

## 파일 타입 검사 함수
- 파일 타입을 검사하기 위한 매크로 함수 
![image](https://github.com/user-attachments/assets/15fb1ed7-2f3b-4705-9910-5163bb190a16)

## ftype.c 
```
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/* 파일 타입을 검사한다. */
int main(int argc, char *argv[])
{
  int i;
  struct stat buf;
  for (i = 1; i < argc; i++) {
    printf("%s: ", argv[i]);
    if (lstat(argv[i], &buf) < 0) {
      perror("lstat()");
      continue;
    }
    if (S_ISREG(buf.st_mode))
      printf("%s \n", "일반 파일");
    if (S_ISDIR(buf.st_mode))
      printf("%s \n", "디렉터리");
    if (S_ISCHR(buf.st_mode))
      printf("%s \n", "문자 장치 파일");
    if (S_ISBLK(buf.st_mode))
      printf("%s \n", "블록 장치 파일");
    if (S_ISFIFO(buf.st_mode))
      printf("%s \n", "FIFO 파일");
    if (S_ISLNK(buf.st_mode))
      printf("%s \n", "심볼릭 링크");
    if (S_ISSOCK(buf.st_mode))
      printf("%s \n", "소켓");
  }
  exit(0);
}
```

## 파일 사용 권한(File permissions) 
- 각 파일에 대한 권한 관리
  - 각 파일마다 사용권한이 있다
  - 소유자(owner)/그룹(group)/기타(others)로 구분해서 관리한다  

- 파일에 대한 권한
  - 읽기 r
  - 쓰기 w
  - 실행 x

## 사용권한 
- read 권한이 있어야
  - O_RDONLY O_RDWR 을 사용하여 파일을 열 수 있다

- write 권한이 있어야
  - O_WRONLY O_RDWR O_TRUNC 을 사용하여 파일을 열 수 있다

- 디렉토리에 write 권한과 execute 권한이 있어야
  - 그 디렉토리에 파일을 생성할 수 있고
  - 그 디렉토리의 파일을 삭제할 수 있다
  - 삭제할 때 그 파일에 대한 read write 권한은 없어도 됨

## 파일 사용 권한 
- 파일 사용권한(file access permission)
- stat 구조체의 st_mode 의 값

![image](https://github.com/user-attachments/assets/622a2a68-43ed-4e4d-87ab-17096939b2d2)

![image](https://github.com/user-attachments/assets/be93337a-6100-4ede-aec9-3a402e58982b)

## chmod(), fchmod() 
```
#include <sys/stat.h>
#include <sys/types.h>
int chmod (const char *path, mode_t mode );
int fchmod (int fd, mode_t mode );
```

- 파일의 사용 권한(access permission)을 변경한다

- 리턴 값
  - 성공하면 0, 실패하면 -1

- mode : bitwise OR
  - S_ISUID, S_ISGID
  - S_IRUSR, S_IWUSR, S_IXUSR
  - S_IRGRP, S_IWGRP, S_IXGRP
  - S_IROTH, S_IWOTH, S_IXOTH

## fchmod.c 
```
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
/* 파일 사용권한을 변경한다. */
main(int argc, char *argv[])
{
  long strtol( );
  int newmode;
  newmode = (int) strtol(argv[1], (char **) NULL, 8);
  if (chmod(argv[2], newmode) == -1) {
    perror(argv[2]);
    exit(1);
  }
  exit(0);
}
```

## chown()
```
#include <sys/types.h>
#include <unistd.h>
int chown (const char *path, uid_t owner, gid_t group );
int fchown (int filedes, uid_t owner, gid_t group );
int lchown (const char *path, uid_t owner, gid_t group );
```
- 파일의 user ID와 group ID를 변경한다

- 리턴
  - 성공하면 0, 실패하면 -1
- lchown()은 심볼릭 링크 자체를 변경한다
- super-user만 변환 가능

## utime()
```
#include <sys/types.h>
#include <utime.h>
int utime (const char *filename, const struct utimbuf *times );
```
- 파일의 최종 접근 시간과 최종 변경 시간을 조정한다
- times가 NULL 이면, 현재시간으로 설정된다.
- 리턴 값
  - 성공하면 0, 실패하면 -1
- UNIX 명령어 touch 참고

## utime()
```
struct utimbuf {
time_t actime; /* access time */
time_t modtime; /* modification time */
}
```
- 각 필드는 1970-1-1 00:00 부터 현재까지의 경과 시간을 초로 환산한 값

## cptime.c
```
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
  struct stat buf; // 파일 상태 저장을 위한 변수
  struct utimbuf time;
  if (argc < 3) {
    fprintf(stderr, "사용법: cptime file1 file2\n");
    exit(1);
  }
  if (stat(argv[1], &buf) <0) { // 상태 가져오기
    perror("stat()");
    exit(-1);
  }
  time.actime = buf.st_atime;
  time.modtime = buf.st_mtime;
  if (utime(argv[2], &time)) // 접근, 수정 시간 복사
    perror("utime");
  else exit(0);
}
```

## 디렉토리 구현
- 디렉터리 내에는 무엇이 저장되어 있을까?
- 디렉터리 엔트리
```
#include <dirent.h>
struct dirent {
  ino_t d_ino; // i-노드 번호
  char d_name[NAME_MAX + 1];
  // 파일 이름
}
```
![image](https://github.com/user-attachments/assets/19379240-ffa1-4e85-b738-a6e77518d60a)

## 디렉토리 리스트 
- opendir()
  - 디렉터리 열기 함수
  - DIR 포인터(열린 디렉터리를 가리키는 포인터) 리턴

- readdir()
  - 디렉터리 읽기 함수
```
#include <sys/types.h>
#include <dirent.h>
DIR *opendir (const char *path);
```
path 디렉터리를 열고 성공하면 DIR 구조체 포인터를, 실패하면 NULL을 리턴

```
struct dirent *readdir(DIR *dp);
```
한 번에 디렉터리 엔트리를 하나씩 읽어서 리턴한다

## list1.c
```
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

/* 디렉터리 내의 파일 이름들을 리스트한다. */
int main(int argc, char **argv)
{
DIR *dp;
char *dir;
struct dirent *d;
struct stat st;
char path[BUFSIZ+1];
  if (argc == 1)
    dir = "."; // 현재 디렉터리를 대상으로
  else dir = argv[1];

  if ((dp = opendir(dir)) == NULL) // 디렉터리 열기
    perror(dir);

  while ((d = readdir(dp)) != NULL) // 각 디렉터리 엔트리에 대해
    printf("%s \n", d->d_name); // 파일 이름 프린트

  closedir(dp);
  exit(0);
}
```

## 파일 이름/크기 출력
- 디렉터리 내에 있는 파일 이름과 그 파일의 크기(블록의 수)를 출력하도록 확장
```
while ((d = readdir(dp)) != NULL) { //디렉터리 내의 각 파일
  sprintf(path, "%s/%s", dir, d->d_name); // 파일경로명 만들기
  if (lstat(path, &st) < 0) // 파일 상태 정보 가져오기
    perror(path);
  printf("%5d %s", st->st_blocks, d->name); // 블록 수, 파일 이름 출력
  putchar('\n');
}
```

## st_mode
- lstat() 시스템 호출
  - 파일 타입과 사용권한 정보는 st->st_mode 필드에 함께 저장됨

- st_mode 필드
  - 4비트: 파일 타입
  - 3비트: 특수용도
  - 9비트: 사용권한
    - 3비트: 파일 소유자의 사용권한
    - 3비트: 그룹의 사용권한
    - 3비트: 기타 사용자의 사용권한

![image](https://github.com/user-attachments/assets/d7860815-faae-48cc-a6e7-4c4d2c00e938)

## 디렉토리 리스트 : 예 
- list2.c
  - ls –l 명령어처럼 파일의 모든 상태 정보를 프린트

- 프로그램 구성
  - main() 메인 프로그램
  - printStat() 파일 상태 정보 프린트
  - type() 파일 타입 리턴
  - perm() 파일 사용권한 리턴

## list2.c 
![image](https://github.com/user-attachments/assets/ce0dcbb6-5ab2-4384-9879-58467b439da1)
```
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* 함수 원형 선언 */
char type(mode_t);
char *perm(mode_t);
void printStat(char*, char*, struct stat*);

/* 디렉터리 내용을 자세히 리스트한다. */
int main(int argc, char **argv)
{
    DIR *dp;
    char *dir;
    struct stat st;
    struct dirent *d;
    char path[BUFSIZ+1];

    if (argc == 1)
        dir = ".";
    else
        dir = argv[1];

    if ((dp = opendir(dir)) == NULL) { // 디렉터리 열기 실패
        perror(dir);
        exit(1);
    }

    while ((d = readdir(dp)) != NULL) { // 디렉터리 항목 순회
        sprintf(path, "%s/%s", dir, d->d_name); // 파일 경로 생성
        if (lstat(path, &st) < 0) { // 파일 상태 정보 가져오기
            perror(path);
            continue;
        }
        printStat(path, d->d_name, &st); // 상태 정보 출력
        putchar('\n');
    }

    closedir(dp);
    exit(0);
}

/* 파일 상태 정보를 출력 */
void printStat(char *pathname, char *file, struct stat *st) {
    printf("%5d ", st->st_blocks);
    printf("%c%s ", type(st->st_mode), perm(st->st_mode));
    printf("%3d ", st->st_nlink);
    printf("%s %s ", getpwuid(st->st_uid)->pw_name,
                     getgrgid(st->st_gid)->gr_name);
    printf("%9d ", st->st_size);
    printf("%.12s ", ctime(&st->st_mtime)+4);
    printf("%s", file);
}

/* 파일 타입을 리턴 */
char type(mode_t mode) {
    if (S_ISREG(mode))  return('-');
    if (S_ISDIR(mode))  return('d');
    if (S_ISCHR(mode))  return('c');
    if (S_ISBLK(mode))  return('b');
    if (S_ISLNK(mode))  return('l');
    if (S_ISFIFO(mode)) return('p');
    if (S_ISSOCK(mode)) return('s');
    return('?');
}

/* 파일 사용권한을 리턴 */
char* perm(mode_t mode) {
    int i;
    static char perms[10] = "---------";

    for (i = 0; i < 3; i++) {
        if (mode & (S_IREAD >> i*3))
            perms[i*3] = 'r';
        if (mode & (S_IWRITE >> i*3))
            perms[i*3+1] = 'w';
        if (mode & (S_IEXEC >> i*3))
            perms[i*3+2] = 'x';
    }
    return(perms);
}
```

