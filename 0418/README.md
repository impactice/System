# 프로세스 

## 프로세스(process) 
- 실행중인 프로그램을 **프로세스**(process)라고 한다
- 프로세스 번호
  - 각 프로세스는 유일한 프로세스 번호 PID를 갖는다
- 부모 프로세스
  - 각 프로세스는 부호 프로세스에 의해 생성된다
 
  - 프로세스의 종류
    - 시스템 프로세스
      시스템 운영에 필요한 기능을 수행하는 프로세스
      어떤 서비스를 위해 부팅 시 생성되는 데몬 프로세스 대표적인 예
    - 사용자 프로세스
      사용자의 명령 혹은 프로그램을 실행시키기 위해 생성된 프로세스

## 프로세스 상태 보기: ps(process staus)
- 사용법
```
$ ps [-옵션]
```
```
현재 시스템 내에 존재하는 프로세스들의 실행 상태를 요약해서 출력한다.
```
- 사용 예
```
$ ps
```
```
PID TTY TIME CMD
1519 pts/3 00:00:00 bash
1551 pts/3 00:00:00 ps
```
```
$ ps -f
```
```
UID PID PPID C STIME TTY TIME CMD
chang 1519 1518 0 17:40 pts/3 00:00:00 /bin/bash
chang 1551 1519 0 17:40 pts/3 00:00:00 ps -f
```

## ps -ef 
```
$ ps –ef | more
```
```
UID PID PPID C STIME TTY TIME CMD
root 1 0 0 9월30 ? 00:00:23 /sbin/init auto noprompt
root 2 0 0 9월30 ? 00:00:00 [kthreadd]
root 3 2 0 9월30 ? 00:00:00 [rcu_gp]
root 4 2 0 9월30 ? 00:00:00 [rcu_par_gp]
root 6 2 0 9월30 ? 00:00:00 [kworker/0:0H-events_highpri]
root 9 2 0 9월30 ? 00:00:00 [mm_percpu_wq]
root 10 2 0 9월30 ? 00:00:00 [rcu_tasks_rude_]
root 11 2 0 9월30 ? 00:00:00 [rcu_tasks_trace]
root 12 2 0 9월30 ? 00:00:03 [ksoftirqd/0]
root 13 2 0 9월30 ? 00:01:07 [rcu_sched]
root 14 2 0 9월30 ? 00:00:02 [migration/0]
...
root 24 2 0 9월30 ? 00:00:00 [netns]
root 25 2 0 9월30 ? 00:00:00 [inet_frag_wq]
root 26 2 0 9월30 ? 00:00:00 [kauditd]
--More--
```

