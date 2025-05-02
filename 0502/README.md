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















