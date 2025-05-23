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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 부모 프로세스가 두 개의 자식 프로세스를 생성한다.
int main() {
    int pid1, pid2;

    // 첫 번째 fork: 자식 프로세스 1 생성
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0) {
        printf("[Child 1] : Hello, world! pid = %d\n", getpid());
        exit(EXIT_SUCCESS);
    }

    // 두 번째 fork: 자식 프로세스 2 생성
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid2 == 0) {
        printf("[Child 2] : Hello, world! pid = %d\n", getpid());
        exit(EXIT_SUCCESS);
    }

    // 부모 프로세스 실행
    printf("[PARENT] : Hello, world! pid = %d\n", getpid());

    return 0;
}
```

```
gcc -o fork3 fork3.c
```

```
./fork3
```

```
[Parent] Hello, world! pid=15740
[Child 1] Hello, world! pid=15741
[Child 2] Hello, world! pid=15742
```

## 프로세스 기다리기: wait() 
- 자식 프로세스 중의 하나가 끝날 때까지 기다린다.
- 끝난 자식 프로세스의 종료 코드가 status에 저장되며 끝난 자식 프로세스의 번호를 리턴한다
```
#include <sys/types.h>
#include <sys/wait.h>
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *statloc, int options);
```

![image](https://github.com/user-attachments/assets/7179e70b-3551-4680-b4af-ab64e7ba67e3)








