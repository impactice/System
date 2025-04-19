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

- mariadb 설치한 경우
  - 구동(start), 서비스 활성화(enable), 실행 상태(status) 확인
```
# systemctl start mariadb
# systemctl enable mariadb
# systemctl status mariadb
```
- 방화벽에 http 등록하고(add-service), 바로 적용(reload), 리스트해서 확인(list-all)
```
# firewall-cmd --permanent --add-service=http
# firewall-cmd –-reload
# firewall-cmd --list-all
```

- httpd는 기본적으로 /var/www/html 디렉터리에서 index.html 파일을 읽어서 웹 브라우저에 디스플레이함
  - index.html을 만들어 넣어서 테스트 할 수 있음
  - PHP의 동작을 확인하기 위해 phinfo.php 파일 생성 <?php phpinfo(); ?> 

- 테스트 페이지가 확인 http://IP주소/index.html

- PHP의 동작 확인 http://IP주소/phpinfo.php

## FTP 서버 설치 
- 대표적인 FTP 서버
  - vsFTPD(Very Secure File Transfer Protocol Daemon) 
- FTP 서버 설치
```
# apt install vsftpd
```
- vsftpd를 시작(start), 서비스 활서화(enable)
```
# systemctl start vsftpd.service
```
```
# systemctl enable vsftpd.service
```

- 방화벽에 신뢰할 수 있는 서비스로 등록
```
# firewall-cmd --add-service=ftp
```

## 원격 접속 서버 SSH 
- 원격 접속
  - 로컬 호스트에서 원격으로 다른 호스트에 접속하여 사용하는 것
  - telnet은 보안 취약점으로 인해 리눅스에서는 지원하지 않음
  - 보안을 강화한 원격 접속 서비스인 ssh(Secure Shell)을 지원

- ssh 데몬(ssh) 설치 후 서비스 시작
```
# apt install ssh
```
```
# systemctl start ssh 
```
```
# systemctl enable ssh
```
```
# systemctl status ssh
```

# 파일 전송 
## FTP(File Transfer Protocol) 
- 파일 전송 프로토콜(File Transfer Protocol, FTP)의 약자
  - FTP 서버와 클라이언트 사이의 파일 전송을 위한 서비스
  - 주로 파일을 업로드 하거나 다운로드 하기 위하여 사용

- ftp 혹은 sftp(secure ftp) 명령어를 이용하여 파일 전송
```
$ ftp -n [호스트명]
```
```
$ sftp -n [호스트명]
```
```
호스트명으로 지정된 FTP 서버에 접속하여 파일을 업로드 혹은 다운로드 한다
```

## ftp, sftp 명령어 
- ftp 시작(유닉스)
```
$ ftp cs.sookmyung.ac.kr
```
```
Connected to cs.sookmyung.ac.kr.
220 cs FTP server ready.
Name (cs.sookmyung.ac.kr:chang):
331 Password required for chang.
Password:
230 User chang logged in.
Remote system type is UNIX.
Using binary mode to transfer
files.
ftp>
```

- sftp 시작(리눅스)
```
$ sftp linux.sookmyung.ac.kr
```
```
Connecting to linux.sookmyung.ac.kr...
chang@linux.sookmyung.ac.kr's password:
sftp > cd test
Sftp > ls
…
```

- 다운로드
```
sftp> get 파일명
```
```
sftp> mget 파일명
```

- 업로드
```
sftp> put 파일명
```
```
sftp> mput 파일명
```

