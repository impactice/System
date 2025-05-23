# 프로세스 제어 

## 프로세스 생성 
- 프로세스 생성
  - 부모 프로세스가 자식 프로세스 생성

- fork() 시스템 호출
  - 부모 프로세스를 똑같이 복제하여 새로운 자식 프로세스를 생성
  - 자기복제(自己複製)

![image](https://github.com/user-attachments/assets/27ff951b-86d7-45c3-9d32-af629033fab8)

```
#include <sys/types.h>
#include <unistd.h>
pid_t fork(void);
```
새로운 자식 프로세스를 생성한다. 자식 프로세스에게는 0을 리턴하고 부모 프로세스에게는 자식 프로세스 ID를 리턴한다

![image](https://github.com/user-attachments/assets/f6effa91-6b03-40fc-ab9e-c43ab77c4bf0)

- fork()는 한 번 호출되면 두 번 리턴한다.
  - 자식 프로세스에게는 0을 리턴하고
  - 부모 프로세스에게는 자식 프로세스 ID를 리턴한다.
  - 부모 프로세스와 자식 프로세스는 병행적으로 각각 실행을 계속한다

## 프로세스 생성: fork1.c
```
#include <stdio.h>
#include <unistd.h>

/* 자식 프로세스를 생성하는 프로그램 */
int main() {
    int pid;

    // 현재 프로세스 ID 출력
    printf("[%d] 프로세스 시작\n", getpid());

    // 새로운 프로세스 생성
    pid = fork();

    // 부모 및 자식 프로세스에서 출력
    printf("[%d] 프로세스 : fork() 반환값 %d\n", getpid(), pid);

    return 0;
}
```
```
gcc -o fork1 fork1.c
```

```
./fork1
```
```
[15065] 프로세스 시작
[15065] 프로세스 : 반환값 15066
[15066] 프로세스 : 반환값 0
```

## 부모 프로세스와 자식 프로세스 구분 
- fork() 호출 후에 리턴값이 다르므로 이 리턴값을 이용하여 부모 프로세스와 자식 프로세스를 구별하고 서로 다른 일을 하도록 할 수 있다

```
pid = fork();
if ( pid == 0 )
{ 자식 프로세스의 실행 코드 }
else
{ 부모 프로세스의 실행 코드 }
```

## fork2.c
```
#include <stdio.h>
#include <unistd.h>

/* 자식 프로세스를 생성하는 프로그램 */
int main() {
    int pid;

    // 현재 프로세스 ID 출력
    printf("[%d] 프로세스 시작\n", getpid());

    // 새로운 프로세스 생성
    pid = fork();

    // 부모 및 자식 프로세스에서 출력
    printf("[%d] 프로세스 : fork() 반환값 %d\n", getpid(), pid);

    return 0;
}
```

```
gcc -o fork2 fork2.c
```
```
./fork2
```

```
[Parent] Hello, world! pid=15799
[Child] Hello, world! pid=15800
```

## 두 개의 자식 프로세스 생성: fork3.c 
```

```





