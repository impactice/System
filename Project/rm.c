#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

typedef struct {
    int recursive;  // -r, -R 옵션
    int force;      // -f 옵션
    int interactive; // -i 옵션
} rm_options;

// 함수 선언
int rm_file(const char *path, rm_options *opts);
int rm_directory(const char *path, rm_options *opts);
int confirm_deletion(const char *path);
void print_usage(void);

// 사용법 출력
void print_usage(void) {
    printf("Usage: rm [OPTION]... FILE...\n");
    printf("Remove (unlink) the FILE(s).\n\n");
    printf("  -f, --force           ignore nonexistent files and arguments, never prompt\n");
    printf("  -i                    prompt before every removal\n");
    printf("  -r, -R, --recursive   remove directories and their contents recursively\n");
    printf("      --help            display this help and exit\n");
}

// 삭제 확인 함수
int confirm_deletion(const char *path) {
    char response[10];
    printf("rm: remove '%s'? ", path);
    fflush(stdout);
    
    if (fgets(response, sizeof(response), stdin) == NULL) {
        return 0;
    }
    
    return (response[0] == 'y' || response[0] == 'Y');
}

// 파일 삭제 함수
int rm_file(const char *path, rm_options *opts) {
    struct stat st;
    
    // 파일 상태 확인
    if (stat(path, &st) == -1) {
        if (!opts->force) {
            fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(errno));
            return -1;
        }
        return 0; // force 옵션이 있으면 존재하지 않는 파일 무시
    }
    
    // 디렉토리인 경우
    if (S_ISDIR(st.st_mode)) {
        if (!opts->recursive) {
            if (!opts->force) {
                fprintf(stderr, "rm: cannot remove '%s': Is a directory\n", path);
            }
            return -1;
        }
        return rm_directory(path, opts);
    }
    
    // interactive 옵션 확인
    if (opts->interactive && !opts->force) {
        if (!confirm_deletion(path)) {
            return 0;
        }
    }
    
    // 파일 삭제
    if (unlink(path) == -1) {
        if (!opts->force) {
            fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(errno));
        }
        return -1;
    }
    
    return 0;
}

// 디렉토리 삭제 함수 (재귀적)
int rm_directory(const char *path, rm_options *opts) {
    DIR *dir;
    struct dirent *entry;
    char full_path[1024];
    int result = 0;
    
    // interactive 옵션 확인 (디렉토리 삭제 전)
    if (opts->interactive && !opts->force) {
        if (!confirm_deletion(path)) {
            return 0;
        }
    }
    
    // 디렉토리 열기
    dir = opendir(path);
    if (dir == NULL) {
        if (!opts->force) {
            fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(errno));
        }
        return -1;
    }
    
    // 디렉토리 내용 삭제
    while ((entry = readdir(dir)) != NULL) {
        // . 과 .. 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 전체 경로 생성
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        // 재귀적으로 삭제
        if (rm_file(full_path, opts) == -1) {
            result = -1;
        }
    }
    
    closedir(dir);
    
    // 빈 디렉토리 삭제
    if (rmdir(path) == -1) {
        if (!opts->force) {
            fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(errno));
        }
        result = -1;
    }
    
    return result;
}

// 옵션 파싱 함수
int parse_options(int argc, char *argv[], rm_options *opts, int *file_start) {
    int i;
    
    // 옵션 초기화
    opts->recursive = 0;
    opts->force = 0;
    opts->interactive = 0;
    
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            break; // 옵션이 아닌 첫 번째 인수
        }
        
        if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 1;
        }
        
        if (strcmp(argv[i], "--") == 0) {
            i++;
            break; // 옵션 끝
        }
        
        // 단일 옵션들 처리
        int j;
        for (j = 1; argv[i][j] != '\0'; j++) {
            switch (argv[i][j]) {
                case 'r':
                case 'R':
                    opts->recursive = 1;
                    break;
                case 'f':
                    opts->force = 1;
                    break;
                case 'i':
                    opts->interactive = 1;
                    break;
                default:
                    fprintf(stderr, "rm: invalid option -- '%c'\n", argv[i][j]);
                    fprintf(stderr, "Try 'rm --help' for more information.\n");
                    return -1;
            }
        }
    }
    
    *file_start = i;
    return 0;
}

int main(int argc, char *argv[]) {
    rm_options opts;
    int file_start;
    int result = 0;
    int i;
    
    // 인수가 없는 경우
    if (argc < 2) {
        fprintf(stderr, "rm: missing operand\n");
        fprintf(stderr, "Try 'rm --help' for more information.\n");
        return 1;
    }
    
    // 옵션 파싱
    int parse_result = parse_options(argc, argv, &opts, &file_start);
    if (parse_result == 1) {
        return 0; // help 출력 후 정상 종료
    }
    if (parse_result == -1) {
        return 1; // 옵션 파싱 오류
    }
    
    // 파일 인수가 없는 경우
    if (file_start >= argc) {
        fprintf(stderr, "rm: missing operand\n");
        fprintf(stderr, "Try 'rm --help' for more information.\n");
        return 1;
    }
    
    // force와 interactive 옵션 충돌 처리
    if (opts.force && opts.interactive) {
        opts.interactive = 0; // force가 우선
    }
    
    // 각 파일/디렉토리 삭제
    for (i = file_start; i < argc; i++) {
        if (rm_file(argv[i], &opts) == -1) {
            result = 1;
        }
    }
    
    return result;
}

// 컴파일 방법:
// gcc -o rm rm.c
//
// 사용 예시:
// ./rm file.txt
// ./rm -r directory/
// ./rm -f file.txt
// ./rm -i file.txt
// ./rm -rf directory/