## ps 출력 정보 
![image](https://github.com/user-attachments/assets/b9ea66e1-55a7-40e8-8d07-4c70af2eea3f) 

## 특정 프로세스: pgrep 
- 특정 프로세스만 리스트
```
$ ps –ef | grep –w sshd
```

- 사용법
```
$ pgrep [옵션] [패턴]
```
```
패턴에 해당하는 프로세스들만을 리스트 한다.
-l : PID와 함께 프로세스의 이름을 출력한다.
-f : 명령어의 경로도 출력한다.
-n : 패턴과 일치하는 프로세스들 중에서 가장 최근 프로세스만을 출력한다.
-x : 패턴과 정확하게 일치되는 프로세스만 출력한다.
-a : 전체 명령어 줄과 PID를 출력한다.
```

- 예
```
$ pgrep sshd
```
```
5032
```
- -l 옵션: 프로세스 번호와 프로세스 이름을 함께 출력
```
$ pgrep -l sshd
```
```
5032 sshd
```
- -n 옵션: 가장 최근 프로세스만 출력한다
```
$ pgrep -ln sshd
```
```
5032 sshd
```

# 작업 제어 
## 쉘과 프로세스 
![image](https://github.com/user-attachments/assets/89768ba0-e345-4ca1-8e4d-7c25958b082a)

## 후면 처리 
$ 명령어 &
[1] 프로세스번호  

![image](https://github.com/user-attachments/assets/61a31ae8-a0b4-4acc-979d-d0ce360d919a)

- 예
```
$ sleep 10 &
```
```
[1] 6530
```
```
$ ps
```
```
PID TTY TIME CMD
1519 pts/0 00:00:00 bash
6530 pts/0 00:00:00 sleep
6535 pts/0 00:00:00 ps
```

## 쉴 재우기 
- 사용법
```
$ sleep 초
```
```
명시된 시간만큼 프로세스 실행을 중지시킨다
```

- 예
```
$ (echo 시작; sleep 5; echo 끝)
```

## 강제 종료 
- 강제종료 Ctrl-C
```
$ 명령어
^C
```
- 예
```
$ (sleep 100; echo DONE)
^C
$
```
- 실행 정지 Ctrl-Z
```
$ 명령어
^Z
[1]+ 정지됨 명령어
```

## 후면 작업의 전면 전환: fg(foreground)  
```
$ fg
```
```
정지된 작업을 다시 전면에서 실행시킨다.
```

- 예
```
$ (sleep 100; echo done)
```
```
^Z
[1]+ 정지됨 ( sleep 100; echo DONE )
```
```
$ fg
( sleep 100; echo DONE )
```
```
$ fg %작업번호
```
```
작업번호에 해당하는 후면 작업을 전면 작업으로 전환시킨다
```

- 예
```
$ (sleep 100; echo DONE) &
```
```
[1] 10067
```
```
$ fg %1
```
```
( sleep 100; echo DONE )
```

## 전면 작업의 후면 전환: bg(background) 
- 사용법
  - Ctrl-Z 키를 눌러 전면 실행중인 작업을 먼저 중지시킨 후
  - bg 명령어 사용하여 후면 작업으로 전환

```
$ bg %작업번호
```
```
작업번호에 해당하는 중지된 작업을 후면 작업으로 전환하여 실행한다.
```

- 예
```
$ ( sleep 100; echo DONE )
```
```
^Z
[1]+ 정지됨 ( sleep 100; echo DONE )
$ bg %1
[1]+ ( sleep 100; echo DONE ) &
```

## 후면 작업의 입출력 제어 
- 후면 작업의 출력
- 후면 작업의 입력

# 프로세스 제어 
- 후면 작업의 출력
```
$ 명령어 > 출력파일 &
$ find . -name test.c -print > find.txt &
$ find . -name test.c -print | mail chang &
```
```
- 후면 작업의 입력
$ 명령어 < 입력파일 &
```

## 프로세스 끝내기: kill 
- 프로세스 강제 종료
```
$ kill 프로세스번호
$ kill %작업번호
```
```
프로세스 번호(혹은 작업 번호)에 해당하는 프로세스를 강제로 종료시킨다
```

- 예
```
$ (sleep 100; echo done) &
```
```
[1] 8320
```
```
$ kill 8320 혹은 $ kill %1
```
```
[1] 종료됨 ( sleep 100; echo done )
```

- exit 명령어
```
exit [종료코드]
```

## 프로세스 기다리기: wait 
- 사용법
$ wait [프로세스번호]
프로세스 번호로 지정한 자식 프로세스가 종료될 때까지 기다린다.
지정하지 않으면 모든 자식 프로세스가 끝나기를 기다린다
- 예
```
$ (sleep 10; echo 1번 끝) &
```
```
[1] 1231
```
```
$ echo 2번 끝; wait 1231; echo 3번 끝
```
```
2번 끝
1번 끝
3번 끝
```

- 예
```
$ (sleep 10; echo 1번 끝) &
$ (sleep 10; echo 2번 끝) &
$ echo 3번 끝; wait; echo 4번 끝
```
```
3번 끝
1번 끝
2번 끝
4번 끝
```

## 프로세스 우선순위 
- 실행 우선순위 nice 값
  - 19(제일 낮음) ~ -20(제일 높음)
  - 보통 기본 우선순위 0으로 명령어를 실행

- nice 명령어
```
$ nice [-n 조정수치] 명령어 [인수들]
```
```
주어진 명령을 조정된 우선순위로 실행한다
```

- 예
```
$ nice // 현재 우선순위 출력
```
```
0
```
```
$ nice -n 10 ps –ef // 조정된 우선순위로 실행
```
## 프로세스 우선순위 조정 
- 사용법
```
$ renice [-n] 우선순위 [-gpu] PID
```
```
이미 수행중인 프로세스의 우선순위를 명시된 우선순위로 변경한다.
-g : 해당 그룹명 소유로 된 프로세스를 의미한다.
-u : 지정한 사용자명의 소유로 된 프로세스를 의미한다.
-p : 해당 프로세스의 PID를 지정한다
```

# 프로세스의 사용자 ID 
## 프로세스의 사용자 ID 
- 프로세스는 사용자 ID와 그룹 ID를 갖는다
  - 그 프로세스를 실행시킨 사용자의 ID와 사용자의 그룹 ID
  - 프로세스가 수행할 수 있는 연산을 결정하는 데 사용된다
 
- id 멸령어
```
 $ id [사용자명]
```
```
사용자의 실제 ID와 유효 사용자 ID, 그룹 ID 등을 보여준다
```
```
$ id
```
```
uid=1000(chang) gid=1000(chang) 그룹들=1000(chang),4(adm),24(cdrom),
27(sudo),30(dip),46(plugdev),121(lpadmin),132(lxd),133(sambashare)
```
```
$ echo $UID $EUID
```
```
1000 1000
```

- 프로세스의 실제 사용자 ID(real user ID)
  - 그 프로세스를 실행시킨 사용자의 ID로 설정된다
  - 예: chang 사용자 ID로 로그인하여 어떤 프로그램을 실행시키면 그 프로세스의 실제 사용자 ID는 chang이 된다
- 프로세스의 유효 사용자 ID(effective user ID)
  - 현재 유효한 사용자 ID
  - 보통 유효 사용자 ID와 실제 사용자 ID는 같다
  - 새로 파일을 만들 때나 파일의 접근권한을 검사할 때 주로 사용됨
  - 특별한 실행파일을 실행할 때 유효 사용자 ID는 달라진다

![image](https://github.com/user-attachments/assets/a85f447e-2587-4603-b2fc-658688e3e64f) 

## set-user-id 실행파일 
- set-user-id(set user ID upon execution) 실행권한
  - set-user-id가 설정된 실행파일을 실행하면
  - 이 프로세스의 유효 사용자 ID는 그 실행파일의 소유자로 바뀜
  - 이 프로세스는 실행되는 동안 그 파일의 소유자 권한을 갖게 됨
- 예
```
$ ls –l /bin/passwd
```
```
-rwsr-xr-x. 1 root root 59976 7월 14 14:57 /bin/passwd 
```

- set-group-id(set group ID upon execution) 실행권한
  - 실행되는 동안에 그 파일 소유자의 그룹이 프로세스의 유효 그룹 ID가 된다
  - set-group-id 실행권한은 8진수 모드로는 2000으로 표현된다
 
- set-group-id 실행파일 예   
```
$ ls -l /bin/wall
```
```
-r-xr-sr-x. 1 root tty 35200 2월 25 2021 /bin/wall 
```

## set-user-id 실행파일을 실행하는 과정 
- /bin/passwd 파일은 set-user-id 실행권한이 설정된 실행파일이며 소유자는 root
- 일반 사용자가 이 파일을 실행하면 이 프로세스의 유효 사용자 ID는 root가 됨.
- 유효 사용자 ID가 root이므로 root만 수정할 수 있는 암호 파일 /etc/shadow 파일을 접근하여 수정

![image](https://github.com/user-attachments/assets/6197d28a-924e-4119-82ee-cedae81b4e1a)

## set-user-id/set-group-id 설정 
- set-user-id 실행권한 설정
```
$ chmod 4755 파일 혹은 $ chmod u+s 파일
```
- set-group-id 실행권한 설정
```
$ chmod 2755 파일 혹은 $ chmod g+s 파일
```

# 시그널과 프로세스 
## 시그널 
- 시그널을 이용하여 프로세스를 제어한다
  - 시그널은 예기치 않은 사건이 발생할 때 이를 알리는 소프트웨어 인터럽트이다
- 시그널 발생 예
  - SIGFPE 부동소수점 오류
  - SIGPWR 정전
  - SIGALRM 알람시계 울림
  - SIGCHLD 자식 프로세스 종료
  - SIGINT 키보드로부터 종료 요청 (Ctrl-C)
  - SIGTSTP 키보드로부터 정지 요청 (Ctrl-Z)

 ![image](https://github.com/user-attachments/assets/2cfce5d5-c6fe-4e42-a5b9-b99949e8b994) 

 ## 주요 시그널 
 ![image](https://github.com/user-attachments/assets/e0297a8c-683b-4c70-95fa-71bc9d263eb8) 

 ![image](https://github.com/user-attachments/assets/6d202c5f-3dae-4389-b5a9-744a32ac2118)

## 시그널 리스트 
![image](https://github.com/user-attachments/assets/69df11ba-ee74-4184-a363-5ab78ef99491) 

## 시그널 보내기: kill 명령어 
- killl 명령어
  - 한 프로세스가 다른 프로세스를 제어하기 위해 특정 프로세스에 임의의 시그널을 강제적으로 보낸다 
![image](https://github.com/user-attachments/assets/f8a74f5e-0ee0-4d20-87a1-efe88cae289c)

- 사용법
```
$ kill [-시그널] 프로세스번호
```
```
$ kill [-시그널] %작업번호
```
```
프로세스 번호(혹은 작업 번호)로 지정된 프로세스에 원하는 시그널을 보낸다.
시그널을 명시하지 않으면 SIGTERM 시그널을 보내 해당 프로세스를 강제 종료
```

- 종료 시그널 보내
```
$ kill –9 프로세스번호
```
```
$ kill –KILL 프로세스번호
```

- 다른 시그널 보내기
```
$ 명령어 &
```
```
[1] 1234
```
```
$ kill -STOP 1234
```
```
[1]+ 정지됨 명령어
```
```
$ kill –CONT 1234
```

# 핵심 개념  
- 프로세스는 실행중인 프로그램이다
- 각 프로세스는 프로세스 ID를 갖는다. 각 프로세스는 부모 프로세스에 의해 생선된다
- 쉘은 사용자와 운영체제 사이에 창구 역할을 하는 소프트웨어로 사용자로부터 명령어를 입력받아 이를 처리하는 명령어 처리기 역할을 한다
- 전면 처리는 명령어가 전면에서 실행되므로 쉘이 명령어 실행이 끝나기를 기다리지만 후면 처리는 명령어가 후면에서 실행되므로 쉘이 명령어 실행이 끝나기를 기다리지 않는다
- 각 프로세스는 실제 사용자 ID와 유효 사용자 ID를 갖는다
- 시그널은 예기치 않은 사건이 발생할 대 이를 알리는 소프트웨어 인터럽트이다
- kill 명령어를 이용하여 특정 프로세스에 원하는 시그널을 보낼 수 있다

# 네트워크 구성 
## LAN(Local Area Network) 
- LAN
  - 근거리 통신망으로 집, 사무실, 학교 등의 건물과 같이 가까운 지역을 한데 묶는 컴퓨터 네트워크
 
- 이더넷(Ethernet)
  - 제록스 PARC에서 개발된 LAN 구현 방법으로 현재 가장 일반적으로 사용되고 있다
 
 ![image](https://github.com/user-attachments/assets/4105dbf5-193f-4060-98fe-f181bb5fd93a) 

## 라우터(router) 
- 두 개 혹은 이상의 네트워크를 연결하는 장치
- 데이터 패킷의 목적지를 추출하여 그 경로에 따라 데이터 패킷을 다음 장치로 보내주는 장치
- 공유기 혹은 스위치라고도 함 

  ![image](https://github.com/user-attachments/assets/d9dadec5-d4ee-4c70-8e59-98ba21ba7c02) 

## 게이트웨이(Gateway) 
- 일종의 고용량 라우터로 LAN을 인터넷에 연결하는 컴퓨터나 장치

  ![image](https://github.com/user-attachments/assets/da10a440-051c-4473-b9d3-5ec63cb88d35)


- 무선 액세스 포인트(wireless access point, WAP)
  - 네트워크에서 와이파이, 블루투스 등을 이용하여 컴퓨터/프린터 등의 무선 장치들을 유선망에 연결할 수 있게 하는 장치  

# 인터넷 
## 인터넷 
- 인터넷
  - 전세계 컴퓨터가 서로 연결되어 TCP/IP 프로토콜을 이용해 정보를 주고 받는 공개 컴퓨터 통신망

- 프로토콜
  - 서로 다른 기종의 컴퓨터 사이에 어떤 자료를, 어떤 방식으로, 언제 주고 언제 받을지 등을 정해놓은 규약
  - 간단히 통신을 하기 위한 규약
 
## TCP/IP 프로토콜 
- IP(Internet Protocol)
  - 호스트의 주소지정과 패킷 분할 및 조립 기능에 대한 규약
  - 인터넷 상의 각 컴퓨터는 자신의 IP 주소를 갖는다
  - IP 주소는 네트워크에서 장치들이 서로를 인식하고 통신을 하기 위해서 사용하는 주소
  - IP 주소 예: 203.252.201.11

- TCP(Transport Control Protocol)
  - IP 위에서 동작하는 프로토콜로, 데이터의 전달을 보증하고 보낸 순서대로 받게 해준다

## 호스트명과 IP 주소 
- 인터넷에 연결된 컴퓨터에게 부여되는 고유한 이름
- 호스트명은 보통 사람이 읽고 이해할 수 있는 이름
- 도메인 이름(domain name)이라고도 한다

![image](https://github.com/user-attachments/assets/495b0a6b-6046-467a-85f9-b7d24d38b7d7) 

## 호스트명 
- 사용법
```
$ hostname
```
```
사용중인 시스템의 호스트명을 출력한다
```
- 예
```
$ hostname
```
```
linux
```

## IP 주소 
- 사용법
```
$ ip addr
```
```
사용중인 시스템의 IP 주소를 출력한다
```

- 예
```
$ ip addr
```
```
...
2: enp3s0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc ...
link/ether e8:03:9a:6a:f8:a3 brd ff:ff:ff:ff:ff:ff
inet 203.153.157.127/24 brd 203.153.157.255 scope global enp3s0
valid_lft forever preferred_lft forever
```

## DNS(Domain Name System) 
- 호스트명을 IP 주소로 번역하는 서비스
- DNS는 마치 전화번호부와 같은 역할

- nslookup(name server lookup) 명령어
  - 도메인 이름 서버(domain name server)에 호스트명에 대해 질의
```
$ nslookup 호스트명
```
```
지정된 호스트의 IP 주소를 알려준다
```
- 예
```
$ nslookup cs.sookmyung.ac.kr
```
```
Server: 203.252.192.1
Address: 203.252.192.1#53
Name: cs.sookmyung.ac.kr
Address: 203.252.201.11
```

## 사용자 정보 
- 사용법 
```
$ finger 사용자명
```
```
지정된 사용자에 대한 보다 자세한 정보를 알려준다
```
- 예
```
$ finger
```
```
Login Name TTY Idle Login Time Office
chang Ubuntu tty2 4:52 Sep 14 12:15 (tty2)
...
```
```
$ finger chang
```
```
Login name: chang Name: Ubuntu
Directory: /home/chang Shell: /bin/bash
On since Tue Sep 14 12:15 (KST) on tty2 from tty2
```

## 네트워크 설정 
- 네트워크 설정 창
- [설정] -> [네트워크]

![image](https://github.com/user-attachments/assets/74b0ebe3-8a82-41ef-ab37-4b950b53b522) 

## 네트워크 수동 설정/자동 설정 
- 수동 설정
  - IP 주소, 네트마스크, 게이트웨이 정보 입력

- 자동 설정
![image](https://github.com/user-attachments/assets/9ac83b71-55bd-4c42-ae06-582621b386d6)

# 서버 설치 
## 웹 서버 
- 웹서버
  - 리눅스 시스템이 많이 사용되고 있는 분야 중의 하나
  - 리눅스에 웹 서버가 설치되어 있어야 사용할 수 있음

- 아파치 웹 서버
  - 현재 가장 널리 사용되고 있는 웹 서버
  - 우분투에서 설치할 패키지 이름은 apache2
  - CentOS에서 패키지 이름은 httpd
 
  - PHP
    - 웹 프로그래밍 언어
    - 웹 서버와 더불어 사용자의 요청에 따라 동적으로 웹 페이지를 생성하는 데 사용
    - 광법위한 데이터를 서비스하기 위해서 MariaDB 데이터베이스와 연동하여 사용
   
  - Apachi 웹 서버, PHP, MariaDB를 총칭하여 APM이라 부름
 
## 아파치 웹 서버 설치
- 필요에 따라 아파치 웹 서버 apache2, PHP, MariaDB 설치
```
# apt install apache2
# apt install php
# apt install mariadb-server
```
- 구동(start), 서비스 활성화(enable), 실행 상태(status) 확인    
```
# systemctl start apache2
# systemctl enable apache2
# systemctl status apache2
```
