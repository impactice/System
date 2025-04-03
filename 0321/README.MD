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
   linux.sookmyung.ac.kr
   ```

2. **운영 체제 이름 확인**
 - `uname` 명령어로 운영 체제의 이름을 확인할 수 있습니다.

   예시:
   ```
   $ uname
   Linux
   ```

3. **시스템 정보 전체 확인**
 - `uname -a` 명령어를 사용하면 시스템의 커널 버전, 아키텍처 등 자세한 정보를 확인할 수 있습니다.

   예시:
   ```
   $ uname -a
   Linux Ubuntu 5.11.0-31-generic #33-Ubuntu SMP Wed Aug 11 13:19:04 UTC 2021 x86_64 x86_64 x86_64 GNU/Linux
   ```

### 사용자 정보 확인
1. **현재 사용자 확인**
   - `whoami` 명령어를 사용하면 현재 로그인한 사용자의 이름을 확인할 수 있습니다.
   
     예시:
     ```
     $ whoami
     chang
     ```

2. **시스템에 로그인한 사용자 확인**
   - `who` 명령어를 사용하면 시스템에 로그인한 사용자 목록과 로그인 정보를 확인할 수 있습니다.
   
     예시:
     ```
     $ who
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
passwd: chang용 암호를 변경하는 중
기존 로그인 암호를 입력하십시오:
새 암호:
새 암호를 다시 입력하십시오:
passwd: 암호(chang용)가 성공적으로 변경되었습니다. 

### 화면 정리 
$ clear

### 온라인 매뉴얼: man 
$ man ls
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

### 명령어에 대한 간단한 설명: whatis 
$ whatis ls
ls (1) - 경로의 내용을 나열한다.
ls (1p) - list directory contents 

## 파일과 디렉터리
