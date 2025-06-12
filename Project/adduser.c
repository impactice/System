#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <crypt.h>
#include <time.h>
#include <errno.h>

#define MAX_USERNAME_LEN 32
#define MAX_PASSWORD_LEN 128
#define MAX_FULLNAME_LEN 256
#define MAX_PATH_LEN 512

// 사용자 정보 구조체
typedef struct {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    char fullname[MAX_FULLNAME_LEN];
    char home_dir[MAX_PATH_LEN];
    char shell[MAX_PATH_LEN];
    uid_t uid;
    gid_t gid;
    int create_home;
    int system_user;
} user_info_t;

// 다음 사용가능한 UID 찾기
uid_t find_next_uid(int system_user) {
    uid_t start_uid = system_user ? 100 : 1000;  // 시스템 사용자는 100부터, 일반 사용자는 1000부터
    uid_t max_uid = system_user ? 999 : 65533;
    
    for (uid_t uid = start_uid; uid <= max_uid; uid++) {
        if (getpwuid(uid) == NULL) {
            return uid;
        }
    }
    
    return 0;  // 사용 가능한 UID가 없음
}

// 사용자명 유효성 검사
int validate_username(const char* username) {
    int len = strlen(username);
    
    // 길이 검사
    if (len == 0 || len >= MAX_USERNAME_LEN) {
        printf("adduser: invalid username length\n");
        return 0;
    }
    
    // 첫 글자는 문자여야 함
    if (!((username[0] >= 'a' && username[0] <= 'z') || 
          (username[0] >= 'A' && username[0] <= 'Z'))) {
        printf("adduser: username must start with a letter\n");
        return 0;
    }
    
    // 허용된 문자만 사용 (영문자, 숫자, 하이픈, 언더스코어)
    for (int i = 0; i < len; i++) {
        char c = username[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
              (c >= '0' && c <= '9') || c == '-' || c == '_')) {
            printf("adduser: username contains invalid characters\n");
            return 0;
        }
    }
    
    // 기존 사용자와 중복 확인
    struct passwd *pw = getpwnam(username);
    if (pw != NULL) {
        printf("adduser: user '%s' already exists\n", username);
        return 0;
    }
    
    return 1;
}

// 비밀번호 입력 (화면에 표시하지 않음)
char* getpass_secure(const char* prompt) {
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

// 사용자 정보 대화형 입력
int get_user_info_interactive(user_info_t* user) {
    char input[256];
    
    printf("Adding user `%s' ...\n", user->username);
    
    // 그룹 생성 (사용자명과 동일한 그룹)
    printf("Adding new group `%s' (%d) ...\n", user->username, user->gid);
    
    // 사용자 추가
    printf("Adding new user `%s' (%d) with group `%s' ...\n", 
           user->username, user->uid, user->username);
    
    // 홈 디렉토리 생성
    if (user->create_home) {
        printf("Creating home directory `%s' ...\n", user->home_dir);
    }
    
    // 기본 파일 복사
    printf("Copying files from `/etc/skel' ...\n");
    
    // 사용자 정보 입력
    printf("Enter new UNIX password: ");
    char* password1 = getpass_secure("");
    printf("Retype new UNIX password: ");
    char* password2 = getpass_secure("");
    
    if (strcmp(password1, password2) != 0) {
        printf("Sorry, passwords do not match\n");
        return 0;
    }
    
    strncpy(user->password, password1, MAX_PASSWORD_LEN - 1);
    
    // 개인정보 입력
    printf("Changing the user information for %s\n", user->username);
    printf("Enter the new value, or press ENTER for the default\n");
    
    printf("\tFull Name []: ");
    fflush(stdout);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n') {
            input[len-1] = '\0';
        }
        if (strlen(input) > 0) {
            strncpy(user->fullname, input, MAX_FULLNAME_LEN - 1);
        }
    }
    
    printf("\tRoom Number []: ");
    fflush(stdout);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // 입력 무시 (데모용)
    }
    
    printf("\tWork Phone []: ");
    fflush(stdout);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // 입력 무시 (데모용)
    }
    
    printf("\tHome Phone []: ");
    fflush(stdout);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // 입력 무시 (데모용)
    }
    
    printf("\tOther []: ");
    fflush(stdout);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // 입력 무시 (데모용)
    }
    
    printf("Is the information correct? [Y/n] ");
    fflush(stdout);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        if (input[0] == 'n' || input[0] == 'N') {
            printf("User creation cancelled.\n");
            return 0;
        }
    }
    
    return 1;
}