## ftp 내부 명령어 
![image](https://github.com/user-attachments/assets/fe7692ad-74fe-497f-8051-c6a807cce6e9) 

## MS 윈도우에서 sftp 사용 
- OpenSSH 클라이언트 추가 설치
  - [설정] -> [앱] -> [앱 및 기능] -> [선택적 기능] -> [OpenSSH 클라이언트]
  - sftp 명령어 실행
```
$ sftp chang@linux.sookmyung.ac.kr
```

![image](https://github.com/user-attachments/assets/0b199f53-fcc5-41e7-aca6-628308837713)

## MS 윈도우에서 FileZilla 사용 
- FileZilla
  - https://filezilla-project.org

![image](https://github.com/user-attachments/assets/e125dc40-9f04-4d06-a3fc-9ab2dc871bbe) 

# 원격 접속 
## telnet 
- 내 컴퓨터에서 원격 호스트에 연결하여 사용할 수 있다
  - 지역 호스트(local host), 원격 호스트(remote host)
``` 
$ telnet 호스트명(혹은 IP 주소)
```
```
지정된 원격 호스트에 원격으로 접속한다
```
- 예
```
$ telnet cs.sookmyung.ac.kr
```
```
Trying 203.252.201.11...
Connected to cs.
Escape character is '^]'.
SunOS 5.9
login:
```

## 안전한 원격 접속: ssh(secure shell)
- 원격 로그인 혹은 원격 명령 실행을 위한 프로그램
  - 보안을 위해 강력한 인증 및 암호화 기법 사용
  - 기존의 rsh, rlogin, telnet 등을 대체하기 위해 설계됨

```
$ ssh 사용자명@호스트명
```
```
$ ssh -l 사용자명 호스트명
```
```
지정된 원격 호스트에 사용자명으로 원격으로 접속한다
```

- 예
```
$ ssh chang@linux.sookmyung.ac.kr
```
```
chang@linux.sookmyung.ac.kr's password:
```

## 원격 명령 실행 
- 사용법
```
$ ssh 호스트명 명령
```
- 예
```
$ ssh linux.sookmyung.ac.kr who
```
```
chang@linux.sookmyung.ac.kr's password:
root :0 2022-02-09 07:48
chang pts/1 2022-02-10 09:53
```

## MS 윈도우에서 원격 접속: ssh 
- 원격 접속을 위해 OpenSSH 클라이언트를 추가 설치
- 명령 프롬프트 또는 실행 창에서 ssh 명령어를 실행
![image](https://github.com/user-attachments/assets/d7254194-8894-4f1b-8517-fd9c3eb9f3ff)

## 호스트 확인 ping
- 원격 컴퓨터의 상태를 확인
```
$ ping 호스트명
```
```
지정된 원격 호스트가 도달 가능한지 테스트하여 상태를 확인한다
```

- 예
```
$ ping www.kbs.co.kr
```
```
PING www.kbs.co.kr (211.233.32.11) 56(84) bytes of data.
64 bytes from 211.233.32.11: icmp_seq=1 ttl=245 time=2.22 ms
64 bytes from 211.233.32.11: icmp_seq=2 ttl=245 time=2.19 ms
64 bytes from 211.233.32.11: icmp_seq=3 ttl=245 time=2.85 ms
...

```
# 원격 데스크톱 연결 
## 원격 데스크톱 연결 
- 원격 데스트톱 프로토콜(Remote Desktop Protocol, RDP)
  - 원격 데스크톱 연결을 위한 프로토콜
  - 다른 컴퓨터에 GUI 인터페이스를 제공하는 프로토콜

- 윈도우에서 원격 데스크톱 연결
![image](https://github.com/user-attachments/assets/16cd269e-1b43-449a-a5f9-376bb641ea01)

![image](https://github.com/user-attachments/assets/b742772f-576b-445f-af05-11cede25c59a)

## 원격 데스크톱 연결 
![image](https://github.com/user-attachments/assets/4faa2cdc-7329-4585-86d4-b7ea23430b93) 

## 원격 데스크톱 설치 
- 자동설치 도구 apt를 이용하여 xrdp 서버 설치
```
# apt install xrdp
```
- systemctl 명령으로 xrdp 서비스 시작
```
# systemctl start xrdp.service
```
- systemctl 명령으로 xrdp 서비스가 실행되었는지 확인
```
# systemctl status xrdp.service
```
```
● xrdp.service - xrdp daemon
Loaded: loaded (/lib/systemd/system/xrdp.service; enabled; vendor preset: >
Active: active (running) since Mon 2021-08-30 12:01:20 KST; 1 day 4h ago
Docs: man:xrdp(8)
man:xrdp.ini(5)
Main PID: 855 (xrdp)
Tasks: 2 (limit: 9358)
Memory: 22.4M
```

- 부팅할 때 xrdp 서비스가 자동으로 실행되도록 설정
```
# systemctl enable xrdp.service
```

- 방화벽에서 xrdp의 포트를 열어준 후 방화벽 재시작
```
# apt install ufw
```
```
# ufw enable
```
```
# ufw allow from any to any port 3389
```

# 월드 와이드 웹 
## 월드 와이드 웹(World Wide Web, WWW, W3) 
- 월드 와이드 웹(WWW)
  - 인터넷에 연결된 컴퓨터들을 통해 사람들이 정보를 공유할 수 있는 전세계적인 정보 공간 

- 하이퍼텍스트(hypertext)
  - 문서 내의 어떤 위치에서 하이퍼링크를 통하여 연결된 문서나 미디어에 쉽게 접근
  - 하이퍼텍스트 작성 언어: HTML(Hyper Text Markup Language)

- HTTP(Hyper Text Transfer Protocol)
  - 웹 서버와 클라이언트가 통신할 때에 사용하는 프로토콜
  - 웹 문서뿐만 아니라 일반 문서, 음성, 영상, 동영상 등 다양한 형식의 데이터 전송

- URL(Uniform Resource Locator) 
  - 인터넷에 존재하는 여러 가지 자원들에 대한 주소 체계
  - http://www.mozila.or.kr
 
## 웹 브라우저(web browser)
- 웹 브라우저
  - WWW에서 정보를 검색하는 데 사용하는 소프트웨어
  - WWW에서 가장 핵심이 되는 소프트웨어
  - 웹페이지 열기, 최근 방문한 URL 및 즐겨찾기 제공, 웹페이지 저장

- 웹 브라우저 종류
  - 1993년, 모자이크(Mosaic)
  - 1994년, 넷스케이프(Netscape)
  - 1995년, 인터넷 익스플로러(Internet Explorer)
  - 파이어폭스(Firefox)
  - 사파리(Safari)
  - 크롬(Chrome)

## 크롬(Chrome) 
- 구글 크롬
  - 빠른 속도가 장점이며 간결한 디자인으로 초보자도 쉽게 사용
  - 악성코드 및 피싱 방지 기능을 사용하여 안전하고 보호된 웹 환경

## 사파리(Safari)
- 애플 사파리
  - 빠른 속도
  - 모바일용 사파리(아이팟, 아이폰, 아이패드)

## 파이어폭스(Firefox)
- 모질라(Mozilla) 파이어폭스
  - 사용자 편의를 위해 스마트 주소창, 탭 브라우징, 라이브 북마크, 통합 검색, 다양한 검색 엔진 지원 등을 제공

# 핵심 개념 
- 인터넷은 TCP/IP 프로토콜을 이용해 정보를 주고 받는 전세계적인 공개 컴퓨터 통신망이다
- 아파치 웹 서버, FTP 서버, 원격 접속 서버를 설치한다
- ftp 혹은 sftp 명령어를 이용하여 파일을 전송할 수 있다
- telnet 혹은 ssh 명령어를 이용하여 원격 호스트에 접속할 수 있다
- 웹 서버, FTP 서버, 원격 접속 서버를 설치할 수 있다 

# 파일 속성으로 파일 찾기 
## find 명령어 
- find 명령어
  - 파일 이름이나 속성을 이용하여 해당하는 파일을 찾는다

- 사용법
```
$ find 디렉터리 [-옵션]
```
```
옵션의 검색 조건에 따라 지정된 디렉터리 아래에서 해당되는 파일들을 모두 찾아 출력한다
```

- 파일명을 명시하는 -name 옵션
```
$ find 디렉터리 –name 파일명 -print 혹은 -ls
```
```
지정된 디렉터리 아래에서 파일명에 해당되는 파일들을 모두 찾아 그 경로를 출력한다
```
- 예
```
$ find ~ –name src -print
```
```
/home/chang/linux/src
```
```
$ find ~ –name src -ls
```
```
89090 4 drwxrwxr-x 13 chang cs 4096 9월22 /home/chang/linux/src
```
```
$ find /usr –name “*.c” –print 
```

## find 명령어: 검색 조건 
![image](https://github.com/user-attachments/assets/3c286e8a-b3f9-4b19-b7bc-6c35a0632275)

- 파일의 소유자(-user)로 검색
```
$ find . -user chang –print
```

- 파일 크기(-size)로 검색
```
$ find . -size +1024 -print
```

- 파일 종류(-type)로 검색
```
d : 디렉터리 f: 일반 파일 l: 심볼릭 링크
b: 블록 장치 파일 c: 문자 장치 파일 s: 소켓 파일
```
```
$ find ~ -type d –print
```

- 파일의 접근권한(-perm)으로 검색
```
$ find . -perm 700 -ls
```

- 파일의 접근 시간(-atime) 혹은 수정 시간(-mtime)으로 검색
```
+n: 현재 시각을 기준으로 n일 이상 전
n: 현재 시각을 기준으로 n일 전
-n: 현재 시각을 기준으로 n일 이내
```
```
$ find . -atime +30 -print
```
```
$ find . -mtime -7 -print
```

## find 명령어: 검색 조건 조합 
- find 명령어는 여러 검색 옵션을 조합해서 사용할 수 있다

- 예 
```
$ find . -type d -perm 700 -print
```
```
$ find . -name core -size +2048 -ls
```

## find 명령어: 검색된 파일 처리
- find 명령어의 -exec 옵션
  - 검색한 모든 파일을 대상으로 동일한 작업(명령어)을 수행

- 예
```
$ find . -name core -exec rm -i {} \;
```
```
$ find . -name “*.c” -atime +30 -exec ls -l {} \;
```

# 파일 필터링 
## grep 명령어 
- 사용법
```
$ grep 패턴 파일*
```
```
파일(들)을 대상으로 지정된 패턴의 문자열을 검색하고, 해당 문자열을 포함하는 줄들을 출력한다
```

![image](https://github.com/user-attachments/assets/68b68cc2-faee-4158-949a-f42e56db08d9)

```
$grep with you.txt
```
```
Until you come and sit awhile with me
There is no life - no life without its hunger;
But when you come and I am filled with wonder,
```
```
$grep -w with you.txt
```
```
Until you come and sit awhile with me
But when you come and I am filled with wonder,
```
```
$grep -n with you.txt 
```
```
4:Until you come and sit awhile with me
15:There is no life - no life without its hunger;
17:But when you come and I am filled with wonder,
```

```
$grep -i when you.txt
```
```
When I am down and, oh my soul, so weary
When troubles come and my heart burdened be
I am strong, when I am on your shoulders
But when you come and I am filled with wonder,
```
```
$grep -v raise you.txt
```
```
When I am down and, oh my soul, so weary
When troubles come and my heart burdened be
Then, I am still and wait here in the silence
Until you come and sit awhile with me
I am strong, when I am on your shoulders
There is no life - no life without its hunger;
Each restless heart beats so imperfectly;
But when you come and I am filled with wonder,
Sometimes, I think I glimpse eternity
```

## grep 명령어의 옵션 
![image](https://github.com/user-attachments/assets/18b6da1c-db56-49e2-af0f-205df0b6167a)

## 정규식 
![image](https://github.com/user-attachments/assets/e760e5c0-91a9-41a9-8337-83d277c33524)

## 정규식 사용 예 
```
$ grep 'st..' you.txt
```
```
Then, I am still and wait here in the silence
You raise me up, so I can stand on mountains
You raise me up, to walk on stormy seas
I am strong, when I am on your shoulders
Each restless heart beats so imperfectly;
```
```
$ grep 'st.*e' you.txt
```
```
Then, I am still and wait here in the silence
You raise me up, to walk on stormy seas
I am strong, when I am on your shoulders
Each restless heart beats so imperfectly;
```
```
$ grep –w 'st.*e' you.txt
```
```
Then, I am still and wait here in the silence
```

## 파이프와 함께 grep 명령어 사용
- 파이프와 함께 grep 명령어 사용
  - 어떤 명령어를 실행하고 그 실행 결과 중에서 원하는 단어 혹은 문자열 패턴을 찾고자 할 때 사용함.

- 예
```
$ ls -l | grep chang
```
```
$ ps –ef | grep chang
```

# 파일 정렬

## 정렬: sort 명령어 
- 사용법 
``` 
$ sort [-옵션] 파일*
```
```
텍스트 파일(들)의 내용을 줄 단위로 정렬한다. 옵션에 따라 다양한 형태로 정렬한다
```

- 정렬 방법
  - 정렬 필드를 기준으로 줄 단위로 오름차순으로 정렬한다.
  - 기본적으로는 각 줄의 첫 번째 필드가 정렬 필드로 사용된다.
  - -r 옵션을 사용하여 내림차순으로 정렬할 수 있다

## sort 명령어 예 
```
$ sort you.txt
```
```
But when you come and I am filled with wonder,
Each restless heart beats so imperfectly;
I am strong, when I am on your shoulders
Sometimes, I think I glimpse eternity
Then, I am still and wait here in the silence
There is no life - no life without its hunger;
Until you come and sit awhile with me
When I am down and, oh my soul, so weary
When troubles come and my heart burdened be
You raise me up, so I can stand on mountains
You raise me up, to more than I can be
You raise me up, to walk on stormy seas
```

```
$ sort -r you.txt
```
```
You raise me up, to walk on stormy seas
You raise me up, to more than I can be
You raise me up, so I can stand on mountains
When troubles come and my heart burdened be
When I am down and, oh my soul, so weary
Until you come and sit awhile with me
There is no life - no life without its hunger;
Then, I am still and wait here in the silence
Sometimes, I think I glimpse eternity
I am strong, when I am on your shoulders
Each restless heart beats so imperfectly;
But when you come and I am filled with wonder,
```

## 정렬 필드 지정 
![image](https://github.com/user-attachments/assets/148eaabe-7dcf-4327-aed8-9d26f7e0ce4d)


## 정렬 필드 지정 예 
```
$ sort –k 3 you.txt 혹은 sort +2 –3 you.txt
```
```
Then, I am still and wait here in the silence
When I am down and, oh my soul, so weary
Until you come and sit awhile with me
When troubles come and my heart burdened be
Each restless heart beats so imperfectly;
You raise me up, so I can stand on mountains
You raise me up, to more than I can be
You raise me up, to walk on stormy seas
There is no life - no life without its hunger;
I am strong, when I am on your shoulders
Sometimes, I think I glimpse eternity
But when you come and I am filled with wonder,
```

## sort 명령어의 옵션 
![image](https://github.com/user-attachments/assets/a9470eb5-c7e4-4548-807d-110f328fc473)

## sort 명령어의 옵션 예
- -o 출력파일 옵션
  - 정렬된 내용을 지정된 파일에 저장할 수 있다.
```
$ sort –o sort.txt you.txt
```
- -n 옵션
  - 숫자 문자열의 경우에 숫자가 나타내는 값의 크기에 따라 비교하여 정렬할 수 있다.
  - 예: “49”와 “100

## 필드 구분 문자 지정 
```
$ sort –t: -k 3 -n /etc/passwd
```
```
root:x:0:0:root:/root:/bin/bash
bin:x:1:1:bin:/bin:/sbin/nologin
daemon:x:2:2:daemon:/sbin:/sbin/nologin
adm:x:3:4:adm:/var/adm:/sbin/nologin
lp:x:4:7:lp:/var/spool/lpd:/sbin/nologin
sync:x:5:0:sync:/sbin:/bin/sync
shutdown:x:6:0:shutdown:/sbin:/sbin/shutdown
halt:x:7:0:halt:/sbin:/sbin/halt
mail:x:8:12:mail:/var/spool/mail:/sbin/nologin
operator:x:11:0:operator:/root:/sbin/nologin
games:x:12:100:games:/usr/games:/sbin/nologin
ftp:x:14:50:FTP User:/var/ftp:/sbin/nologin
...
```

# 파일 비교 
## 파일 비교: cmp 명령어 
- 사용법
```
$ cmp 파일1 파일2
```
```
파일1과 파일2가 같은지 비교한다
```
- 출력
  - 두 파일이 같으면 아무 것도 출력하지 않음
  - 두 파일이 서로 다르면 서로 달라지는 위치 출력
- 예 
```
$ cmp you.txt me.txt
```
```
you.txt me.txt 다름: 340 자, 10 행
```

## 파일 비교: diff 
- 사용법

```
$ diff [-i] 파일1 파일2
```
```
파일1과 파일2를 줄 단위로 비교하여 그 차이점을 출력한다.
-i 옵션은 대소문자를 무시하여 비교한다
```

- 출력
  - 첫 번째 파일을 두 번째 파일 내용과 같도록 바꿀 수 있는 편집 명령어 형 

## diff 출력: 편집 명령어 
- 추가(a)
  - 첫 번째 파일의 줄 n1 이후에 두 번째 파일의 n3부터 n4까지의 줄들을 추가하면 두 파일은 서로 같다

n1 a n3,n4
> 추가할 두 번째 파일의 줄들

- 예
```
$ diff you.txt me.txt
```
```
9a10,13
>
> You raise me up, so I can stand on mountains
> You raise me up, to walk on stormy seas
> I am strong, when I am on your shoulders
```

## diff 출력: 편집 명령어
- 삭제(d)
  - 첫 번째 파일의 n1부터 n2까지의 줄들을 삭제하면 두 번째 파일의 줄 n3 이후와 서로 같다 

n1,n2 d n3
< 삭제할 첫 번째 파일의 줄들 

- 예 
```
$ diff me.txt you.txt
```
```
10,13d9
<
< You raise me up, so I can stand on mountains
< You raise me up, to walk on stormy seas
< I am strong, when I am on your shoulders
```

## diff 출력: 편집 명령어 
- 변경(c)
  - 첫 번째 파일의 n1부터 n2까지의 줄들을 두 번째 파일의 n3부터 n4까지의 줄들로 대치하면 두 파일은 서로 같다.
n1,n2 c n3,n4
< 첫 번째 파일의 대치될 줄들
--
> 두 번째 파일의 대치할 줄들

- 예
```
$ diff 파일1 파일2
```
```
1 c 1
< This is the first file
--
> This is the second file
```

# 기타 파일 조작  

## 파일 분할 : split 
- 사용법 
```
$ split [-l n] 입력파일 [출력파일]
```
```
하나의 입력파일을 일정한 크기의 여러 개 작은 파일들로 분할한다. -l n 옵션을 이용하여 분할할 줄 수를 지정할 수 있다
```
- 1000줄씩 분할하여 xaa, xab, ... 형태의 파일명으로 저장

- 예
```
$ split -l 10 you.txt
```
```
$ ls -l
```
```
-rw-r--r-- 1 chang faculty 341 2월 16일 14:36 xaa
-rw-r--r-- 1 chang faculty 177 2월 16일 14:36 xab
-rw-r--r-- 1 chang faculty 518 2월 15일 19:33 you.txt
```

## 파일 합병: cat 
- cat 명령어를 이용한 파일 합병
```
$ cat 파일1 파일2 > 파일3
```
```
파일1과 파일2의 내용을 붙여서 새로운 파일3을 만들어 준다
```

- 예
```
$ cat xaa xab > xmerge
```

## 파일 합병: paste
- paste 명령어를 이용한 줄 단위 파일 합병 
```
$ paste [ -s ][ -d구분문자 ] 파일*
```
```
여러 파일들을 줄 단위로 합병하여 하나의 파일을 만들어 준다.
-s : 한 파일 끝에 다른 파일 내용을 덧붙인다
```

- 예
```
$ paste -s xaa xab > xmerge
```

## 파일 합병: paste 예 
```
line.txt
```
```
line 1:
line 2:
...
line 13:
line 14:
```
```
$ paste line.txt you.txt
```
```
line 1: When I am down and, oh my soul, so weary
line 2: When troubles come and my heart burdened be
...
line 13: But when you come and I am filled with wonder,
line 14: Sometimes, I think I glimpse eternity
```
```
$ paste line.txt you.txt > lineyou.txt
```

# 핵심 개념 
- find 명령어는 파일 이름이나 속성을 이용하여 해당하는 파일을 찾는 데 사용된다
- grep 명령어는 파일들을 대상으로 지정된 패턴의 문자열을 검색하고, 해당 문자열을 포함하는 줄들을 출력한다
- sort 명령어는 텍스트 파일을 줄 단위로 정렬한다
- cmp 명령어는 두 파일이 같은지 비교한다
- diff 명령어는 두 파일이 서로 다른지 비교한다

# 명령어 스케줄링 
## 주기적 실행 cron 
- cron 시스템
  - 유닉스의 명령어 스케줄링 시스템으로 crontab 파일에 명시된 대로 주기적으로 명령을 수행한

- crontab 파일 등록법
```
$ crontab 파일
```
```
crontab 파일을 cron 시스템에 등록한다
```

- crontab 파일
  - 7개의 필드로 구성
  - 분 시 일 월 요일 [사용자] 명령


## 주기적 실행 cron 
- crontab 명령어
```
$ crontab -l [사용자]
```
```
사용자의 등록된 crontab 파일 리스트를 보여준다
```
```
$ crontab -e [사용자]
```
```
사용자의 등록된 crontab 파일을 수정 혹은 생성한다
```
```
$ crontab -r [사용자]
```
```
사용자의 등록된 crontab 파일을 삭제한다
```

## crontab 파일 예 
- chang.cron
![image](https://github.com/user-attachments/assets/38ad72c1-592e-417c-8a2f-325c7e82caab)

- 사용 예
```
$ crontab chang.cron
```
```
$ crontab -l
```
```
30 18 * * * rm /home/chang/tmp/*
```
```
$ crontab -r
```
```
$ crontab -l
```
```
no crontab for chang
```

- crontab 파일 예1
  - 0 * * * * echo “뻐꾹” >> /tmp/x
  - 매 시간 정각에 “뻐꾹” 메시지를 /tmp/x 파일에 덧붙인다.
- crontab 파일 예2
  - 20 1 * * * root find /tmp –atime +3 –exec rm –f {} \;
  - 매일 새벽 1시 20분에 3일간 접근하지 않은 /tmp 내의 파일을 삭제
- crontab 파일 예3
  - 30 1 * 2,4,6,8,10,12 3-5 /usr/bin/wall /var/tmp/message
  - 2개월마다 수요일부터 금요일까지 1시 30분에 wall 명령을 사용해서 시스템의 모든 사용자에게 메시지를 전송

## 한번 실행: at
- at 명령어
  - 미래의 특정 시간에 지정한 명령어가 한 번 실행되도록 한다
  - 실행할 명령은 표준입력을 통해서 받는

- 사용법
```
$ at [-f 파일] 시간
```
```
지정된 시간에 명령이 실행되도록 등록한다. 실행할 명령은 표준입력으로 받는다.
-f : 실행할 명령들을 파일로 작성해서 등록할 수도 있다
```

- 예
```
$ at 1145 jan 31
```
```
at> sort infile > outfile
at> <EOT>
```

- atq 명령어
  - at 시스템의 큐에 등록되어 있는 at 작업을 볼 수 있다.
- 사용 예
```
$ atq
```
```
Rank Execution Date Owner Job Queue Job Name
1st Jan 31, 2012 11:45 chang 1327977900.a a stdin
```

- at -r 옵션
```
$ at -r 작업번호
```
```
지정된 작업번호에 해당하는 작업을 제거한다
```
- 사용 예
```
$ at –r 1327977900.a
```

# 디스크 및 아카이브 
- 사용법
```
$ df 파일시스템*
```
```
파일 시스템에 대한 디스크 사용 정보를 보여준다
```

- 사용 
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

## 디스크 사용: du 
- 사용법
```
$ du [-s] 파일*
```
```
파일이나 디렉토리가 사용하는 디스크 사용량(블록 수)을 알려준다.
파일을 명시하지 않으면 현재 디렉터리의 사용 공간을 보여준다
```

- 사용 
```
$ du
```
```
208 ./사진
4 ./.local/share/nautilus/scripts
8 ./.local/share/nautilus
144 ./.local/share/gvfs-metadata
. . .
```
```
$ du –s -s(sum)
```
```
22164 .
```

## tar 아카이브 
- 아카이브
  - 백업 또는 다른 장소로의 이동을 위해 여러 파일들을 하나로 묶어놓은 묶음
  -  아카이브를 만들거나 푸는데 tar(tape archive) 명령어 사용

- tar의 역학
![image](https://github.com/user-attachments/assets/f69c8b05-23b1-40fc-ac8e-6ba8cdc248aa)

- tar 명령어
  - 옵션: c(create), v(verbose), x(extract), t(table of contents), f(file)

  - $ tar -cvf 타르파일 파일+
    - 여러 파일들을 하나의 타르파일로 묶는다. 보통 확장자로 .tar 사용
  - $ tar -xvf 타르파일
    - 하나의 타르파일을 풀어서 원래 파일들을 복원한다.
  - $ tar -tvf 타르파일
    - 타르파일의 내용을 확인한다

## tar 아카이브: 사용 예 
- 현재 디렉터리에 있는 모든 파일을 다른 곳으로 옮기기
```
$ tar -cvf src.tar *
```
```
… src.tar를 다른 곳으로 이동
```
```
$ tar -tvf src.tar
```
```
$ tar -xvf src.tar
```

# 파일 압축

## 파일 압축: gzip 
- gzip 명령어
![image](https://github.com/user-attachments/assets/869167bd-5400-45c4-921a-a59faa3487e3)

```
$ gzip [옵션] 파일*
```
```
파일(들)을 압축하여 .gz 파일을 만든다.
-d : 압축을 해제한다
-l : 압축파일 안에 있는 파일 정보(압축된 크기, 압축률) 출력한다
-r : 하위 디렉터리까지 모두 압축한다
-v : 압축하거나 풀 때 압축률, 파일명을 출력한다
```

- 사용법
```
$ gzip 파일*
```
```
$ gzip -d 파일.gz*
```
- 사용 방법
  - 파일들을 하나의 타르파일로 묶은 후 compress/gzip을 사용해 압축
  - 파일 복원: 압축을 해제한 후, 타르파일을 풀어서 원래 파일들을 복원

## 사용 예
- 사용 예
  - 파일들을 하나의 타르파일로 묶은 후 gzip을 사용해 압축
  - 파일 복원: 압축을 해제한 후, 타르파일을 풀어서 원래 파일들을 복원
```
$ tar -cvf src.tar *
```
```
$ gzip src.tar
```
```
… 이 파일을 원하는 곳으로 이동
```
```
$ gzip -d src.tar.gz
```
```
$ tar -xvf src.tar
```

![image](https://github.com/user-attachments/assets/cfedde7e-6a59-4d12-8481-53c34f58361e)


## 압축 풀기 
- 사용법
```
$ gzip –d 파일.gz*
```
```
gzip으로 압축된 파일들을 복원한다
```
```
$ gunzip 파일.gz*
```
```
gzip으로 압축된 파일들을 복원한다
```

## 파일 압축: compress 
- 명령어 compress/ uncompress 명령어
```
$ compress 파일*
```
```
파일(들)을 압축하여 .Z 파일을 만든다
```
```
$ uncompress 파일.Z*
```
```
압축된 파일(들)을 복원한다
```

- 사용 
```
$ ls -sl
```
```
5892 -rw-r--r-- 1 chang chang 6031360 10월 8 2012 src.tar
```
```
$ compress src.tar
```
```
$ ls -sl
```
```
1046 -rw-r--r-- 1 chang chang 1071000 10월 8 2012 src.tar.Z
```
```
$ uncompress src.tar.Z
```
```
$ ls
```
```
5892 -rw-r--r-- 1 chang chang 6031360 10월 8 2012 src.tar
```

# AWK
## AWK 
- AWK
  - 일반 스크립트 언어
  - AWK(Aho. Weinberger, Kernighan)
  - 텍스트 형태로 되어있는 각 줄을 필드로 구분하여 처리한다
  - 필드: 줄을 구성하는 단어
 
![image](https://github.com/user-attachments/assets/504cf2b8-49d7-4243-a7c3-5d9bdfc4e2ca)
 
- awk 프로그램
  - 간단한 프로그램은 명령줄에 직접 작성하여 수행
  - awk 프로그램을 파일로 작성하여 -f 옵션을 이용하여 수행
```
$ awk 프로그램 파일*
```
```
$ awk [-f 프로그램파일] 파일*
```
```
텍스트 파일을 대상으로 하여 각 줄을 필드들로 구분하고 이들을 awk 프로그램이 지시하는 대로 처리한다
```

## awk 프로그램 
- awk 프로그램
  - 조건과 액션을 기술하는 명령어들로 구성됨
  - [ 조건 ] [ { 액션 } ]
  - 대상 파일의 각 줄을 스캔하여 조건을 만족하는 줄에 액션 수행
- 간단한 awk 프로그램 예
```
$ awk ‘{ print NF, $0 }’ you.txt
```
```
$ awk ‘{ print $1, $3, $NF }’ you.txt
```
```
$ awk ‘NR > 1 && NR < 4 { print NR, $1, $3, $NF }’ you.txt
```

## 조건(condition)
- 조건에서 사용 가능한 연산자 및 패턴 
  - BEGIN
    파일 시작
  - END
    파일 끝
  - 관계 연산자 혹은 논리 연산자를 포함한 조건식

   - /패턴/
    패턴에 해당하는 줄
  - /패턴1/, /패턴2/
    패턴1을 포함한 줄부터 패턴2를 포함한 줄까지

## 액션(action) 
- 액션에서 사용 가능한 문장
  - if (조건) 실행문 [else 실행문]
  - while (조건) 실행문
  - for (식; 조건; 식) 실행문
  - break
  - continue
  - 변수 = 식
  - print [식들의 리스트]
  - printf 포맷 [, 식들의 리스트]
  - next
    현재 줄에 대한 나머지 패턴 건너뛰기
  - exit
    현재 줄의 나머지 부분 건너뛰기
  - { 실행문 리스트 }

## 연산자 
- 액션에서 사용 가능한 연산자(C 언어 연산자)
  - 산술 연산자: +, -, *, /, %, ++, --
  - 대입 연산자: =,+=, -=, *=, /=, %=
  - 조건 연산자: ? :
  - 논리 연산자: ||, &&, !
  - 패턴 비교 연산자: ~, !~
  - 비교 연산자: <, <=, >, >=, !=, ==
  - 필드참조 연산자: $

- 간단한 AWK 프로그램 예
- $ awk 'END { print NR }' 파일명
- $ awk 'NR % 2 == 0 { print NR, $0 }' 파일명
- $ awk 'NF > 5{ print NR, $0}' 파일명
- $ awk '/raise/ { print NR, $0 }' 파일명
- $ awk '/the?/ { print NR, $0 }' 파일명
- $ awk '/a..e/ { print NR, $0 }' 파일명
- $ awk '/a.*e/ { print NR, $0 }' 파일명
- $ ls -l | awk '{x += $5}; END {print x}'

# AWK 프로그램 작성 
## AWK 프로그램 예 
- [예제 1]
```
BEGIN { print "파일 시작:", FILENAME }
{ print $1, $NF }
END { print "파일 끝" }
```
- [예제 2] 줄 수/단어 수 계산
```
BEGIN { print "파일 시작" }
{
printf "line %d: %d \n", NR, NF;
word += NF
}
END { printf "줄 수 = %d, 단어 수 = %d\n", NR, word }
```
- [예제 3]
```
{
for (I = 1; I <= NF; I += 2)
printf "%s ", $I
printf " \n"
}
```
- [예제 4]
```
/st.*e/ {print NR, $0 }
```
- [예제 5]
```
/strong/, /heart/ { print NR, $0 }
```

- [예제 6]
```
/raise/ { ++line }
END { print line }
```
- [예제 7] 단어별 출현 빈도수 계산
```
BEGIN {
FS="[^a-zA-Z]+"
}
{
for (i=1; i<=NF; i++)
words[tolower($i)]++ 
}
END {
for (i in words)
print i, words[i]
}
```
  - 단어를 인덱스로 사용하는 연관 배열 사용

- [예제 8] wc 구현
```
BEGIN { print "파일 시작" }
{
printf "line %d: %d %d\n", NR, NF, length($0);
word += NF;
char += length($0)
}
END { printf "줄 수 = %d, 단어 수 = %d, 문자 수 = %d\n", NR,
word, char }
```

## awk 내장 함수 
- 문자열 함수
  - index(s1, s2), length([s]), match(s, r), sub(r, s), tolower(s), toupper(s), …
- 입출력 함수
  - getline, next, print, printf, system …
- 수학 함수
  - atan2(x,y), cos(x), sin(x), exp(arg), log(arg)
  - sqrt(arg), int(arg), rand(), srand(expr)

# 핵심 개념 
- cron은 유닉스의 명령어 스케줄링 시스템으로 crontab 파일에 명시된 대로 주기적으로 명령을 수행한다
- 유닉스에서는 tar 명령어를 사용하여 여러 파일을 하나로 묶은 후에 compress 혹은 gzip 명령어를 이용하여 압축한다
- awk 프로그램은 조건과 액션을 기술하는 명령어들로 구성되며 텍스트 파일의 줄들을 스캔하여 조건을 만족하는 각 줄에 대해 액션을 수행한다

# Bash 쉘 소개 

## Bash(Borune-again shell)
- 리눅스, 맥 OS X 등의 운영 체제의 기본 쉘
- Bash 문법은 본 쉘의 문법을 대부분 수용하면서 확장
- 시작 파일(start-up file)
  - /etc/profile
    - 전체 사용자에게 적용되는 환경 설정, 시작 프로그램 지정
  - /etc/bash.bashrc
    - 전체 사용자에게 적용되는 별명과 함수들을 정의
  - ~/.bash_profile
    - 각 사용자를 위한 환경을 설정, 시작 프로그램 지정
  - ~/.bashrc
    - 각 사용자를 위한 별명과 함수들을 정의 

## Bash 시작 과정
![image](https://github.com/user-attachments/assets/9dea07e2-ed69-4154-b34d-8cb1ef6bb23d)

## 시작 파일 예: .bash_profile
```
# .bash_profile
```
```
# 사용자의 환경변수 설정 및 시작 프로그램
```
```
if [ -f ~/.bashrc ]
then
. ~/.bashrc
fi
```
```
PATH=$PATH:$HOME/bin:.
BASH_ENV=$HOME/.bashrc
USERNAME=“chang"
export USERNAME BASH_ENV PATH 
```

## 시작 파일 예: .bashrc
```
1 # .bashrc
2 # 사용자 시작 파일
3 # 히스토리 길이 설정
4 HISTSIZE=1000
5 HISTFILESIZE=2000
6
7 # 사용자의 별명 설정
8 alias rm='rm-i’
9 alias cp='cp -i'
10 alias mv='mv-i'
11 alias ls=’ls --color=auto’
12 alias grep=’grep —color=auto’
13 alias ll='ls -al --color=yes' 
```

# 별명 및 히스토리 기능 
## 별명 
- alias 명령어
  - 문자열이 나타내는 기존 명령에 대해 새로운 이름을 별명으로 정의
```
$ alias 이름=문자열
```
```
$ alias dir='ls –aF'
```
```
$ dir
```
```
$ alias h=history
```
```
$ alias ll='ls –l‘
```
- 현재까지 정의된 별명들을 확인
```
$ alias # 별명 리스트
alias dir='ls –aF‘
alias h=history
alias ll='ls –l'
```
- 이미 정의된 별명 해제
```
$ unalias 단어
```

## 히스토리 
- 입력된 명령들을 기억하는 기능
```
$ history [-rh] [번호]
```

- 기억할 히스토리의 크기
```
$ HISTSIZE=100
```

- 로그아웃 후에도 히스토리가 저장되도록 설정
```
$ HISTFIESIZE=100
```

![image](https://github.com/user-attachments/assets/52bbd0d6-5bc8-4682-bd78-20e6af80e992)

## 재실행 
![image](https://github.com/user-attachments/assets/91b82241-77b1-4e06-b43f-1fff86841c7c)

- 예
```
$ !! # 바로 전 명령 재실행
```
```
$ !20 # 20번 이벤트 재실행
```
```
$ !gcc # gcc로 시작하는 최근 명령 재실행
```
```
$ !?test.c # test.c를 포함하는 최근 명령 재실행
```

# 변수 
## 단순 변수 (simple variable)
- 하나의 값(문자열)만을 저장할 수 있는 변수
```
$ 변수이름=문자열
```
```
$ city=seoul
```

- 변수의 값 사용
```
$ echo $city
```
```
seoul
```
- 변수에 어느 때나 필요하면 다른 값을 대입
```
$ city=pusan
```
- 한 번에 여러 개의 변수를 생성
```
$ country=korea city=seoul
```

## 단순 변수 
- 한글 문자열을 값으로 사용
```
$ country=대한민국 city=서울
```
```
$ echo $country $city
```
대한민국 서울
- 따옴표를 이용하여 여러 단어로 구성된 문자열 저장 가능
```
$ address="서울시 용산구" 
```

## 리스트 변수(list variable)
- 한 변수에 여러 개의 값(문자열)을 저장할 수 있는 변수
```
$ 이름=( 문자열리스트 )
```
```
$ cities=(서울 부산 목포)
```
- 리스트 변수 사용
![image](https://github.com/user-attachments/assets/e19a70de-85c3-414d-96a7-07c299f87bcc)


## 리스트 변수 사용 예
- 리스트 변수 사용
```
$ echo ${cities[*]}
```
```
서울 부산 목포
```
```
$ echo ${cities[1]}
```
```
부산
```
- 리스트의 크기
```
$ echo ${#cities[*]} # 리스트 크기
```
```
3
```
```
$ echo ${cities[3]}
```
- 리스트 변수에 새로운 도시 추가
```
$ cities[3]=제주
```
```
$ echo ${cities[3]}
```
```
제주
```

## 표준입력 읽기
- read 명령어
  - 표준입력에서 한 줄을 읽어서 단어들을 변수들에 순서대로 저장
  - 마지막 변수에 남은 문자열을 모두 저장
```
$ read 변수1 ... 변수n
```

```
$ read x y
```
```
Merry Christmas !
```
```
$ echo $x
```
```
Merry
```
```
$ echo $y
```
```
Christmas !
```

- 변수를 하나만 사용
```
$ read x
```
```
Merry Christmas !
```
```
$ echo $x
```
```
Merry Christmas !
```

# 지역변수와 환경변수 
## 환경변수와 지역변수 
- 쉘 변수
  - 환경변소와 지역변수 두 종류로 나눌 수 있다
  - 환경 변수는 값이 자식 프로세스에게 상속되며 지역 변수는 그렇지 않다

![image](https://github.com/user-attachments/assets/abbacb65-934a-4799-a83c-c5625ee19e21)

## 환경변수와 지역변수 예
```
$ country=대한민국 city=서울
```
```
$ export country
```
```
$ echo $country $city
```
```
대한민국 서울
```
```
$ bash # 자식 쉘 시작
```
```
$ echo $country $city
``` 
```
대한민국
```
```
$ ^D # 자식 쉘 끝
``` 
```
$ echo $country $city
```
```
대한민국 서울
```

## 사전 정의 환경변수(predefined environment variable) 
- 그 의미가 미리 정해진 환경변수들 
![image](https://github.com/user-attachments/assets/aa171a7d-e8e0-4500-93b4-2b810a842777)

```
$ echo 홈 = $HOME 사용자 = $USER 쉘 = $SHELL
```
```
홈 = /user/faculty/chang 사용자 = chang 쉘 = /bin/bash
```
```
$ echo 터미널 = $TERM 경로 리스트 = $PATH
```
```
터미널 = xterm 경로 리스트 = /bin:/usr/bin:/usr/local/bin
```

## 사전 정의 지역 변수(predefined local variable)
```
#!/bin/bash
```
```
# builtin.bash
```
```
echo 이 스크립트 이름: $0
echo 첫 번째 명령줄 인수: $1
echo 모든 명령줄 인수: $*
echo 이 스크립트를 실행하는 프로세스 번호: $$
```
```
$ builtin.bash hello shell
```
```
이 스크립트 이름: builtin.sh
첫 번째 명령줄 인수: hello
모든 명령줄 인수: hello shell
이 스크립트를 실행하는 프로세스 번호: 1259
```
![image](https://github.com/user-attachments/assets/3d458e17-14d0-4411-b9b8-7480308334a3)

# Bash 쉘 스크립

## Bash 스크립트 작성 및 실행 과정
- (1) 에디터를 사용하여 Bash 스크립트 파일을 작성한다.
```
#!/bin/bash
```
```
# state.bash
```
```
echo -n 현재 시간:
date
echo 현재 사용자:
who
echo 시스템 현재 상황:
uptime
```
- (2) chmod를 이용하여 실행 모드로 변경한다 
```
$ chmod +x state.bash
```
- (3) 스크립트 이름을 타입핑하여 실행한다 
```
$ state.bash
```

## if 문 
- if 문
```
if 조건식
then
명령들
fi
```
- 조건식
```
[수식]
```

- 예
```
if [ $#-eq 1 ]
then
  wc $1
fi
```

![image](https://github.com/user-attachments/assets/a315b776-ffef-4b1a-a696-66e5d8da0a7c)

## if-then-else
- if-then-else 구문
```
if 조건식
then
명령들
else
명령들
fi
```

![image](https://github.com/user-attachments/assets/2f9d2995-9c31-450f-88e1-46e7efbd1415)

# 수식 
## 비교 연산 
- 비교 연산은 산술 비교 연산, 문자열 비교 연산 
![image](https://github.com/user-attachments/assets/7c500be6-66f2-4d00-b2ad-bd3ef4c6d763)

## 문자열 비교 연산
![image](https://github.com/user-attachments/assets/cbedfd8b-0651-4086-9729-f2e90847fae8)

![image](https://github.com/user-attachments/assets/608296b3-3e21-4079-bdb5-9381bca956e3)

## 파일 관련 연산 
![image](https://github.com/user-attachments/assets/c5e1425a-61ad-4b6e-9758-3db8b94b7fef)

## 파일 관련 연산: 예
```
if [ -e $file ]
then # $file이 존재하면
wc $file
else # $file이 존재하지 않으면
echo "오류 ! 파일 없음“
fi

```

```
if [ -d $dir ]
then
echo -n $dir 내의 파일과 서브디
렉터리 개수:
ls $dir | wc -l
else
echo $dir\: 디렉터리 아님
fi
```

## 부울 연산자 
- 조건식에 부울 연산자 사용 
  - ! 부정(negation)
  - && 논리곱(logical and)
  - || 논리합(logical or)

![image](https://github.com/user-attachments/assets/5645752f-0ff5-4374-872c-7d72e6e8af28)

## 산술 연산 
- 산술 연산
```
$ a=2+3
```
```
$ echo $a
```
```
$ a=`expr 2 + 3`
```
- let 명령어를 이용한 산술연산
```
$ let 변수=수식
$ let a=2*3
$ echo $a
6
$ let a=$a+2
$ echo $a
8
$ let a*=10
$ let b++
```

## 변수 타입 선언 
- 변수 타입 선언: declare
![image](https://github.com/user-attachments/assets/9caba08a-3edc-46f3-96c0-e5e1c52e0089)

![image](https://github.com/user-attachments/assets/34137c39-3951-4fe4-ba36-08e1e684ce40)

# 조건문 

## Bash 제어구조
- 조건
if
- 스위치
case
- 반복
for, while 

## 조건문
![image](https://github.com/user-attachments/assets/4ed27ce1-7f94-4a71-a1a7-0b151af77690)

- 중첩 조건문
![image](https://github.com/user-attachments/assets/e0d1125a-c1bf-4ffb-bd39-8ff1d0c05d5d)

## 새로운 조건식 
- 새로운 조건
if ((수식))
…

- 예
![image](https://github.com/user-attachments/assets/b493e3cb-d981-411f-b557-0c5c5ff65ad8)


## 산술 연산자 
![image](https://github.com/user-attachments/assets/99431a5a-67f5-46c4-95fa-2b4ab88e660b)

## 중첩 조건문: 
```
#!/bin/bash
```
```
# 사용법: score1.bash
```
```
# 점수에 따라 학점을 결정하여 프린트
```
```
echo -n '점수 입력: '
read score
if (( $score >= 90 ))
then
echo A
elif (( $score >= 80 ))
then
echo B
elif (( $score >= 70 ))
then
echo C
else
echo 노력 요함
fi
```
```
$score1.bash
```
```
점수 입력: 85 
B
```

## 스위치 
![image](https://github.com/user-attachments/assets/8018ebf0-4a24-4f24-ad55-401b224b2af9)

![image](https://github.com/user-attachments/assets/e853b7ff-aeb1-4e20-86db-57798fdb9baf)

# 반복문 

## 반복문: for 
- for 구문
  - 리스트의 각 값에 대해서 명령어들을 반복  

![image](https://github.com/user-attachments/assets/13840ba3-32b9-485e-902c-cdeb8ff5af77)

![image](https://github.com/user-attachments/assets/a7fc24e0-ef08-4048-811d-add2538f9477)

## 모든 명령줄 인수 처리 
- 모든 명령줄 인수 처리 
![image](https://github.com/user-attachments/assets/8111e200-58ed-439e-be12-be54718bb4f1)

![image](https://github.com/user-attachments/assets/45ba7afe-a9af-45fa-a134-160c80e29f5a)

## 반복문: while 
- while 문
  - 조건에 따라 명령어들을 반복적으로 실행  

![image](https://github.com/user-attachments/assets/85b0bcb8-b696-4507-a737-c773ea4abc60)

![image](https://github.com/user-attachments/assets/42dafc4a-b877-45d3-b4b6-ba391ec029fc)

## menu.bash 
![image](https://github.com/user-attachments/assets/c8b3a604-f90d-4c7b-ae84-12a202008eda)

![image](https://github.com/user-attachments/assets/d78546d7-b1fa-425d-ac9b-631653dde18f)

![image](https://github.com/user-attachments/assets/d344f103-0431-42c4-9980-730996bae14b)

# 고급 기능 

## 함수 
- 함수 정의
![image](https://github.com/user-attachments/assets/77bd4dba-f77b-4baa-a5b3-a672dd456773)

- 함수 호출 
![image](https://github.com/user-attachments/assets/b4f70484-16c1-473d-a078-5e866dfedd31)

![image](https://github.com/user-attachments/assets/35a77a15-52db-440b-a38e-7302c4785475)

## 함수 
```
$lshead.bash
```
```
안녕하세요
함수 시작, 매개변수 /tmp
2022. 02. 23. (수) 17:43:27 KST
디렉터리 /tmp 내의 처음 3개 파일만 리스트
총 1184
-rw------- 1 chang faculty 11264 2009년 3월 28일 Ex01378
-rw------- 1 chang faculty 12288 2011년 5월 8일 Ex02004
-rw------- 1 root other 8192 2011년 5월 4일 Ex02504
```

## 디버깅 
```
$ bash -vx 스크립트 [명령줄 인수]
```
```
$ bash -v menu.bash
```
```
#!/bin/bash
echo 명령어 메뉴
명령어 메뉴
cat << MENU
d : 날짜 시간
l : 현재 디렉터리 내용
w : 사용자 보기
q : 끝냄
MENU
d : 날짜 시간
l : 현재 디렉터리 내용
w : 사용자 보기
q : 끝냄
stop=0
while (($stop == 0))
do
echo -n '? '
read reply
case $reply in
"d") date;;
"l") ls;;
"w") who;;
"q") stop=1;;
*) echo 잘못된 선택;;
esac
done
? d
2023년 … 17:43:27 KST
? q
```

## shift 
- shift 명령어
  - shift [리스트변수]
  - 명령줄 인수[리스트 변수] 내의 원소들을 하나씩 왼쪽으로 이동 
```
#!/bin/bash
# 사용법: perm2.bash 파일*
# 파일의 사용권한과 이름을 프린트
if [ $# -eq 0 ]
then
echo 사용법: $0 files
exit 1
fi
echo " 허가권 파일"
while [ $# -gt 0 ]
do
file=$1
if [ -f $file ]
then
fileinfo=`ls -l $file`
perm=`echo "$fileinfo" |
cut -d' ' -f1`
echo "$perm $file"
fi
shift
done
```

## 디렉토리 내의 모든 파일 처리 
- 디렉터리 내의 모든 파일 처리
  - 해당 디렉터리로 이동
  - for 문과 대표 문자 *를 사용
  - 대표 문자 *는 현재 디렉터리 내의 모든 파일 이름들로 대 
```
cd $dir
for file in *
do
echo $file
done
```

## 디렉터리 내의 모든 파일 처리: 예 
```
#!/bin/bash
# 사용법: count2.bash [디렉터리]
# 대상 디렉터리 내의 파일, 서브디렉터리, 기타 개수를 세서 프린트
if [ $# -eq 0 ]
then
dir="."
else
dir=$1
fi
if [ ! -d $dir ]
then
echo $0\: $dir 디렉터리 아님
exit 1
fi
let fcount=0
let dcount=0
let others=0
echo $dir\:
cd $dir
for file in *
do
if [ -f $file ]
then
let fcount++
elif [ -d $file ]
then
let dcount++
else
let others++
fi
done
echo 파일: $fcount 디렉터리: $dcount 기타: $others
```

## 리커전(recursion)
- 스크립트도 자기 자신을 호출 가능
- 어떤 디렉터리의 모든 하위 디렉터리에 대해 동일한 작업을 수행할 때 매우 유용함 
```
#!/bin/bash
# 사용법 lssr.bash [디렉터리]
# 대상 디렉터리와 모든 하위 디렉터리 내에 있는 파일들의 크기를 리스트한다
```
```
if [ $# -eq 0 ]
then
dir="."
else
dir=$1
fi
if [ ! -d $dir ]
then
echo $0\: $dir 디렉터리 아님
exit 1
fi
cd $dir
echo -e "\n $dir :"
ls -s
for x in *
do
if [ -d $x ]
then
/home/chang/bash/lssr.bash $x
fi
done
```

## 터미널에서 실행 
- 터미널에서 while 혹은 for문도 실
```
$ for f in *
```
```
> do
> echo $f
> done
```
```
$ let i=2
$ let j=1
$ while (( $j <= 10 ))
> do
> echo '2 ^' $j = $i
> let i*=2
> let j++
> done
2 ^ 1 = 2
2 ^ 2 = 4
…
2 ^ 10 = 1024
```

# 핵심 개념 
- 단순 변수는 하나의 값(문자열)을 리스트 변수는 여러 개의 값(문자열)을 저장할 수 있다
- 쉘 변수는 크게 환경변수와 지역변수 두 종류로 나눌 수 있다 환경 변수는 값이 자식 프로세스에게 상속되며 지역변수는 그렇지 않다
- Bash 쉘은 조건, 스위치, 반복 등을 위한 제어구조로 if, case, for, while 등의 문장을 제공한다
- Bash 쉘의 식은 비교 연산, 파일 관련 연산, 산술 연산 등을 할 수 있

# 프로그램 작성과 컴파일 
## gedit 문서편집기
- GNU의 대표적인 GUI 텍스트 편집기
- GNOME 환경의 기본 편집기
  - 텍스트, 프로그램 코드, 마크업 언어 편집에 적합
  - 깔끔하고 단순한 GUI

- gedit 실행 방법
  - 메인 메뉴
    - [프로그램] -> [보조 프로그램] -> [지에디트] 선택
  - 터미널
    - $ gedit [파일이름] &
- 파일 관리자:
  - 텍스트 파일 클릭하면 자동실행

![image](https://github.com/user-attachments/assets/d1f7d01e-6031-403d-a86a-5a32a71fa444)

## gedit 메뉴
- 파일
  - 새로 만들기, 열기, 저장, 되돌리기, 인쇄
- 편집
  - 입력 취소, 다시 실행, 잘라내기, 복사, 붙여넣기, 삭제
- 보기
  - 도구모음, 상태표시줄, 전체화면, 강조 모드
- 검색
  - 찾기, 바꾸기, 줄로 이동
- 도구
  - 맞춤법 검사, 오타가 있는 단어 강조, 언어 설정, 문서 통계
- 문서
  - 모두 저장, 모두 닫기, 새 탭 그룹, 이전 문서

## 단일 모듈 프로그램 
- 프로그램 작성
  - gedit 이용
- [보기] 메뉴
  - C 구문 강조 기능 설정
- 프로그램 편집하는 화면
  - #include 같은 전처리 지시자는 황색
  - 주석은 파란색
  - 자료형 이름은 초록색
  - if나 while 같은 문장 키워드는 브라운 색

![image](https://github.com/user-attachments/assets/9019ef79-852c-4d3b-915e-a1c3c5da40da)

## gcc 컴파일러 
- gcc(GNU cc) 컴파일러
```
$ gcc [-옵션] 파일
```
```
C 프로그램을 컴파일한다. 옵션을 사용하지 않으면 실행파일 a.out를 생성한다
```

- 간단한 컴파일 및 실행
```
$ gcc longest.c
```
```
$ a.out // 실행
```
- -c 옵션: 목적 파일 생성
```
$ gcc –c longest.c
```
- -o 옵션: 실행 파일 생성
```
$ gcc –o longest longest.o 혹은 $ gcc –o longest longest.c
```
- 실행
```
$ longest // 실행
```

## 단일 모듈 프로그램:longest.c 
```
#include <stdio.h>
#include <string.h>
#define MAXLINE 100
char line[MAXLINE]; // 입력 줄
char longest[MAXLINE]; // 가장 긴 줄
void copy(char from[], char to[]);
/*입력 줄 가운데 가장 긴 줄 프린트 */
int main()
{
int len, max = 0;
while(fgets(line,MAXLINE,stdin) !=
NULL) {
len = strlen(line);
if (len > max) {
max = len;
copy(line, longest);
}
}
if (max > 0) // 입력 줄이 있었다면
printf("%s", longest);
return 0;
}
/* copy: from을 to에 복사; to가 충분히 크다고 가정*/
void copy(char from[], char to[])
{
int i;
i = 0;
while ((to[i] = from[i]) != '\0')
++i;
}
```

## 다중 모듈 프로그램 
- 단일 모듈 프로그램
  - 코드의 재사용(reuse)이 어렵고,
  - 여러 사람이 참여하는 프로그래밍이 어렵다
  - 예를 들어 다른 프로그램에서 copy 함수를 재사용하기 힘들다

- 다중 모듈 프로그램
  - 여러 개의 .c 파일들로 이루어진 프로그램
  - 일반적으로 복잡하며 대단위 프로그램인 경우에 적합

![image](https://github.com/user-attachments/assets/b8ce502c-4491-465e-a05e-0f0192117351)

## 다중 모듈 프로그램: 예
- main 프로그램과 copy 함수를 분리하여 별도 파일로 작성
  - main.c
  - copy.c
  - copy.h // 함수의 프로토타입을 포함하는 헤더 파일

- 컴파일
```
$ gcc -c main.c
```
```
$ gcc -c copy.c
```
```
$ gcc -o main main.o copy.o
```
혹은 
```
$ gcc -o main main.c copy.c
```

## main.c 
```
#include <stdio.h>
#include <string.h>
#include "copy.h"
char line[MAXLINE]; // 입력 줄
char longest[MAXLINE]; // 가장 긴 줄
/*입력 줄 가운데 가장 긴 줄 프린트 */
int main()
{
int len, max = 0;
while (fgets(line,MAXLINE,stdin) != NULL) {
len = strlen(line);
if (len > max) {
max = len;
copy(line, longest);
}
}
if (max > 0) // 입력 줄이 있었다면
printf("%s", longest);
return 0;
}
```

## copy.c 
```
#include <stdio.h>
/* copy: from을 to에 복사; to가 충분히 크다고 가정*/
void copy(char from[], char to[])
{
int i;
i = 0;
while ((to[i] = from[i]) !='\0')
++i;
}
```
## copy.h
```
#define MAXLINE 100
void copy(char from[], char to[]);
```

# 자동 빌드 도구 
## make 시스템의 필요성 
- 다중 모듈 프로그램을 구성하는 일부 파일이 변경된 경우?
  - 변경된 파일만 컴파일하고, 파일들의 의존 관계에 따라서 필요한 파일만 다시 컴파일하여 실행 파일을 만들면 좋다

- 예
  - copy.c 소스 코드를 수정
  - 목적 파일 copy.o 생성
  - 실행파일을 생성

- make 시스템
- 대규모 프로그램의 경우에는 헤더, 소스 파일, 목적 파일, 실행 파일의 모든 관계를 기억하고 체계적으로 관리하는 것이 필요
- make 시스템을 이용하여 효과적으로 작업

## 메이크파일 
- 메이크파일
  - 실행 파일을 만들기 위해 필요한 파일들
  - 그들 사이의 의존 관계
  - 만드는 방법을 기술

- make 시스템
  - 메이크파일을 이용하여 파일의 상호 의존 관계를 파악하여 실행 파일을 쉽게 다시 만듬

- 사용법
```
$ make [-f 메이크파일]
```
```
make 시스템은 메이크파일(makefile 혹은 Makefile)을 이용하여 보통 실행 파일을 빌드한다. 옵션을 사용하여 별도의 메이크파일을 지정할 수 있다
```

## 마이크파일의 구성 
- 메이크파일의 구성 형식
  - 목표(target): 의존리스트(dependencies) 명령리스트(commands)

- 예: Makefile
```
main: main.o copy.o
gcc -o main main.o
copy.o
main.o: main.c copy.h
gcc -c main.c
copy.o: copy.c
gcc -c copy.c
```
- 의존 관계 그래프
![image](https://github.com/user-attachments/assets/dca89e04-d00b-4206-b7c1-4fe8c27eb808)

## 메이크파일의 구성 
- make 실행
```
$ make 혹은 $ make main 
gcc -c main.c
gcc -c copy.c
gcc -o main main.o copy.o
```
- copy.c 파일이 변경된 후
```
$ make
gcc -c copy.c
gcc -o main main.o copy.o
```
![image](https://github.com/user-attachments/assets/59b6924f-66ea-47b6-994c-937b3e7eff48)

# gdb 디버거  
## gdb
- 가장 대표적인 디버거
  - GNU debugger(gdb)

- gdb 주요 기능
  - 정지점(breakpoint) 설정
  - 한 줄씩 실행
  - 변수 접근 및 수정
  - 함수 탐색
  - 추적(tracing)
```
$ gdb [실행파일]
```
```
gdb 디버거는 실행파일을 이용하여 디버깅 모드로 실행한다
```
- gdb 사용을 위한 컴파일
  - -g 옵션을 이용하여 컴파일
```
$ gcc -g -o longest longest.c
```
  - 다중 모듈 프로그램
```
$ gcc -g -o main main.c copy.c
```
- gdb 실행
```
$ gdb [실행파일]
```
```
gdb 디버거는 실행파일을 이용하여 디버깅 모드로 실행한다
```

## gdb 기능
- 소스보기 : l(ist)
  - l [줄번호] 지정된 줄을 프린트
  - l [파일명]:[함수명] 지정된 함수를 프린트
  - set listsize n 출력되는 줄의 수를 n으로 변경
```
(gdb) l copy
1 #include <stdio.h>
2
3 /* copy: copy 'from' into 'to'; assume to is big enough */
4 void copy(char from[], char to[])
5 {
6 int i;
7
8 i = 0;
9 while ((to[i] = from[i]) != '\0')
10 ++i;
```
- 정지점 : b(reak), clear, d(elete)
  - b [파일:]함수 파일의 함수 시작부분에 정지점 설정
  - b n n번 줄에 정지점을 설정
  - b +n 현재 줄에서 n개 줄 이후에 정지점 설정
  - b -n 현재 줄에서 n개 줄 이전에 정지점 설정
  - info b 현재 설정된 정지점을 출력
  - clear 줄번호 해당 정지점을 삭제
  - d 모든 정지점을 삭제
```
(gdb) b copy
Breakpoint 1 at 0x804842a: file copy.c, line 9.
(gdb) info b
Num Type Disp Enb Address What
1 breakpoint keep y 0x0804842a in copy at copy.c:9
```
- 프로그램 수행
  - r(un) 인수 명령줄 인수를 받아 프로그램 수행
  - k(ill) 프로그램 수행 강제 종료
  - n(ext) 멈춘 지점에서 다음 줄을 수행하고 멈춤
  - s(tep) n과 같은 기능 함수호출 시 함수내부로 진입
  - c(ontinue) 정지점을 만날 때 까지 계속 수행
  - u 반복문에서 빠져나옴
  - finish 현재 수행하는 함수의 끝으로 이동
  - return 현재 수행중인 함수를 빠져나옴
  - quit 종료
```
(gdb) r
Starting program: /home/chang/바탕화면/src/long
Merry X-mas !
Breakpoint 1, copy (from=0x8049b60 "Merry X-mas !", to=0x8049760 "")
at copy.c:8
8 i = 0;
```
- 변수 값 프린트: p(rint)
  - p [변수명] 해당 변수 값 프린트
  - p 파일명::[변수명] 특정 파일의 전역변수 프린트
  - p [함수명]::[변수명] 특정 함수의 정적 변수 프린트
  - info locals 현재 상태의 지역변수 리스트
```
(gdb) p from
$1 = 0x8049b60 "Merry X-mas !"
(gdb) n
9 while ((to[i] = from[i]) != '\0')
(gdb) n
10 ++i;
(gdb) p to
$2 = 0x8049760 "M"
```
```
gdb) c
Continuing.
Happy New Year !
Breakpoint 1, copy(from=0x8049b60
"Happy New Year !",
to=0x8049760 "Merry X-mas !")
at copy.c:9
8 i = 0;
(gdb) p from
$3 = 0x8049b60 "Happy New Year !"
(gdb) n
9 while ((to[i] = from[i])!='\0')
(gdb) n
10 ++i;
```
```
(gdb) p to
$4 = 0x8049760 "Herry X-mas !"
(gdb) c
Continuing.
Happy New Year !
Program exited normally
```

# 이클립스 통합개발환경 
## 이클립스 통합개발환
- 다양한 언어(C/C++, Java 등)를 지원하는 통합개발환경
- 설치
  - 이클립스 홈페이지에서 C/C++ 개발자를 위한 리눅스용 이클립스(Eclipse IDE for C/C++ Developers)를 다운받아 설치
  - 사전에 make 시스템과 g++ 컴파일러 등이 설치되어야 함

![image](https://github.com/user-attachments/assets/8bfd2921-8ddb-4115-a361-24637fd771ea)

## 새로운 C 프로젝트 생성 
- [Create a new C/C++ project] 혹은 [File]→[New]→[C/C++ Projects]
- 프로젝트 선택 화면에서 [C Managed Build] 선택
- 프로젝트 타입 [Hello World ANSI C Project] 선택
- 컴파일러 선택 후 [Finish] 버튼 클릭
- ‘HelloWorld.c’ 프로그램 자동으로 생성
- 프로젝트 타입 [Empty Project]를 선택하면 빈 프로젝트가 생성

![image](https://github.com/user-attachments/assets/a3439cbf-306f-4a4e-b0b1-a754920df21e)
![image](https://github.com/user-attachments/assets/b65c80dc-ed06-4ede-a7b5-f37cca4a4782)

## 메인 화면 
- 좌측 [Project Explorer] 탐색 창
  - 새로 생성된 프로젝트 확인
  - 프로젝트 및 파일들을 탐색 가능
  - 소스 파일은 src 폴더에, 헤더 파일은 include 폴더에 저장됨
- 중앙 창
  - 소스 파일 등을 편집할 수 있는 창
- 우측 창의 [Outline] 탭
  - 이 프로그램에서 사용하는 소스 파일들을 리스트
  - 해당 파일을 선택하여 파일 내용을 확인하거나 편집 가능
- 하단
  - C 파일을 컴파일 혹은 실행한 결과를 보여주는 창들
 
![image](https://github.com/user-attachments/assets/f223da51-ce36-4991-a130-63c50098d644)

# vi 에디터 

## vi 에디 
- vi 에디터
  - 기본 텍스트 에디터로 매우 강력한 기능을 가지고 있으나
  - 배우는데 상당한 시간과 노력이 필요하다.
```
$ vi 파일*
```

![image](https://github.com/user-attachments/assets/9d70d78a-7f40-4102-b15e-470f399bf92e) 

## 명령 모드/입력 모드 
- vi 에디터는 명령 모드와 입력 모드가 구분되어 있으며 시작하면 명령 모드이다
![image](https://github.com/user-attachments/assets/af6ca1e3-7c2e-4351-8fd9-1f9dba44686b)

- 마지막 줄 모드
  - :wq 작업 내용을 저장하고 종료 (ZZ와 동일한 기능)
  - :q 아무런 작업을 하지 않은 경우의 종료
  - :q! 작업 내용을 저장하지 않고 종료
 
## vi 내부 명령어 
- 원하는 위치로 이동하는 명령
- 입력모드로 전환하는 명령
- 수정 혹은 삭제 명령
- 복사 및 붙이기
- 기타 명령

## 원하는 위치로 이동하는 명령 
- 커서 이동
![image](https://github.com/user-attachments/assets/d5c6056c-3110-47fd-95b3-fe7a630f2d24)

- 화면 이동
![image](https://github.com/user-attachments/assets/85dd3152-f580-4291-8568-e32260ba9466)

- 특정 줄로 이동
![image](https://github.com/user-attachments/assets/70a12b61-7bff-4351-baa4-112fffd5e3b5)

- 탐색 (search)
![image](https://github.com/user-attachments/assets/4cd15213-9477-461d-809a-0b1a02e16787)

## 입력모드로 전환하는 명령 
- 입력모드로 전환
![image](https://github.com/user-attachments/assets/5954958a-a2ab-4b05-afa1-fdba2d9a49a2)

![image](https://github.com/user-attachments/assets/e98882f5-ffbb-49f0-a84c-2eded13dac86)

![image](https://github.com/user-attachments/assets/22f80188-0465-4bda-b5f0-d3cc3c8d9269)

## 수정 혹은 삭제 명령 
- 현재 커서를 중심으로 수정
![image](https://github.com/user-attachments/assets/d81478e2-785a-4de7-bf1d-1a2db57d934e) 

- 삭제
![image](https://github.com/user-attachments/assets/e051a9cd-49a2-4655-8560-58c0cc68805a)

## 대체, 수행취소/재수행 
- 대체 명령
![image](https://github.com/user-attachments/assets/c05c8dc0-57f4-4d45-ac83-ea1b89a5652c)

- 수행취소/재수행
![image](https://github.com/user-attachments/assets/24a47480-3610-4b0b-9883-d969cb3d2842)

## 복사/붙이기 
- 줄 내용 복사(copy)
![image](https://github.com/user-attachments/assets/7a13de9b-064f-46fe-ac7a-dc100ad080a1)

- 마지막으로 삭제/복사한 내용을 붙이기(put)
![image](https://github.com/user-attachments/assets/705bc051-16fc-4b80-a388-4b4b8f638cd3)

## 파일에 저장 및 끝내기 
- 파일에 저장
![image](https://github.com/user-attachments/assets/d73cff69-6e8f-4be0-9b2e-0420b9e39141)

- 파일에 저장하고 끝내기
![image](https://github.com/user-attachments/assets/e2fc0930-8958-4156-ad6e-8f1b8472b1a7)

- 저장하지 않고 끝내기
![image](https://github.com/user-attachments/assets/43556f4f-4543-4166-969f-8824666fb6c4)

![image](https://github.com/user-attachments/assets/d55b4c9d-16cd-4986-b16c-121e59dac215)

## 기타  
- 다른 파일 편집
![image](https://github.com/user-attachments/assets/8634fcbc-38bb-4dde-ae4b-8c576c2116d2)

- 줄 번호 붙이기
![image](https://github.com/user-attachments/assets/ab32e35d-5317-43f3-b407-7c1627f5cc9d)

- 쉘 명령어 수행
![image](https://github.com/user-attachments/assets/3c5802a6-141a-4ba4-8f72-c2cd2fab7168)

# 핵심 개념
- gedit는 GNU가 제공하는 대표적인 텍스트 편집기이다
- gcc 컴파일러는 C 프로그램을 컴파일한다. 옵션을 사용하지 않으면 실행파일 a.out를 생성한다
- make 시스템은 메이크파일(makefile 혹은 Makefile)을 이용하여 보통 실행 파일을 빌드한다
- gdb 디버거는 실행파일을 이용하여 디버깅 모드로 실행한다
- vi 에디터는 명령 모드와 입력 모드가 구분되어 있으며 시작한면 명령 모드이다
