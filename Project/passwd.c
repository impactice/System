#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <termios.h>
#include <crypt.h>
#include <time.h>
#include <sys/types.h>

#define MAX_PASSWORD_LEN 128
#define MIN_PASSWORD_LEN 6

// 비밀번호 입력 시 화면에 표시하지 않기 위한 함수
char* getpass_custom(const char* prompt) {
    static char password[MAX_PASSWORD_LEN];
    struct termios old_termios, new_termios;
    
    printf("%s", prompt);
    fflush(stdout);
    
    // 터미널 설정 변경 (echo 끄기)
    tcgetattr(STDIN_FILENO, &old_termios);
    new_termios = old_termios;
    new_termios.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    
    // 비밀번호 입력
    if (fgets(password, sizeof(password), stdin) != NULL) {
        // 개행문자 제거
        size_t len = strlen(password);
        if (len > 0 && password[len-1] == '\n') {
            password[len-1] = '\0';
        }
    }
    
    // 터미널 설정 복구
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
    printf("\n");
    
    return password;
}

// 비밀번호 강도 검사
int validate_password(const char* password) {
    int len = strlen(password);
    int has_upper = 0, has_lower = 0, has_digit = 0, has_special = 0;
    
    if (len < MIN_PASSWORD_LEN) {
        printf("BAD PASSWORD: The password is shorter than %d characters\n", MIN_PASSWORD_LEN);
        return 0;
    }
    
    if (len > MAX_PASSWORD_LEN) {
        printf("BAD PASSWORD: The password is too long\n");
        return 0;
    }
    
    // 문자 종류 검사
    for (int i = 0; i < len; i++) {
        if (password[i] >= 'A' && password[i] <= 'Z') has_upper = 1;
        else if (password[i] >= 'a' && password[i] <= 'z') has_lower = 1;
        else if (password[i] >= '0' && password[i] <= '9') has_digit = 1;
        else has_special = 1;
    }
    
    int complexity = has_upper + has_lower + has_digit + has_special;
    if (complexity < 3) {
        printf("BAD PASSWORD: The password must contain at least 3 of the following:\n");
        printf("  - uppercase letters\n");
        printf("  - lowercase letters\n");
        printf("  - digits\n");
        printf("  - special characters\n");
        return 0;
    }
    
    // 간단한 사전 단어 검사
    const char* common_passwords[] = {
        "password", "123456", "qwerty", "admin", "root", "user", "guest", NULL
    };
    
    for (int i = 0; common_passwords[i] != NULL; i++) {
        if (strcasecmp(password, common_passwords[i]) == 0) {
            printf("BAD PASSWORD: The password is too simple\n");
            return 0;
        }
    }
    
    return 1;
}

// 비밀번호 암호화 (교육용 - 실제로는 더 강력한 해시 사용)
char* encrypt_password(const char* password) {
    static char salt[16];
    static char* encrypted;
    
    // 간단한 salt 생성
    snprintf(salt, sizeof(salt), "$6$%.8lx$", (unsigned long)time(NULL));
    
    // crypt 함수 사용 (실제 시스템에서는 더 강력한 해시 사용)
    encrypted = crypt(password, salt);
    return encrypted;
}

// 현재 사용자의 비밀번호 변경
int change_own_password() {
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    
    if (pw == NULL) {
        perror("getpwuid");
        return 1;
    }
    
    printf("Changing password for %s.\n", pw->pw_name);
    
    // 현재 비밀번호 확인 (실제로는 /etc/shadow 확인)
    char* current_pass = getpass_custom("(current) UNIX password: ");
    
    // 교육용이므로 간단한 확인
    if (strlen(current_pass) == 0) {
        printf("passwd: Authentication token manipulation error\n");
        return 1;
    }
    
    // 새 비밀번호 입력
    char* new_pass1 = getpass_custom("Enter new UNIX password: ");
    
    // 비밀번호 강도 검사
    if (!validate_password(new_pass1)) {
        printf("passwd: Have exhausted maximum number of retries for service\n");
        return 1;
    }
    
    // 비밀번호 확인
    char* new_pass2 = getpass_custom("Retype new UNIX password: ");
    
    if (strcmp(new_pass1, new_pass2) != 0) {
        printf("Sorry, passwords do not match.\n");
        printf("passwd: Authentication token manipulation error\n");
        return 1;
    }
    
    // 비밀번호 암호화
    char* encrypted = encrypt_password(new_pass1);
    
    printf("passwd: password updated successfully\n");
    printf("[교육용] 암호화된 비밀번호: %s\n", encrypted);
    
    return 0;
}

// 다른 사용자의 비밀번호 변경 (root 권한 필요)
int change_user_password(const char* username) {
    // root 권한 확인
    if (getuid() != 0) {
        printf("passwd: You may not view or modify password information for %s.\n", username);
        return 1;
    }
    
    // 사용자 존재 확인
    struct passwd *pw = getpwnam(username);
    if (pw == NULL) {
        printf("passwd: user '%s' does not exist\n", username);
        return 1;
    }
    
    printf("Changing password for user %s.\n", username);
    
    // 새 비밀번호 입력
    char* new_pass1 = getpass_custom("New password: ");
    
    // 비밀번호 강도 검사
    if (!validate_password(new_pass1)) {
        printf("passwd: Have exhausted maximum number of retries for service\n");
        return 1;
    }
    
    // 비밀번호 확인
    char* new_pass2 = getpass_custom("Retype new password: ");
    
    if (strcmp(new_pass1, new_pass2) != 0) {
        printf("Sorry, passwords do not match.\n");
        printf("passwd: Authentication token manipulation error\n");
        return 1;
    }
    
    // 비밀번호 암호화
    char* encrypted = encrypt_password(new_pass1);
    
    printf("passwd: password updated successfully\n");
    printf("[교육용] 사용자 %s의 암호화된 비밀번호: %s\n", username, encrypted);
    
    return 0;
}

// passwd 명령어 도움말
void print_passwd_help() {
    printf("Usage: passwd [OPTION] [LOGIN]\n");
    printf("Change user password.\n\n");
    printf("Options:\n");
    printf("  -h, --help     display this help message and exit\n");
    printf("  LOGIN          change password for LOGIN user (root only)\n\n");
    printf("If no LOGIN is provided, changes password for current user.\n");
}

// 터미널에서 사용할 메인 함수
int execute_passwd(char **args) {
    // 도움말 옵션 확인
    if (args[1] != NULL && (strcmp(args[1], "-h") == 0 || strcmp(args[1], "--help") == 0)) {
        print_passwd_help();
        return 0;
    }
    
    // 사용자 지정 여부 확인
    if (args[1] != NULL) {
        // 다른 사용자의 비밀번호 변경
        return change_user_password(args[1]);
    } else {
        // 자신의 비밀번호 변경
        return change_own_password();
    }
}

// 테스트용 main 함수
int main(int argc, char *argv[]) {
    printf("=== 교육용 passwd 명령어 구현 ===\n");
    printf("주의: 이는 교육용 구현으로 실제 시스템 비밀번호를 변경하지 않습니다.\n\n");
    
    return execute_passwd(argv);
}