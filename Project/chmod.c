#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>

#define MAX_PATH_LENGTH 4096

// chmod 옵션을 저장하는 구조체
typedef struct {
    int recursive;       // -R: 재귀적 적용
    int verbose;         // -v: 자세한 출력
    int changes_only;    // -c: 변경된 것만 출력
    int preserve_root;   // --preserve-root: 루트 디렉토리 보호
} ChmodOptions;

// 권한을 문자열로 변환
void mode_to_string(mode_t mode, char *str) {
    strcpy(str, "----------");
    
    // 파일 타입
    if (S_ISDIR(mode)) str[0] = 'd';
    else if (S_ISLNK(mode)) str[0] = 'l';
    else if (S_ISCHR(mode)) str[0] = 'c';
    else if (S_ISBLK(mode)) str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    
    // 사용자 권한
    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) str[3] = 'x';
    
    // 그룹 권한
    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) str[6] = 'x';
    
    // 기타 권한
    if (mode & S_IROTH) str[7] = 'r';
    if (mode & S_IWOTH) str[8] = 'w';
    if (mode & S_IXOTH) str[9] = 'x';
    
    // 특수 비트
    if (mode & S_ISUID) str[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID) str[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX) str[9] = (mode & S_IXOTH) ? 't' : 'T';
}

// 8진수 문자열을 mode_t로 변환
mode_t parse_octal_mode(const char *str) {
    mode_t mode = 0;
    int len = strlen(str);
    
    // 3자리 또는 4자리 8진수만 허용
    if (len < 3 || len > 4) {
        return (mode_t)-1;
    }
    
    // 모든 문자가 8진수인지 확인
    for (int i = 0; i < len; i++) {
        if (str[i] < '0' || str[i] > '7') {
            return (mode_t)-1;
        }
    }
    
    // 4자리인 경우 (특수 비트 포함)
    if (len == 4) {
        int special = str[0] - '0';
        if (special & 4) mode |= S_ISUID;  // setuid
        if (special & 2) mode |= S_ISGID;  // setgid
        if (special & 1) mode |= S_ISVTX;  // sticky bit
        str++;  // 다음 3자리로 이동
    }
    
    // 사용자 권한
    int user = str[0] - '0';
    if (user & 4) mode |= S_IRUSR;
    if (user & 2) mode |= S_IWUSR;
    if (user & 1) mode |= S_IXUSR;
    
    // 그룹 권한
    int group = str[1] - '0';
    if (group & 4) mode |= S_IRGRP;
    if (group & 2) mode |= S_IWGRP;
    if (group & 1) mode |= S_IXGRP;
    
    // 기타 권한
    int other = str[2] - '0';
    if (other & 4) mode |= S_IROTH;
    if (other & 2) mode |= S_IWOTH;
    if (other & 1) mode |= S_IXOTH;
    
    return mode;
}

