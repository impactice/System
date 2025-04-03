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

## 디렉터리 계층 구조 
- 리눅스의 디렉터리는 루트로부터 시작하여 트리 형태의 계층 구조를 이룬다. 
![Image](https://github.com/user-attachments/assets/05d15e15-c963-4dbd-a507-f0016bf52195)
