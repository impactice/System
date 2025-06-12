#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <limits.h>

// 전역 변수: 이전 디렉토리 저장
static char previous_dir[PATH_MAX] = "";
static int has_previous = 0;

// 함수 선언
void print_usage(const char *program_name);
char *get_home_directory(void);
char *resolve_path(const char *path);
int change_directory(const char *path);
void print_current_directory(void);

int main(int argc, char *argv[]) {
    char *target_dir = NULL;
    
    // 인수 처리
    if (argc > 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (argc == 2) {
        target_dir = argv[1];
    }
    
    // cd 명령어 실행
    if (change_directory(target_dir) == 0) {
        // 성공 시 현재 디렉토리 출력 (선택사항)
        // print_current_directory();
        return 0;
    } else {
        return 1;
    }
}

void print_usage(const char *program_name) {
    printf("사용법: %s [디렉토리]\n", program_name);
    printf("옵션:\n");
    printf("  (없음)    홈 디렉토리로 이동\n");
    printf("  ..        상위 디렉토리로 이동\n");
    printf("  ~         홈 디렉토리로 이동\n");
    printf("  -         이전 디렉토리로 이동\n");
    printf("  경로      지정된 경로로 이동\n");
}

char *get_home_directory(void) {
    char *home = getenv("HOME");
    if (home != NULL) {
        return home;
    }
    
    // HOME 환경변수가 없으면 passwd에서 가져오기
    struct passwd *pw = getpwuid(getuid());
    if (pw != NULL) {
        return pw->pw_dir;
    }
    
    return NULL;
}

char *resolve_path(const char *path) {
    static char resolved[PATH_MAX];
    
    if (path == NULL) {
        // 인수가 없으면 홈 디렉토리
        char *home = get_home_directory();
        if (home == NULL) {
            fprintf(stderr, "cd: 홈 디렉토리를 찾을 수 없습니다\n");
            return NULL;
        }
        strcpy(resolved, home);
        return resolved;
    }
    
    if (strcmp(path, "~") == 0) {
        // 홈 디렉토리
        char *home = get_home_directory();
        if (home == NULL) {
            fprintf(stderr, "cd: 홈 디렉토리를 찾을 수 없습니다\n");
            return NULL;
        }
        strcpy(resolved, home);
        return resolved;
    }
    
    if (strcmp(path, "-") == 0) {
        // 이전 디렉토리
        if (!has_previous) {
            fprintf(stderr, "cd: OLDPWD가 설정되지 않았습니다\n");
            return NULL;
        }
        strcpy(resolved, previous_dir);
        printf("%s\n", resolved);  // 이전 디렉토리로 갈 때는 경로 출력
        return resolved;
    }
    
    if (path[0] == '~' && path[1] == '/') {
        // ~/path 형태의 경로
        char *home = get_home_directory();
        if (home == NULL) {
            fprintf(stderr, "cd: 홈 디렉토리를 찾을 수 없습니다\n");
            return NULL;
        }
        snprintf(resolved, PATH_MAX, "%s%s", home, path + 1);
        return resolved;
    }
    
    // 절대 경로 또는 상대 경로
    if (realpath(path, resolved) == NULL) {
        // realpath 실패 시 원본 경로 사용
        strcpy(resolved, path);
    }
    
    return resolved;
}

int change_directory(const char *path) {
    char current_dir[PATH_MAX];
    char *target_path;
    
    // 현재 디렉토리 저장 (이전 디렉토리로 사용하기 위해)
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        perror("cd: 현재 디렉토리를 가져올 수 없습니다");
        return -1;
    }
    
    // 목표 경로 결정
    target_path = resolve_path(path);
    if (target_path == NULL) {
        return -1;
    }
    
    // 디렉토리 존재 여부 확인
    struct stat st;
    if (stat(target_path, &st) != 0) {
        perror(target_path);
        return -1;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "cd: %s: 디렉토리가 아닙니다\n", target_path);
        return -1;
    }
    
    // 디렉토리 변경
    if (chdir(target_path) != 0) {
        perror(target_path);
        return -1;
    }
    
    // 이전 디렉토리 업데이트
    strcpy(previous_dir, current_dir);
    has_previous = 1;
    
    // 환경변수 업데이트
    setenv("OLDPWD", current_dir, 1);
    
    // PWD 환경변수 업데이트
    char new_pwd[PATH_MAX];
    if (getcwd(new_pwd, sizeof(new_pwd)) != NULL) {
        setenv("PWD", new_pwd, 1);
    }
    
    return 0;
}

void print_current_directory(void) {
    char current_dir[PATH_MAX];
    if (getcwd(current_dir, sizeof(current_dir)) != NULL) {
        printf("%s\n", current_dir);
    } else {
        perror("pwd");
    }
}

// 터미널에서 사용할 수 있는 추가 함수들
void init_cd(void) {
    // 터미널 시작 시 호출하여 초기화
    char *pwd = getenv("PWD");
    if (pwd != NULL) {
        strcpy(previous_dir, pwd);
        has_previous = 1;
    }
}

// 터미널 프로그램에서 사용할 수 있는 cd 함수
int terminal_cd(const char *path) {
    return change_directory(path);
}

// 현재 디렉토리 가져오기 (터미널 프롬프트용)
char *get_current_directory(void) {
    static char current_dir[PATH_MAX];
    if (getcwd(current_dir, sizeof(current_dir)) != NULL) {
        return current_dir;
    }
    return NULL;
}