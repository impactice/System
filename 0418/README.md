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
$ 명령어
^C

- 예
$ (sleep 100; echo DONE)
^C
$ 
- 실행 정지 Ctrl-Z
$ 명령어
^Z
[1]+ 정지됨 명령어
