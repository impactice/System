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
### 파일 시스템 구조 

![image](https://github.com/user-attachments/assets/67eb3bdb-4f8e-4d13-b378-f0d0cd61fc22)


