// 심볼릭 모드 파싱 (예: u+x, go-w, a=r)
mode_t parse_symbolic_mode(const char *str, mode_t current_mode) {
    mode_t new_mode = current_mode;
    char *mode_str = strdup(str);
    char *token = strtok(mode_str, ",");
    
    while (token != NULL) {
        char *ptr = token;
        mode_t who_mask = 0;
        char op = 0;
        mode_t perm_mask = 0;
        
        // who 부분 파싱 (u, g, o, a)
        while (*ptr && strchr("ugoa", *ptr)) {
            switch (*ptr) {
                case 'u': who_mask |= S_IRWXU | S_ISUID; break;
                case 'g': who_mask |= S_IRWXG | S_ISGID; break;
                case 'o': who_mask |= S_IRWXO | S_ISVTX; break;
                case 'a': who_mask |= S_IRWXU | S_IRWXG | S_IRWXO; break;
            }
            ptr++;
        }
        
        // who가 지정되지 않으면 기본값은 'a' (all)
        if (who_mask == 0) {
            who_mask = S_IRWXU | S_IRWXG | S_IRWXO;
        }
        
        // 연산자 파싱 (+, -, =)
        if (*ptr && strchr("+-=", *ptr)) {
            op = *ptr++;
        } else {
            free(mode_str);
            return (mode_t)-1;  // 잘못된 형식
        }
        
        // 권한 부분 파싱 (r, w, x, s, t)
        while (*ptr) {
            switch (*ptr) {
                case 'r':
                    perm_mask |= (who_mask & S_IRWXU) ? S_IRUSR : 0;
                    perm_mask |= (who_mask & S_IRWXG) ? S_IRGRP : 0;
                    perm_mask |= (who_mask & S_IRWXO) ? S_IROTH : 0;
                    break;
                case 'w':
                    perm_mask |= (who_mask & S_IRWXU) ? S_IWUSR : 0;
                    perm_mask |= (who_mask & S_IRWXG) ? S_IWGRP : 0;
                    perm_mask |= (who_mask & S_IRWXO) ? S_IWOTH : 0;
                    break;
                case 'x':
                    perm_mask |= (who_mask & S_IRWXU) ? S_IXUSR : 0;
                    perm_mask |= (who_mask & S_IRWXG) ? S_IXGRP : 0;
                    perm_mask |= (who_mask & S_IRWXO) ? S_IXOTH : 0;
                    break;
                case 's':
                    perm_mask |= (who_mask & S_ISUID) ? S_ISUID : 0;
                    perm_mask |= (who_mask & S_ISGID) ? S_ISGID : 0;
                    break;
                case 't':
                    perm_mask |= (who_mask & S_ISVTX) ? S_ISVTX : 0;
                    break;
                default:
                    free(mode_str);
                    return (mode_t)-1;  // 잘못된 권한 문자
            }
            ptr++;
        }
        
        // 연산 수행
        switch (op) {
            case '+':
                new_mode |= perm_mask;
                break;
            case '-':
                new_mode &= ~perm_mask;
                break;
            case '=':
                new_mode = (new_mode & ~who_mask) | perm_mask;
                break;
        }
        
        token = strtok(NULL, ",");
    }
    
    free(mode_str);
    return new_mode;
}

// 파일/디렉토리 권한 변경
int chmod_file(const char *path, const char *mode_str, const ChmodOptions *opts) {
    struct stat st;
    
    // 현재 파일 정보 가져오기
    if (lstat(path, &st) != 0) {
        fprintf(stderr, "chmod: cannot access '%s': %s\n", path, strerror(errno));
        return -1;
    }
    
    mode_t old_mode = st.st_mode;
    mode_t new_mode;
    
    // 모드 파싱
    if (isdigit(mode_str[0])) {
        // 8진수 모드
        new_mode = parse_octal_mode(mode_str);
        if (new_mode == (mode_t)-1) {
            fprintf(stderr, "chmod: invalid mode: '%s'\n", mode_str);
            return -1;
        }
        // 파일 타입 비트는 유지
        new_mode |= (old_mode & S_IFMT);
    } else {
        // 심볼릭 모드
        new_mode = parse_symbolic_mode(mode_str, old_mode);
        if (new_mode == (mode_t)-1) {
            fprintf(stderr, "chmod: invalid mode: '%s'\n", mode_str);
            return -1;
        }
    }
    
    // 권한 변경
    if (chmod(path, new_mode) != 0) {
        fprintf(stderr, "chmod: changing permissions of '%s': %s\n", path, strerror(errno));
        return -1;
    }
    
    // 출력 옵션 처리
    if (opts->verbose || (opts->changes_only && old_mode != new_mode)) {
        char old_str[11], new_str[11];
        mode_to_string(old_mode, old_str);
        mode_to_string(new_mode, new_str);
        
        if (old_mode != new_mode) {
            printf("mode of '%s' changed from %04o (%s) to %04o (%s)\n",
                   path, old_mode & 07777, old_str, new_mode & 07777, new_str);
        } else if (opts->verbose) {
            printf("mode of '%s' retained as %04o (%s)\n",
                   path, new_mode & 07777, new_str);
        }
    }
    
    return 0;
}

