# 리눅스 기본 명령어 사용

## 기본 명령어

### 날짜 및 시간 확인
- `date` 명령어를 사용하여 현재 날짜와 시간 정보를 확인할 수 있습니다.
  
  예시:$ date 2022. 01. 01. (토) 12:26:10 KST 


### 시스템 정보 확인
1. **호스트 이름 확인**
 - `hostname` 명령어로 시스템의 호스트 이름을 확인할 수 있습니다.

   예시:
   ```
   $ hostname 
   ```
   ```
   linux.sookmyung.ac.kr 
   ```

2. **운영 체제 이름 확인**
 - `uname` 명령어로 운영 체제의 이름을 확인할 수 있습니다.

   예시:
   ```
   $ uname 
   ```
   ```
   Linux
   ```

3. **시스템 정보 전체 확인**
 - `uname -a` 명령어를 사용하면 시스템의 커널 버전, 아키텍처 등 자세한 정보를 확인할 수 있습니다.

   예시:
   ```
   $ uname -a 
   ```
   ```
   Linux Ubuntu 5.11.0-31-generic #33-Ubuntu SMP Wed Aug 11 13:19:04 UTC 2021 x86_64 x86_64 x86_64 GNU/Linux
   ```

### 사용자 정보 확인
1. **현재 사용자 확인**
   - `whoami` 명령어를 사용하면 현재 로그인한 사용자의 이름을 확인할 수 있습니다.
   
     예시:
     ```
     $ whoami 
     ``` 
     ```
     chang
     ```

2. **시스템에 로그인한 사용자 확인**
   - `who` 명령어를 사용하면 시스템에 로그인한 사용자 목록과 로그인 정보를 확인할 수 있습니다.
   
     예시:
     ```
     $ who 
     ```
     ```
     chang tty2 2022-01-09 12:19 (tty2)
     ```

### 디렉터리 내용 확인
- `ls` 명령어를 사용하면 현재 디렉터리의 내용을 확인할 수 있습니다.
  
  예시:$ ls 
  - 공개 다운로드 문서 바탕화면 비디오 사진 음악 템플릿

### 패스워드 변경 (`passwd`)
사용자의 비밀번호를 변경하는 명령어입니다.

```sh
$ passwd 
``` 
```
passwd: chang용 암호를 변경하는 중
기존 로그인 암호를 입력하십시오:
새 암호:
새 암호를 다시 입력하십시오:
passwd: 암호(chang용)가 성공적으로 변경되었습니다. 
```
### 화면 정리 
```
$ clear
```
### 온라인 매뉴얼: man 
```
$ man ls 
```
```
LS(1) User Commands LS(1)
NAME
ls - list directory contents
SYNOPSIS
ls [OPTION]... [FILE]...
DESCRIPTION
List information about the FILEs (the current directory by default).
Sort entries alphabetically if none of -cftuvSUX nor --sort is speci‐
fied.
Mandatory arguments to long options are mandatory for short options
too.
-a, --all
do not ignore entries starting with .
-A, --almost-all
do not list implied . and ..
Manual page ls(1) line 1 (press h for help or q to quit) 
``` 

### 명령어에 대한 간단한 설명: whatis 
```
$ whatis ls 
``` 
```
ls (1) - 경로의 내용을 나열한다.
ls (1p) - list directory contents 
```
## 파일과 디렉터리

### 1. 일반 파일 (Ordinary File)
- 데이터를 저장하며, 디스크에 기록되는 파일입니다.
- **종류**:
  - **텍스트 파일**: 사람이 읽을 수 있는 문자로 이루어진 파일 (예: `.txt`, `.c`, `.html` 등).
  - **이진 파일**: 실행 파일, 이미지, 압축 파일 등 (예: `.exe`, `.jpg`, `.zip` 등).

---

### 2. 디렉터리 (Directory) 또는 폴더 (Folder)
- 파일을 계층적으로 조직화하는 특수한 파일입니다.
- 디렉터리 내에는 여러 개의 **파일** 또는 **서브디렉토리**가 포함될 수 있습니다.

### 3.장치 파일(device special file) 
- 물리적인 장치에 대한 내부적인 표현 
- 키보드(stdin), 모니터(stdout), 프린터 등도 파일처럼 사용 

### 4. 심볼릭 링크 파일
- 어떤 파일을 가리키는 또 하나의 경로명을 저장하는 파일 

