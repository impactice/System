# 파일 시스템 

## 리눅스 파일 시스템 구조 
- 유닉스 파일 시스템의 전체적인 구조
  - 부트 블록, 슈퍼 블록, i-리스트, 데이터 블록으로 구성

- ext 파일 시스템
  - 리눅스는 유닉스 파일 시스템을 확장한 ext 파일 시스템 사용

- 현재 리눅스에서 사용되는 ext4 파일 시스템
  - 1EB(엑사바이트, 1EB=1024 × 1024TB) 이상의 볼륨과
  - 16TB 이상의 파일을 지원한다

## 파일 시스템 구조
![image](https://github.com/user-attachments/assets/21ba5e24-533a-407a-832e-9d3d5926647b)
- 현재 리눅스에서 사용되고 있는 ext4 파일 시스템은 이 보다 복잡한 구조이며 이 그림은 파일 시스템의 이해를 위해 단순화하여 표시한 것임

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

## i-노드와 블록 포인터
![image](https://github.com/user-attachments/assets/1e220330-32bf-48e6-90bb-b7babab67db0)


- 데이터 블록에 대한 포인터
  - 파일의 내용을 저장하기 위해 할당된 데이터 블록의 주소

- 하나의 i-노드 내의 블록 포인터
  - 직접 블록 포인터 10개
  - 간접 블록 포인터 1개
  - 이중 간접 블록 포인터 1개
  - 삼중 간접 블록 포인터 1개

# 파일 입출력 구현 

## 파일 열기 및 파일 입출력 구현 
- 파일 열기 open()
  - 시스템은 커널 내에 파일을 사용할 준비를 한다

- 파일 입출력 구현을 위한 커널 내 자료구조
  - 파일 디스크립터 배열(Fd array)
  - 열린 파일 테이블(Open File Table)
  - 동적 i-노드 테이블(Active i-node table)

## 파일을 위한 커널 자료구조
- fd = open(“file”, O_RDONLY);

![image](https://github.com/user-attachments/assets/7792f6ab-11a2-4046-ba6c-f3c5c78eb68a)


## 파일 디스크립터 배열(Fd Array)
- 프로세스 당 하나씩 갖는다

- 파일 디스크립터 배열
  - 열린 파일 테이블의 엔트리를 가리킨다

- 파일 디스크립터
  - 파일 디스크립터 배열의 인덱스
  - 열린 파일을 나타내는 번호

## 열린 파일 테이블(Open File Table)
- 파일 테이블 (file table)
  - 커널 자료구조
  - 열려진 모든 파일 목록
  - 열려진 파일 -> 파일 테이블의 항목

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

## 한 파일을 두 번 열기 구현
- fd = open(“file”, O_RDONLY); //두 번 open

![image](https://github.com/user-attachments/assets/bfe69dba-bd37-405f-aa94-b12d6376d3d8)

- fd = dup(3); 혹은 fd = dup2(3,4);
![image](https://github.com/user-attachments/assets/8950c135-af89-4ef5-a34d-a40c19918a50)

# 파일 상태 정보 

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

## 파일 타입 출력: ftype.c 
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

## 파일 접근 권한(File permissions) 
- 각 파일에 대한 권한 관리
  - 각 파일마다 사용권한이 있다
  - 소유자(owner)/그룹(group)/기타(others)로 구분해서 관리한다  

- 파일에 대한 권한
  - 읽기 r
  - 쓰기 w
  - 실행 x

- 파일의 접근권한 가져오기
  - stat() 시스템 호출
- 파일의 접근권한 변경하기
  - chmod() 시스템 호출

## chmod(), fchmod()
```
#include <sys/stat.h>
#include <sys/types.h>
int chmod (const char *path, mode_t mode );
int fchmod (int fd, mode_t mode );
```
- 파일의 접근권한(access permission)을 변경한다
- 리턴 값
  - 성공하면 0, 실패하면 -1
- mode
  - 8진수 접근권한
  - 예: 0644

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

## 접근 및 수정 시간 변경: utime()
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

```
struct utimbuf {
  time_t actime; /* access time */
  time_t modtime; /* modification time */
}
```
- 각 필드는 1970-1-1 00:00 부터 현재까지의 경과 시간을 초로 환산한 값

## 접근 및 수정 시간 변경: touch.c
```
#include <utime.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stderr, "사용법: touch file1 \n");
    exit(-1);
  }
  utime(argv[1], NULL);
}
```

## 접근 및 수정 시간 복사: cptime.c
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

## 접근 및 수정 시간 복사
```
$ ls –asl a.c b.c
```
```
4 -rw-rw-r--. 1 chang chang 0 3월 18 12:13 a.c
4 -rw-rw-r--. 1 chang chang 5 3월 18 13:30 b.c
```

```
$ cptime a.c b.c
$ ls –asl a.c b.c
```
```
4 -rw-rw-r--. 1 chang chang 0 3월 18 12:13 a.c
4 -rw-rw-r--. 1 chang chang 5 3월 18 12:13 b.c
```

## 파일 소유자 변경 chown()
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

# 디렉터리 

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

## 디렉터리 리스트: 예
```
$ list2 .
```
```
8 drwxrwxr-x 2 chang chang 4096 Dec 21 13:43 .
8 drwxrwxr-x 16 chang chang 4096 Aug 30 18:55 ..
8 –rw-r—r-- 1 chang chang 781 Mar 21 17:13 list1.c
8 –rw-r—r-- 1 chang chang 2178 Mar 28 14:25 list2.c
24 –rwxrwxr-x 1 chang chang 8775 Mar 21 17:14 list1
32 –rwxrwxr-x 1 chang chang 13376 Dec 21 13:43 list2
...
```

## 디렉터리 생성
- mkdir() 시스템 호출
  - path가 나타내는 새로운 디렉터리를 만든다
  - "." 와 ".." 파일은 자동적으로 만들어진다

```
#include <sys/types.h>
#include <sys/stat.h>
int mkdir (const char *path, mode_t mode );
```
새로운 디렉터리 만들기에 성공하면 0, 실패하면 -1을 리턴한다

## 디렉터리 삭제 
- rmdir() 시스템 호출
  - path가 나타내는 디렉터리가 비어 있으면 삭제한다
```
#include <unistd.h>
int rmdir (const char *path);
```
디렉터리가 비어 있으면 삭제한다. 성공하면 0, 실패하면 -1을 리턴 

## 디렉터리 구현 
- 파일 시스템 내에서 디렉터리를 어떻게 구현할 수 있을까?
  - 디렉터리도 일종의 파일로 다른 파일처럼 구현된다.
  - 디렉터리도 다른 파일처럼 하나의 i-노드로 표현된다.
  - 디렉터리의 내용은 디렉터리 엔트리(파일이름, i-노드 번호)

![image](https://github.com/user-attachments/assets/38e285e6-6626-464a-977e-0d40bf7c8a3c)


# 링크 

## 하드 링크 vs 심볼릭 링크
- 하드 링크(hard link)
  - 지금까지 살펴본 링크
  - 파일 시스템 내의 i-노드를 가리키므로
  - 같은 파일 시스템 내에서만 사용될 수 있다
![image](https://github.com/user-attachments/assets/3f098dc7-615b-4595-abb0-b54ab1b10bf6)



- 심볼릭 링크(symbolic link)
  - 소프트 링크(soft link)
  - 실제 파일의 경로명 저장하고 있는 링크
  - 파일에 대한 간접적인 포인터 역할을 한다.
  - 다른 파일 시스템에 있는 파일도 링크할 수 있다
![image](https://github.com/user-attachments/assets/49fe6538-18f0-4a11-96fd-de8b435b8450)

## 링크의 구현
- link() 시스템 호출
  - 기존 파일 existing에 대한 새로운 이름 new 즉 링크를 만든다
![image](https://github.com/user-attachments/assets/fec60efe-6c77-4a39-ab6d-f8d9466faa07)

## link.c
```
#include <unistd.h>
int main(int argc, char *argv[ ])
{
  if (link(argv[1], argv[2]) == -1) {
    exit(1);
  }
  exit(0);
}
```

## unlink.c
```
#include <unistd.h>
main(int argc, char *argv[ ])
{
  int unlink( );
  if (unlink(argv[1]) == -1 {
    perror(argv[1]);
    exit(1);
  }
  exit(0);
}
```

## 심볼릭 링크
```
int symlink (const char *actualpath, const char *sympath );
```
심볼릭 링크를 만드는데 성공하면 0, 실패하면 -1을 리턴한다

```
#include <unistd.h>
int main(int argc, char *argv[ ])
{
  if (symlink(argv[1], argv[2]) == -1) {
    exit(1);
  }
  exit(0);
}
```
```
$ slink /usr/bin/gcc cc
$ ls -l cc
```
2 lrwxrwxrwx 1 chang chang 7 4월 8 19:58 cc -> /usr/bin/gcc 

## 심볼릭 링크 내용
```
#include <unistd.h>
int readlink (const char *path, char *buf, size_t bufsize);
```
path 심볼릭 링크의 실제 내용을 읽어서 buf에 저장한다.
성공하면 buf에 저장한 바이트 수를 반환하며 실패하면 –1을 반환한다

## 심볼릭 링크 내용 확인: rlink.c
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(int argc, char *argv[ ])
{
  char buffer[1024];
  int nread;
  nread = readlink(argv[1], buffer, 1024);
  if (nread > 0) {
    write(1, buffer, nread);
    exit(0);
  } else {
    fprintf(stderr, "오류 : 해당 링크 없음\n");
    exit(1);
  }
}
```

```
$ rlink cc
```
/usr/bin/gcc 

## 핵심 개념 
- 표준 유닉스 파일 시스템은 부트 블록, 슈퍼 블록, i-리스트, 데이터 블록 부분으로 구성된다
- 파일 입출력 구현을 위해서 커널 내에 파일 디스크립터 배열, 파일 테이블, 동적 i-노드 테이블 등의 자료구조를 사용한다
- 파일 하나당 하나의 i-노드가 있으며 i-노드 내에 파일에 대한 모든 상태 정보가 저장되어 있다
- 디렉터리는 일련의 디렉터리 엔트리들을 포함하고 각 디렉터리 엔트리는 파일 이름과 그 파일의 i-노드 번호로 구성된다
- 링크는 기존 파일에 대한 또 다른 이름으로 하드 링크와 심볼릭(소프트) 링크가 있다

# 파일 및 레코드 잠금 
## 파일 잠금 예제 (flock) 
```
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int fd;

    fd = open(argv[1], O_WRONLY | O_CREAT, 0600);
    if (flock(fd, LOCK_EX) != 0) {
        printf("flock error\n");
        exit(0);
    }

    for (int i = 0; i < 5; i++) {
        printf("file lock %d : %d\n", getpid(), i);
        sleep(1);
    }

    if (flock(fd, LOCK_UN) != 0) {
        printf("unlock error\n");
    }
    close(fd);
}
```
- 사용 예
```
$ flock lock.f &
$ flock lock.f &
```

# 레코드 잠금 
## 레코드 잠금 예제 - rdlock.c 
```
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int fd, id;
    struct student record;
    struct flock lock;

    if (argc < 2) {
        fprintf(stderr, "사용법 : %s 파일\n", argv[0]);
        exit(1);
    }

    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        perror(argv[1]);
        exit(2);
    }

    printf("\n검색할 학생의 학번 입력:");
    while (scanf("%d", &id) == 1) {
        lock.l_type = F_RDLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = (id - START_ID) * sizeof(record);
        lock.l_len = sizeof(record);

        if (fcntl(fd, F_SETLKW, &lock) == -1) {
            perror(argv[1]);
            exit(3);
        }

        lseek(fd, (id - START_ID) * sizeof(record), SEEK_SET);
        if ((read(fd, (char *)&record, sizeof(record)) > 0) && (record.id != 0))
            printf("이름:%s\t 학번:%d\t 점수:%d\n", record.name, record.id, record.score);
        else
            printf("레코드 %d 없음\n", id);

        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);

        printf("\n검색할 학생의 학번 입력:");
    }

    close(fd);
    exit(0);
}
```
- 기능 요약
  - 특정 학생 레코드를 읽기 전에 읽기 잠금
  - 레코드 읽은 후 잠금 해제 

## 레코드 잠금 예제 - wrlock.c
```
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int fd, id;
    struct student record;
    struct flock lock;

    if (argc < 2) {
        fprintf(stderr, "사용법 : %s 파일 \n", argv[0]);
        exit(1);
    }

    if ((fd = open(argv[1], O_RDWR)) == -1) {
        perror(argv[1]);
        exit(2);
    }

    printf("\n수정할 학생의 학번 입력:");
    while (scanf("%d", &id) == 1) {
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = (id - START_ID) * sizeof(record);
        lock.l_len = sizeof(record);

        if (fcntl(fd, F_SETLKW, &lock) == -1) {
            perror(argv[1]);
            exit(3);
        }

        lseek(fd, (long)(id - START_ID) * sizeof(record), SEEK_SET);
        if ((read(fd, (char *)&record, sizeof(record)) > 0) && (record.id != 0))
            printf("이름:%s\t 학번:%d\t 점수:%d\n", record.name, record.id, record.score);
        else
            printf("레코드 %d 없음\n", id);

        printf("새로운 점수: ");
        scanf("%d", &record.score);
        lseek(fd, (long) -sizeof(record), SEEK_CUR);
        write(fd, (char *)&record, sizeof(record));

        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);

        printf("\n수정할 학생의 학번 입력:");
    }

    close(fd);
    exit(0);
}
```
- 기능 요약
  - 특정 레코드 수정 전 쓰기 잠금
  - 수정 후 잠금 해제  

## lockf() 기반 잠금 예제 - wrlockf.c 
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "student.h"

int main(int argc, char *argv[]) {
    int fd, id;
    struct student record;

    if (argc < 2) {
        fprintf(stderr, "사용법 : %s file\n", argv[0]);
        exit(1);
    }

    if ((fd = open(argv[1], O_RDWR)) == -1) {
        perror(argv[1]);
        exit(2);
    }

    printf("\n수정할 학생의 학번 입력:");
    while (scanf("%d", &id) == 1) {
        lseek(fd, (long) (id - START_ID) * sizeof(record), SEEK_SET);

        if (lockf(fd, F_LOCK, sizeof(record)) == -1) {
            perror(argv[1]);
            exit(3);
        }

        if ((read(fd, (char *)&record, sizeof(record)) > 0) && (record.id != 0))
            printf("이름:%s\t 학번:%d\t 점수:%d\n", record.name, record.id, record.score);
        else
            printf("레코드 %d 없음\n", id);

        printf("새로운 점수: ");
        scanf("%d", &record.score);
        lseek(fd, (long) -sizeof(record), SEEK_CUR);
        write(fd, (char *)&record, sizeof(record));

        lseek(fd, (long) (id - START_ID) * sizeof(record), SEEK_SET);
        lockf(fd, F_ULOCK, sizeof(record));

        printf("\n수정할 학생의 학번 입력:");
    }

    close(fd);
    exit(0);
}
```
- 기능 요약
  - lockf()를 이용한 단순한 쓰기 잠금 예제
  - 비블로킹 또는 블로킹 방식으로 설정 가능 

## 권고 잠금 vs 강제 잠금 예제 - file_lock.c 
```
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
    static struct flock lock;
    int fd, ret, c;

    if (argc < 2) {
        fprintf(stderr, "사용법: %s 파일\n", argv[0]);
        exit(1);
    }

    fd = open(argv[1], O_WRONLY);
    if (fd == -1) {
        printf("파일 열기 실패 \n");
        exit(1);
    }

    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    lock.l_pid = getpid();

    ret = fcntl(fd, F_SETLKW, &lock);
    if (ret == 0) {
        // 파일 잠금 성공 후 대기
        c = getchar();
    }

    return 0;
}
```
- 권고 잠금 vs 강제 잠금 설정 방법
  - 파일은 그냥 생성하면 됨

- 강제 잠금 설정 명령
```
$ chmod 2644 mandatory.txt
$ ls -l mandatory.txt
-rw-r-Sr-- 1 user group size date mandatory.txt
```
- 테스트 명령 예시
```
$ ./file_lock mandatory.txt
$ ls >> mandatory.txt  # 강제 잠금이라면 실패

$ ./file_lock advisory.txt
$ ls >> advisory.txt   # 권고 잠금이라면 성공
```
- 요약
  - 권고 잠금: 커널이 강제하지 않음
  - 강제 잠금: 커널이 잠금 규칙을 강제
 
# 프로세스 (Process) 
## 쉘과 프로세스

- 쉘(Shell)이란?
  - 사용자와 운영체제 사이의 명령어 처리기
  - 명령어를 입력받아 해석하고 처리하는 역할

- 쉘의 명령 실행 예
```
$ date; who; pwd               # 명령어 열
$ (date; who; pwd) > out.txt   # 명령어 그룹
```

- 전면 처리 vs 후면 처리
  - 전면 처리: 명령 실행 완료까지 쉘이 기다림
  - 후면 처리: 백그라운드에서 실행, 쉘은 계속 입력 받음
```
$ (sleep 100; echo done) &
$ find . -name test.c -print &
$ jobs
$ fg %1
```

## 프로세스 정보 보기 
- ps 명령어
```
$ ps
$ ps -aux   # BSD 스타일
$ ps -ef    # System V 스타일
```
- 출력 예
```
UID PID PPID C STIME TTY TIME CMD
root 1   0    0 ...  /sbin/init auto noprompt
```

## 프로세스 제어 명령어 
- sleep
```
$ sleep 5
```
- kill
```
$ kill 8320
$ kill %1
```
- wait
```
$ wait [PID]     # 해당 자식 프로세스 종료 대기
$ wait           # 모든 자식 프로세스 종료 대기
```
- exit
```
$ exit 0         # 종료 코드 0
```

## 프로그램 실행
- 명령줄 인수 전달
```
int main(int argc, char *argv[]);
```
- 명령줄 인수 출력: args.c
```
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++)
        printf("argv[%d]: %s \n", i, argv[i]);
    exit(0);
}
```
- 환경 변수 출력: environ.c
```
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    extern char **environ;
    for (char **ptr = environ; *ptr != NULL; ptr++)
        printf("%s \n", *ptr);
    exit(0);
}
```
- 환경 변수 접근: myenv.c
```
#include <stdio.h>
#include <stdlib.h>

int main() {
    char *ptr;
    ptr = getenv("HOME");
    printf("HOME = %s \n", ptr);
    ptr = getenv("SHELL");
    printf("SHELL = %s \n", ptr);
    ptr = getenv("PATH");
    printf("PATH = %s \n", ptr);
    exit(0);
}
```
- 환경 변수 설정
```
#include <stdlib.h>

putenv("VAR=value");
setenv("VAR", "value", 1);
unsetenv("VAR");
```
## 프로그램 종료 
- 정상 종료
  - main()에서 return
  - exit(status) 호출
  - _exit(status) 호출

- 비정상 종료
  - abort() 또는 시그널

- exit() vs _exit()
```
#include <stdlib.h>

void exit(int status);     // 정리 수행 후 종료
void _exit(int status);    // 즉시 종료
```
- exit 처리기 등록: atexit()
```
#include <stdio.h>
#include <stdlib.h>

static void exit_handler1(void) { printf("첫 번째 exit 처리기\n"); }
static void exit_handler2(void) { printf("두 번째 exit 처리기\n"); }

int main(void) {
    atexit(exit_handler1);
    atexit(exit_handler2);
    printf("main 끝 \n");
    exit(0);
}
```
## 8.6 프로세스 ID와 사용자 ID 
- 프로세스 ID
```
#include <unistd.h>

getpid();   // 현재 프로세스 ID
getppid();  // 부모 프로세스 ID
```
- 예: pid.c
```
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("나의 프로세스 번호 : [%d] \n", getpid());
    printf("내 부모 프로세스 번호 : [%d] \n", getppid());
}
```
- 사용자 ID / 그룹 ID
```
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

int main() {
    printf("나의 실제 사용자 ID : %d (%s)\n", getuid(), getpwuid(getuid())->pw_name);
    printf("나의 유효 사용자 ID : %d (%s)\n", geteuid(), getpwuid(geteuid())->pw_name);
    printf("나의 실제 그룹 ID : %d (%s)\n", getgid(), getgrgid(getgid())->gr_name);
    printf("나의 유효 그룹 ID : %d (%s)\n", getegid(), getgrgid(getegid())->gr_name);
}
```

- ID 변경 함수
```
setuid(uid_t uid);
seteuid(uid_t uid);
setgid(gid_t gid);
setegid(gid_t gid);
```

## 프로세스 이미지 구조

- 텍스트 영역: 실행 코드
- 데이터 영역: 전역/정적 변수
- 힙: malloc() 등 동적 메모리
- 스택: 함수 호출 스택
- U-영역: 열린 파일, 작업 디렉토리 등

## 핵심 요약 
- 프로세스는 실행중인 프로그램이다.
- 쉘은 사용자와 커널 사이의 명령 인터페이스다.
- exec → main → exit 또는 _exit 흐름
- exit 처리기는 atexit()으로 등록 가능
- 프로세스는 ID, 사용자/그룹 ID를 가진다.
- 프로세스 구조: 텍스트, 데이터, 힙, 스택, U영역


