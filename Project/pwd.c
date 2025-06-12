#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

// 옵션 플래그
struct pwd_options {
    int logical;    // -L: 논리적 경로 (심볼릭 링크 유지)
    int physical;   // -P: 물리적 경로 (심볼릭 링크 해석)
};

// 함수 선언
void print_usage(const char *program_name);
void parse_options(int argc, char *argv[], struct pwd_options *opts);
char *get_logical_pwd(void);
char *get_physical_pwd(void);
int is_same_directory(const char *path1, const char *path2);

int main(int argc, char *argv[]) {
    struct pwd_options opts = {0, 0};
    char *current_dir = NULL;
    
    // 옵션 파싱
    parse_options(argc, argv, &opts);
    
    // 기본값: 논리적 경로 (환경변수 PWD 사용)
    if (!opts.physical) {
        opts.logical = 1;
    }
    
    if (opts.logical && !opts.physical) {
        // 논리적 경로 출력 (-L 또는 기본값)
        current_dir = get_logical_pwd();
        if (current_dir == NULL) {
            // PWD 환경변수가 없거나 신뢰할 수 없으면 물리적 경로 사용
            current_dir = get_physical_pwd();
        }
    } else {
        // 물리적 경로 출력 (-P)
        current_dir = get_physical_pwd();
    }
    
    if (current_dir != NULL) {
        printf("%s\n", current_dir);
        free(current_dir);
        return 0;
    } else {
        fprintf(stderr, "pwd: 현재 디렉토리를 가져올 수 없습니다\n");
        return 1;
    }
}

void print_usage(const char *program_name) {
    printf("사용법: %s [옵션]\n", program_name);
    printf("현재 작업 디렉토리의 전체 경로를 출력합니다.\n\n");
    printf("옵션:\n");
    printf("  -L     논리적 경로 출력 (심볼릭 링크 유지, 기본값)\n");
    printf("  -P     물리적 경로 출력 (심볼릭 링크 해석)\n");
    printf("  --help 이 도움말을 표시하고 종료\n");
    printf("\n");
    printf("기본적으로 pwd는 논리적 경로를 출력합니다 (-L와 동일).\n");
    printf("이는 셸이 유지하는 PWD 환경변수의 값을 사용합니다.\n");
}

void parse_options(int argc, char *argv[], struct pwd_options *opts) {
    int opt;
    
    while ((opt = getopt(argc, argv, "LP")) != -1) {
        switch (opt) {
            case 'L':
                opts->logical = 1;
                opts->physical = 0;
                break;
            case 'P':
                opts->physical = 1;
                opts->logical = 0;
                break;
            case '?':
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    // --help 옵션 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        }
    }
    
    // 추가 인수가 있으면 오류
    if (optind < argc) {
        fprintf(stderr, "pwd: 추가 인수가 있습니다: %s\n", argv[optind]);
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
}

char *get_logical_pwd(void) {
    char *pwd_env = getenv("PWD");
    char *physical_pwd = NULL;
    
    // PWD 환경변수가 없으면 NULL 반환
    if (pwd_env == NULL) {
        return NULL;
    }
    
    // PWD 환경변수가 현재 물리적 디렉토리와 같은지 확인
    physical_pwd = get_physical_pwd();
    if (physical_pwd == NULL) {
        return NULL;
    }
    
    if (is_same_directory(pwd_env, physical_pwd)) {
        free(physical_pwd);
        // PWD가 유효하면 복사해서 반환
        char *result = malloc(strlen(pwd_env) + 1);
        if (result != NULL) {
            strcpy(result, pwd_env);
        }
        return result;
    } else {
        // PWD가 현재 디렉토리와 다르면 물리적 경로 반환
        return physical_pwd;
    }
}

char *get_physical_pwd(void) {
    char *current_dir = NULL;
    
    // getcwd를 사용하여 현재 디렉토리 가져오기
    current_dir = getcwd(NULL, 0);
    if (current_dir == NULL) {
        perror("getcwd");
        return NULL;
    }
    
    return current_dir;
}

int is_same_directory(const char *path1, const char *path2) {
    struct stat st1, st2;
    
    if (stat(path1, &st1) != 0 || stat(path2, &st2) != 0) {
        return 0;
    }
    
    // 같은 디바이스의 같은 inode면 같은 디렉토리
    return (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino);
}

// 터미널에서 사용할 수 있는 추가 함수들
char *terminal_pwd(int physical) {
    if (physical) {
        return get_physical_pwd();
    } else {
        char *logical = get_logical_pwd();
        if (logical == NULL) {
            logical = get_physical_pwd();
        }
        return logical;
    }
}

// 현재 디렉토리가 심볼릭 링크를 포함하는지 확인
int pwd_contains_symlinks(void) {
    char *logical = get_logical_pwd();
    char *physical = get_physical_pwd();
    int different = 0;
    
    if (logical && physical) {
        different = strcmp(logical, physical) != 0;
    }
    
    free(logical);
    free(physical);
    return different;
}

// 상대 경로를 절대 경로로 변환
char *resolve_absolute_path(const char *path) {
    char *resolved = malloc(PATH_MAX);
    if (resolved == NULL) {
        return NULL;
    }
    
    if (realpath(path, resolved) == NULL) {
        free(resolved);
        return NULL;
    }
    
    return resolved;
}