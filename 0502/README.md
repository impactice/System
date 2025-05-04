# 파일 시스템과 파일 입출력 

## 파일 시스템 

### 파일 시스템 보기 
- 사용법
```
$ df 파일시스템*
```
파일 시스템에 대한 디스크 사용 정보를 보여준다

- 사용 예
```
$ df
```
```
Filesystem 1K-blocks Used Available Use% Mounted on
udev 1479264 0 1479264 0% /dev
tmpfs 302400 1684 300716 1% /run
/dev/sda5 204856328 14082764 180297788 8% /
/dev/sda1 523248 4 523244 1% /boot
... 
```
- / 루트 파일 시스템 현재 8% 사용
- /dev 각종 디바이스 파일들을 위한 파일 시스템
- /boot 리눅스 커널의 메모리 이미지와 부팅을 위한 파일 시스템

### 디스크 사용량 보기 
- 사용법
```
$ du [-s] 파일명*
```
파일 혹은 디렉터리의 사용량을 보여준다 파일을 명시하지 않으면 현재
디렉터리 내의 모든 파일들의 사용 공간을 보여준다

- 예
```
$ du
```
```
208 ./사진
4 ./.local/share/nautilus/scripts
8 ./.local/share/nautilus
144 ./.local/share/gvfs-metadata
4 ./.local/share/icc
```
```
$ du -s
```
```
22164 .
```
### 파일 시스템 구조 

![image](https://github.com/user-attachments/assets/67eb3bdb-4f8e-4d13-b378-f0d0cd61fc22)

- 부트 블록(Boot block)
  - 파일 시스템 시작부에 위치하고 보통 첫 번째 섹터를 차지
  - 부트스트랩 코드가 저장되는 블록

- 슈퍼 블록(Super block)
  - 전체 파일 시스템에 대한 정보를 저장
    - 총 블록 수, 사용 가능한 i-노드 개수, 사용 가능한 블록 비트 맵, 블록의 크기, 사용중인 블록 수, 사용 가능한 블록 수 등

- i-리스트(i-list)
  - 각 파일을 나타내는 모든 i-노드들의 리스트
  - 한 블록은 약 40개 정도의 i-노드를 포함
 
- 데이터 블록(Data block)
  - 파일의 내용(데이터)을 저장하기 위한 블록들    

## 파일 상태 정보와 i-노드 

### 파일 상태(file status) 
- 파일 상태
  - 파일에 대한 모든 정보
  - 블록 수, 파일 타입, 접근권한, 링크 수, 파일 소유자의 사용자 ID, 그룹 ID, 파일 크기, 최종 수정 시간

![image](https://github.com/user-attachments/assets/9c413301-45d6-4610-af8c-bf68607a843a)

- 예
![image](https://github.com/user-attachments/assets/9b355ad9-d00a-4caa-a231-6d50f24704c7)

### stat 명령어 
- 사용법
```
$ stat [옵션] 파일
```
파일의 자세한 상태 정보를 출력한다

- 예
```
$ stat cs1.txt
```
```
File: cs1.txt
Size: 2088 Blocks: 8 IO Block: 4096 일반 파일
Device: 803h/2051d Inode: 1196554 Links: 1
Access: (0600/-rw-rw-r--) Uid: (1000/chang) Gid: (1000/chang)
Access: 2021-10-04 01:28:01.726822341 -0700
Modify: 2021-10-04 01:28:01.726822341 -0700
Change: 2021-10-04 01:28:01.726822341 -0700
Birth: 2021-10-04 01:28:01.726822341 -0700
```

### i-노드
- 한 파일은 하나의 i-노드를 갖는다
```
$ ls -i cs1.txt
```
1196554 cs1.txt
- 파일에 대한 모든 정보를 가지고 있음
![image](https://github.com/user-attachments/assets/8413e54b-370c-411d-b09e-db07b46e038a)


----------------------------------------------------------------------

## 컴퓨터 시스템 구조 
- 유닉스 커널(kernel)
  - 하드웨어를 운영 관리하여 다음과 같은 서비스를 제공
  - 파일 관리(File management)
  - 프로세스 관리(Process management)
  - 메모리 관리(Memory management)
  - 통신 관리(Communication management)
  - 주변장치 관리(Device management)

![image](https://github.com/user-attachments/assets/9aa232eb-90fc-4023-b851-06f9c4fb0a0f)


## 시스템 호출 
- 시스템 호출은 커널에 서비스 요청을 위한 프로그래밍 인터페이스
- 응용 프로그램은 시스템 호출을 통해서 커널에 서비스를 요청한다

![image](https://github.com/user-attachments/assets/c4931891-5d44-4166-bf99-63ff5f41e096)

## 시스템 호출 과정 

![image](https://github.com/user-attachments/assets/3841d371-f167-473d-b43c-b5f27aaa3a71)

## 시스템 호출 요약 

![image](https://github.com/user-attachments/assets/6a75c805-235e-4798-8deb-80d265c109ce)

## 유닉스에서 파일 
- 연속된 바이트의 나열
- 특별한 다른 포맷을 정하지 않음
- 디스크 파일뿐만 아니라 외부 장치에 대한 인터페이스
![image](https://github.com/user-attachments/assets/4aa90eea-3277-4d91-959b-9bb23247fdf3)

## 파일 열기: open() 
- 파일을 사용하기 위해서는 먼저 open() 시스템 호출을 이용하여 파일을 열어야 한다
- 파일 디스크립터는 열린 파일을 나타내는 번호
```
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int open (const char *path, int oflag, [ mode_t mode ]);
```
파일 열기에 성공하면 파일 디스크립터를, 실패하면 -1을 리턴 

- oflag
  - O_RDONLY
    읽기 모드, read() 호출은 사용 가능
  - O_WRONLY
    쓰기 모드, write() 호출은 사용 가능
  - O_RDWR
    읽기/쓰기 모드, read(), write() 호출 사용 가능
  - O_APPEND
    데이터를 쓰면 파일끝에 첨부된다
  - O_CREAT
    해당 파일이 없는 경우에 생성하며 mode는 생성할 파일의 사용권한을 나타낸다
  -  O_TRUNC
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
  - 다음 시스템 호출과 동일 open(path, WRONLY | O_CREAT | O_TRUNC, mode); 
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
  - **fd**가 나타내는 파일에서 **nbytes** 만큼의 데이터를 읽고 읽은 데이터는 **bur**에 저장한다
 
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
  if ((fd1 = open(argv[1], O_RDONLY)) == -1) {
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
}
```