// 디렉토리 재귀 처리
int chmod_recursive(const char *dir_path, const char *mode_str, const ChmodOptions *opts) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "chmod: cannot access '%s': %s\n", dir_path, strerror(errno));
        return -1;
    }
    
    struct dirent *entry;
    struct stat st;
    char full_path[MAX_PATH_LENGTH];
    int result = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // . 과 .. 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 전체 경로 생성
        int path_len = snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        if (path_len >= sizeof(full_path)) {
            fprintf(stderr, "chmod: path too long: %s/%s\n", dir_path, entry->d_name);
            continue;
        }
        
        // 파일 정보 가져오기
        if (lstat(full_path, &st) != 0) {
            fprintf(stderr, "chmod: cannot access '%s': %s\n", full_path, strerror(errno));
            result = -1;
            continue;
        }
        
        // 권한 변경
        if (chmod_file(full_path, mode_str, opts) != 0) {
            result = -1;
        }
        
        // 디렉토리면 재귀 처리 (심볼릭 링크 제외)
        if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
            if (chmod_recursive(full_path, mode_str, opts) != 0) {
                result = -1;
            }
        }
    }
    
    closedir(dir);
    return result;
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTION]... MODE[,MODE]... FILE...\n", prog_name);
    printf("       %s [OPTION]... OCTAL-MODE FILE...\n", prog_name);
    printf("Change the mode of each FILE to MODE.\n\n");
    printf("Options:\n");
    printf("  -c, --changes          like verbose but report only when a change is made\n");
    printf("  -R, --recursive        change files and directories recursively\n");
    printf("  -v, --verbose          output a diagnostic for every file processed\n");
    printf("      --preserve-root    fail to operate recursively on '/'\n");
    printf("  -h, --help             display this help and exit\n\n");
    printf("MODE is of the form '[ugoa]*([-+=]([rwxXst]*|[ugo]))+|[-+=][0-7]+'.\n\n");
    printf("Examples:\n");
    printf("  %s 755 file.txt          # Set permissions to 755\n", prog_name);
    printf("  %s u+x script.sh         # Add execute permission for user\n", prog_name);
    printf("  %s go-w file.txt         # Remove write permission for group and others\n", prog_name);
    printf("  %s a=r file.txt          # Set read-only for all\n", prog_name);
    printf("  %s -R 644 /path/to/dir   # Recursively set 644\n", prog_name);
}

int main(int argc, char *argv[]) {
    ChmodOptions opts = {0};
    
    static struct option long_options[] = {
        {"changes", no_argument, 0, 'c'},
        {"recursive", no_argument, 0, 'R'},
        {"verbose", no_argument, 0, 'v'},
        {"preserve-root", no_argument, 0, 1},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "cRvh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                opts.changes_only = 1;
                break;
            case 'R':
                opts.recursive = 1;
                break;
            case 'v':
                opts.verbose = 1;
                break;
            case 1:  // --preserve-root
                opts.preserve_root = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // 모드와 파일이 최소한 하나씩은 있어야 함
    if (optind + 1 >= argc) {
        fprintf(stderr, "chmod: missing operand\n");
        print_usage(argv[0]);
        return 1;
    }
    
    char *mode_str = argv[optind++];
    int exit_status = 0;
    
    // 각 파일에 대해 권한 변경
    for (int i = optind; i < argc; i++) {
        char *file_path = argv[i];
        
        // --preserve-root 옵션 확인
        if (opts.preserve_root && opts.recursive && strcmp(file_path, "/") == 0) {
            fprintf(stderr, "chmod: it is dangerous to operate recursively on '/'\n");
            fprintf(stderr, "chmod: use --no-preserve-root to override this failsafe\n");
            exit_status = 1;
            continue;
        }
        
        struct stat st;
        if (lstat(file_path, &st) != 0) {
            fprintf(stderr, "chmod: cannot access '%s': %s\n", file_path, strerror(errno));
            exit_status = 1;
            continue;
        }
        
        // 권한 변경
        if (chmod_file(file_path, mode_str, &opts) != 0) {
            exit_status = 1;
        }
        
        // 디렉토리이고 재귀 옵션이 켜져있으면 재귀 처리
        if (opts.recursive && S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
            if (chmod_recursive(file_path, mode_str, &opts) != 0) {
                exit_status = 1;
            }
        }
    }
    
    return exit_status;
}