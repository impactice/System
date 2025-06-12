#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>

int cmd_whoami() {
    // 방법 1: getuid()와 getpwuid() 사용
    uid_t uid = getuid();  // 현재 사용자의 UID 가져오기
    struct passwd *pw = getpwuid(uid);  // UID로 사용자 정보 가져오기
    
    if (pw == NULL) {
        perror("getpwuid");
        return 1;
    }
    
    printf("%s\n", pw->pw_name);
    return 0;
}

// 대안 방법: getlogin() 사용
int cmd_whoami_alt() {
    char *username = getlogin();
    
    if (username == NULL) {
        perror("getlogin");
        return 1;
    }
    
    printf("%s\n", username);
    return 0;
}

// 환경변수를 사용하는 방법 (백업용)
int cmd_whoami_env() {
    char *username = getenv("USER");
    
    if (username == NULL) {
        username = getenv("LOGNAME");
    }
    
    if (username == NULL) {
        fprintf(stderr, "whoami: cannot find username\n");
        return 1;
    }
    
    printf("%s\n", username);
    return 0;
}

// 테스트용 main 함수
int main() {
    printf("Method 1 (getuid + getpwuid): ");
    cmd_whoami();
    
    printf("Method 2 (getlogin): ");
    cmd_whoami_alt();
    
    printf("Method 3 (environment): ");
    cmd_whoami_env();
    
    return 0;
}

// 터미널 구현에서 사용할 함수
int execute_whoami(char **args) {
    (void)args;  // 인자 사용하지 않음을 명시
    return cmd_whoami();
}