// 사용자 생성 시뮬레이션
int create_user_simulation(const user_info_t* user) {
    printf("\n=== 사용자 생성 시뮬레이션 ===\n");
    
    // 그룹 생성 시뮬레이션
    printf("[시뮬레이션] Creating group '%s' with GID %d\n", user->username, user->gid);
    printf("[시뮬레이션] Group entry: %s:x:%d:\n", user->username, user->gid);
    
    // 사용자 생성 시뮬레이션
    printf("[시뮬레이션] Creating user '%s' with UID %d\n", user->username, user->uid);
    
    // 비밀번호 암호화
    char salt[16];
    snprintf(salt, sizeof(salt), "$6$%.8lx$", (unsigned long)time(NULL));
    char* encrypted = crypt(user->password, salt);
    
    printf("[시뮬레이션] Password entry: %s:%s:%d:%d:%s:%s:%s\n",
           user->username, encrypted, user->uid, user->gid,
           user->fullname, user->home_dir, user->shell);
    
    // 홈 디렉토리 생성 시뮬레이션
    if (user->create_home) {
        printf("[시뮬레이션] Creating home directory: %s\n", user->home_dir);
        printf("[시뮬레이션] Setting permissions: chmod 755 %s\n", user->home_dir);
        printf("[시뮬레이션] Setting ownership: chown %d:%d %s\n", 
               user->uid, user->gid, user->home_dir);
        
        // 기본 파일 복사 시뮬레이션
        printf("[시뮬레이션] Copying files from /etc/skel:\n");
        printf("  - .bash_logout\n");
        printf("  - .bashrc\n");
        printf("  - .profile\n");
    }
    
    printf("\n사용자 '%s'가 성공적으로 생성되었습니다.\n", user->username);
    return 1;
}

// adduser 명령어 메인 함수
int cmd_adduser(const char* username, int system_user, int no_create_home) {
    // root 권한 확인
    if (getuid() != 0) {
        printf("adduser: Only root may add a user or group to the system.\n");
        return 1;
    }
    
    // 사용자명 유효성 검사
    if (!validate_username(username)) {
        return 1;
    }
    
    // 사용자 정보 초기화
    user_info_t user = {0};
    strncpy(user.username, username, MAX_USERNAME_LEN - 1);
    user.system_user = system_user;
    user.create_home = !no_create_home && !system_user;
    
    // UID/GID 할당
    user.uid = find_next_uid(system_user);
    if (user.uid == 0) {
        printf("adduser: no available UID found\n");
        return 1;
    }
    user.gid = user.uid;  // 기본적으로 UID와 동일한 GID 사용
    
    // 홈 디렉토리 설정
    if (system_user) {
        strncpy(user.home_dir, "/var/lib/", MAX_PATH_LEN - 1);
        strncat(user.home_dir, username, MAX_PATH_LEN - strlen(user.home_dir) - 1);
        strncpy(user.shell, "/usr/sbin/nologin", MAX_PATH_LEN - 1);
    } else {
        strncpy(user.home_dir, "/home/", MAX_PATH_LEN - 1);
        strncat(user.home_dir, username, MAX_PATH_LEN - strlen(user.home_dir) - 1);
        strncpy(user.shell, "/bin/bash", MAX_PATH_LEN - 1);
    }
    
    // 시스템 사용자가 아닌 경우 대화형 입력
    if (!system_user) {
        if (!get_user_info_interactive(&user)) {
            return 1;
        }
    } else {
        // 시스템 사용자는 비밀번호 없음
        user.password[0] = '\0';
        printf("Adding system user `%s' (UID %d) ...\n", username, user.uid);
        printf("Adding new group `%s' (GID %d) ...\n", username, user.gid);
        printf("Adding new user `%s' (UID %d) with group `%s' ...\n", 
               username, user.uid, username);
    }
    
    // 사용자 생성 (시뮬레이션)
    return create_user_simulation(&user) ? 0 : 1;
}

// 도움말 출력
void print_adduser_help() {
    printf("Usage: adduser [OPTIONS] LOGIN\n");
    printf("Add a user to the system.\n\n");
    printf("Options:\n");
    printf("  --system           create a system account\n");
    printf("  --no-create-home   do not create the user's home directory\n");
    printf("  --help             display this help and exit\n\n");
    printf("Examples:\n");
    printf("  adduser john              # Add regular user 'john'\n");
    printf("  adduser --system www-data # Add system user 'www-data'\n");
}

// 터미널에서 사용할 함수
int execute_adduser(char **args) {
    int system_user = 0;
    int no_create_home = 0;
    char* username = NULL;
    
    // 인자 파싱
    for (int i = 1; args[i] != NULL; i++) {
        if (strcmp(args[i], "--help") == 0) {
            print_adduser_help();
            return 0;
        } else if (strcmp(args[i], "--system") == 0) {
            system_user = 1;
        } else if (strcmp(args[i], "--no-create-home") == 0) {
            no_create_home = 1;
        } else if (args[i][0] != '-') {
            if (username == NULL) {
                username = args[i];
            } else {
                printf("adduser: too many arguments\n");
                return 1;
            }
        } else {
            printf("adduser: unknown option '%s'\n", args[i]);
            return 1;
        }
    }
    
    if (username == NULL) {
        printf("adduser: missing username\n");
        print_adduser_help();
        return 1;
    }
    
    return cmd_adduser(username, system_user, no_create_home);
}

// 테스트용 main 함수
int main(int argc, char *argv[]) {
    printf("=== 교육용 adduser 명령어 구현 ===\n");
    printf("주의: 이는 교육용 구현으로 실제 시스템에 사용자를 추가하지 않습니다.\n\n");
    
    return execute_adduser(argv);
}