### 디렉터리 계층 구조 
- 리눅스의 디렉터리는 루트로부터 시작하여 트리 형태의 계층 구조를 이룬다. 
![Image](https://github.com/user-attachments/assets/05d15e15-c963-4dbd-a507-f0016bf52195) 

### 홈 디렉터리 
- 홈 디렉터리(home directory) 
  - 각 사용자마다 별도의 홈 디렉터리가 있음 
  - 사용자가 로그인하면 홈 디렉터리에서 작업을 시작함
![Image](https://github.com/user-attachments/assets/4e364964-bc20-49d0-8e12-3e8e4137554d) 

### 경로명 
- 파일이나 디렉터리에 대한 정확한 이름 
- 절대 경로명(absolute pathname) 
  - 루트 디렉터리로부터 시작하여 경로 이름을 정확하게 적는 것 
- 상대 경로명(relative path name)
  - 현재 작업 디렉터리부터 시작해서 경로 이름을 적는 것
![Image](https://github.com/user-attachments/assets/b23819ba-743a-4d74-a928-6f31c396519c) 

### 명령어의 경로 확인: which 
- 사용법 
``` 
$ which 명령어
``` 
명령어의 절대경로를 보여준다. 

- 예 
``` 
$ which ls
/bin/ls
$ which pwd
/bin/pwd
$ which passwd
/bin/passwd 
``` 

## 디렉터리 명령어 

### 현재 작업 디렉터리 출력: pwd(print working directory) 
- 사용법 
``` 
$ pwd
``` 
현재 작업 디렉터리의 절대 경로명을 출력한다. 

- 현재 작업 디렉터리(current working directory) 
  - 현재 작업 중인 디렉터리 
  - 로그인 하면 홈 디렉터리에서부터 작업이 시작된다.  

- 예 
``` 
$ pwd
/home/chang/바탕화면
$ cd ~
$ pwd
/home/chang 
``` 

### 디렉터리 이동: cd(change directory) 
- 사용법 
```
$ cd [디렉터리]
``` 
현재 작업 디렉터리를 지정된 디렉터리로 이동한다.
디렉터리를 지정하지 않으면 홈 디렉터리로 이동한다.

- 예 
```
$ cd
$ cd ~
$ cd 바탕화면
$ pwd
/home/chang/바탕화면
$ cd .. // 부모 디렉터리로 이동 
``` 

### 디렉터리 생성: mkdir(make directory) 

- 사용법 
```
$ mkdir [-p] 디렉터리+
```
디렉터리(들)을 새로 만든다. 

- 예 
```
$ cd ~ // 홈 디렉터리로 이동
$ mkdir test
$ mkdir test temp
$ ls -l
drwxrwxr-x. 2 chang chang 6 5월 12 10:12 temp
drwxrwxr-x. 2 chang chang 6 5월 12 10:12 test 
``` 

### 디렉터리 생성: mkdir 
- 중간 디렉터리 자동 생성 옵션 -p
  - 필요한 경우에 중간 디렉터리를 자동으로 만들어 준다. 

- 예 : ~/dest 디렉터리가 없는 경우 
``` 
$ mkdir ~/dest/dir1
mkdir: '/home/chang/dest/dir1' 디렉터리를 만들 수 없습
니다: 그런 파일이나 디렉터리가 없습니다
$ mkdir –p ~/dest/dir1 
``` 

### 디렉터리 삭제 : rmdir(remove directory) 
- 사용법(주의: 빈 디렉토리만 삭제할 수 있다) 
``` 
$ rmdir 디렉터리+
``` 
디렉터리(들)을 삭제한다. 
 
- 예 
```
$ rmdir test
rmdir: failed to remove ‘test’: 디렉터리가 비어있지 않음 
``` 

## 디렉터리 리스트 

### 디렉터리 리스트: ls(list) 
- 사용법 
```
$ ls(혹은 dir) [-aslFR] 디렉터리* 파일* 
``` 
지정된 디렉터리의 내용을 리스트 한다. 디렉터리를 지정하지 않으면 현재 디렉터리
내용을 리스트 한다. 또한 파일을 지정하면 해당 파일만을 리스트 한다. 

- 예 
```
$ ls /
bin dev home lib64 mnt proc run srv tmp var
boot etc lib media opt root sbin sys usr
$ ls ~
test 공개 다운로드 문서 바탕화면 비디오 사진 음악 템플릿
$ cd test
$ ls
cs1.txt 
``` 

### ls 명령어 옵션 
- 주요 옵션 

![Image](https://github.com/user-attachments/assets/64f91472-859c-4b3a-bcb7-57b6c2552da1) 

- ls -s 
  - -s(size) 옵션 
  - 디렉터리 내에 있는 모든 파일의 크기를 K 바이트 단위로 출력 
``` 
$ ls -s 
``` 
```
합계 4
4 cs1.txt
```

- ls -a 
  - -a(all) 옵션 
  - 숨겨진 파일들을 포함하여 모든 파일과 디렉터리를 리스트 
  - “.”은 현재 디렉터리, “..”은 부모 디렉터리 
```
$ ls -a
```
``` 
. .. cs1.txt
``` 

- ls -l(long) 
  - 파일 속성(file attribute) 출력 
  - 블록 수, 파일 종류, 접근권한, 링크 수, 소유자명, 크기, 수정 시간, 파일 이름 등 

![Image](https://github.com/user-attachments/assets/5bb9a520-2901-4d60-9cce-a8dd31b8a005) 

- ls –asl 
```
$ ls -asl
``` 
```
합계 12
4 drwxr-xr-x 2 chang chang 4906 4월 16일 13:37 .
4 drwx------ 3 chang chang 4096 4월 16일 13:37 ..
4 -rw-r--r-- 1 chang chang 2088 4월 16일 13:37 cs1.txt 
``` 
- ls –F 
  - 기호로 파일의 종류를 표시 
  *: 실행파일, /: 디렉터리, @:심볼릭 링크
