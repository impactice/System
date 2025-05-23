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

## 프로세스 기다리기: forkwait.c 
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid, child;
    int status;

    printf("[%d] 부모 프로세스 시작\n", getpid());

    // 자식 프로세스 생성
    pid = fork();
    if (pid < 0) {
        perror("fork 실패");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        // 자식 프로세스 코드 실행
        printf("[%d] 자식 프로세스 시작\n", getpid());
        exit(1);
    }

    // 부모 프로세스: 자식의 종료를 기다림
    child = wait(&status);
    if (child == -1) {
        perror("wait 실패");
        exit(EXIT_FAILURE);
    }

    printf("[%d] 자식 프로세스 %d 종료\n", getpid(), child);
    printf("\t종료 코드 %d\n", status >> 8);  // 또는 WEXITSTATUS(status) 사용 가능

    return EXIT_SUCCESS;
}
```

```
gcc -o forkwait forkwait.c
```

```
./forkwait
```
```
[15943] 부모 프로세스 시작
[15944] 자식 프로세스 시작
[15943] 자식 프로세스 15944 종료
종료코드 1
```

## 특정 자식 프로세스 기다리기: waitpid.c 
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    int pid1, pid2, child, status;

    printf("[%d] 부모 프로세스 시작\n", getpid());

    /* 첫 번째 자식 프로세스 생성 */
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork 오류");
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0) {
        printf("[%d] 자식 프로세스[1] 시작\n", getpid());
        sleep(1);
        printf("[%d] 자식 프로세스[1] 종료\n", getpid());
        exit(1);
    }
    
    /* 두 번째 자식 프로세스 생성 */
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork 오류");
        exit(EXIT_FAILURE);
    }
    if (pid2 == 0) {
        printf("[%d] 자식 프로세스 #2 시작\n", getpid());
        sleep(2);
        printf("[%d] 자식 프로세스 #2 종료\n", getpid());
        exit(2);
    }

    /* 자식 프로세스 [1]의 종료를 기다림 */
    child = waitpid(pid1, &status, 0);
    if (child < 0) {
        perror("waitpid 오류");
        exit(EXIT_FAILURE);
    }
    printf("[%d] 자식 프로세스 #1 (%d) 종료\n", getpid(), child);
    printf("\t종료 코드: %d\n", status >> 8);

    return EXIT_SUCCESS;
}
```

```
gcc -o waitpid waitpid.c
```
```
./waitpid
```
```
[16840] 부모 프로세스 시작
[16841] 자식 프로세스[1] 시작
[16842] 자식 프로세스[2] 시작
[16841] 자식 프로세스[1] 종료
[16840] 자식 프로세스[1] 16841 종료
종료코드 1
[16842] 자식 프로세스[2] 종료
```

## 프로그램 실행 
- fork() 후
  - 자식 프로세스는 부모 프로세스와 똑같은 코드 실행

- 자식 프로세스에게 새 프로그램 실행
  - exec() 시스템 호출 사용
  - 프로세스 내의 프로그램을 새 프로그램으로 대치

- 보통 fork() 후에 exec( ) 

![image](https://github.com/user-attachments/assets/a275605c-3540-4fa2-89b8-966111490275)

## 프로그램 실행: exec() 
- 프로세스가 exec() 호출을 하면,
 그 프로세스 내의 프로그램은 완전히 새로운 프로그램으로 대치
 자기대치(自己代置)
 새 프로그램의 main()부터 실행이 시작된다
























