# I. 파일 및 디렉토리 관리 (File and Directory Management)

## ls: 현재 디렉토리 파일 및 폴더 목록 출력
- -l: 상세 정보 (권한, 소유자, 크기, 수정 날짜)
- -a: 숨김 파일 포함
- -h: 용량을 사람이 읽기 쉬운 형식으로 (e.g., KB, MB)
- -R: 하위 디렉토리 재귀적 나열

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>

// 옵션 플래그 구조체
struct ls_options {
    int show_all;        // -a 옵션
    int long_format;     // -l 옵션
    int human_readable;  // -h 옵션
    int recursive;       // -R 옵션
};

// 파일 정보를 저장하는 구조체
struct file_info {
    char name[256];
    struct stat st;
    char *full_path;
};

// 함수 선언
void print_usage(const char *program_name);
void parse_options(int argc, char *argv[], struct ls_options *opts, char **directory);
void ls_directory(const char *path, struct ls_options *opts, int depth);
int compare_files(const void *a, const void *b);
void print_file_info(struct file_info *file, struct ls_options *opts);
void print_permissions(mode_t mode);
char *format_size(off_t size, int human_readable);
char *format_time(time_t time);

int main(int argc, char *argv[]) {
    struct ls_options opts = {0, 0, 0, 0};
    char *directory = ".";
    
    parse_options(argc, argv, &opts, &directory);
    ls_directory(directory, &opts, 0);
    
    return 0;
}

void print_usage(const char *program_name) {
    printf("사용법: %s [옵션] [디렉토리]\n", program_name);
    printf("옵션:\n");
    printf("  -a    숨김 파일 포함\n");
    printf("  -l    상세 정보 표시\n");
    printf("  -h    사람이 읽기 쉬운 크기 형식\n");
    printf("  -R    하위 디렉토리 재귀적 나열\n");
}

void parse_options(int argc, char *argv[], struct ls_options *opts, char **directory) {
    int opt;
    
    while ((opt = getopt(argc, argv, "alhR")) != -1) {
        switch (opt) {
            case 'a':
                opts->show_all = 1;
                break;
            case 'l':
                opts->long_format = 1;
                break;
            case 'h':
                opts->human_readable = 1;
                break;
            case 'R':
                opts->recursive = 1;
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    // 디렉토리 인수가 있으면 사용
    if (optind < argc) {
        *directory = argv[optind];
    }
}

void ls_directory(const char *path, struct ls_options *opts, int depth) {
    DIR *dir;
    struct dirent *entry;
    struct file_info *files = NULL;
    int file_count = 0;
    int capacity = 10;
    
    // 재귀 호출 시 디렉토리 경로 출력
    if (opts->recursive && depth > 0) {
        printf("\n%s:\n", path);
    }
    
    dir = opendir(path);
    if (dir == NULL) {
        perror(path);
        return;
    }
    
    // 파일 정보 저장을 위한 동적 배열 할당
    files = malloc(capacity * sizeof(struct file_info));
    if (files == NULL) {
        perror("메모리 할당 실패");
        closedir(dir);
        return;
    }
    
    // 디렉토리 내 파일들 읽기
    while ((entry = readdir(dir)) != NULL) {
        // 숨김 파일 처리
        if (!opts->show_all && entry->d_name[0] == '.') {
            continue;
        }
        
        // 배열 크기 확장
        if (file_count >= capacity) {
            capacity *= 2;
            files = realloc(files, capacity * sizeof(struct file_info));
            if (files == NULL) {
                perror("메모리 재할당 실패");
                closedir(dir);
                return;
            }
        }
        
        // 파일 정보 저장
        strcpy(files[file_count].name, entry->d_name);
        
        // 전체 경로 생성
        int path_len = strlen(path) + strlen(entry->d_name) + 2;
        files[file_count].full_path = malloc(path_len);
        snprintf(files[file_count].full_path, path_len, "%s/%s", path, entry->d_name);
        
        // stat 정보 가져오기
        if (stat(files[file_count].full_path, &files[file_count].st) == -1) {
            perror(files[file_count].full_path);
            free(files[file_count].full_path);
            continue;
        }
        
        file_count++;
    }
    
    closedir(dir);
    
    // 파일명으로 정렬
    qsort(files, file_count, sizeof(struct file_info), compare_files);
    
    // 파일 정보 출력
    for (int i = 0; i < file_count; i++) {
        print_file_info(&files[i], opts);
    }
    
    // 재귀 옵션이 활성화된 경우 하위 디렉토리 처리
    if (opts->recursive) {
        for (int i = 0; i < file_count; i++) {
            if (S_ISDIR(files[i].st.st_mode) && 
                strcmp(files[i].name, ".") != 0 && 
                strcmp(files[i].name, "..") != 0) {
                ls_directory(files[i].full_path, opts, depth + 1);
            }
        }
    }
    
    // 메모리 해제
    for (int i = 0; i < file_count; i++) {
        free(files[i].full_path);
    }
    free(files);
}

int compare_files(const void *a, const void *b) {
    struct file_info *file_a = (struct file_info *)a;
    struct file_info *file_b = (struct file_info *)b;
    return strcmp(file_a->name, file_b->name);
}

void print_file_info(struct file_info *file, struct ls_options *opts) {
    if (opts->long_format) {
        // 권한 출력
        print_permissions(file->st.st_mode);
        
        // 링크 수
        printf(" %2ld", file->st.st_nlink);
        
        // 소유자
        struct passwd *pw = getpwuid(file->st.st_uid);
        printf(" %-8s", pw ? pw->pw_name : "unknown");
        
        // 그룹
        struct group *gr = getgrgid(file->st.st_gid);
        printf(" %-8s", gr ? gr->gr_name : "unknown");
        
        // 크기
        char *size_str = format_size(file->st.st_size, opts->human_readable);
        printf(" %8s", size_str);
        free(size_str);
        
        // 수정 시간
        char *time_str = format_time(file->st.st_mtime);
        printf(" %s", time_str);
        free(time_str);
        
        // 파일명
        printf(" %s", file->name);
        
        // 심볼릭 링크인 경우 링크 대상 표시
        if (S_ISLNK(file->st.st_mode)) {
            char link_target[256];
            ssize_t len = readlink(file->full_path, link_target, sizeof(link_target) - 1);
            if (len != -1) {
                link_target[len] = '\0';
                printf(" -> %s", link_target);
            }
        }
        
        printf("\n");
    } else {
        // 간단한 형식으로 출력
        printf("%s\n", file->name);
    }
}

void print_permissions(mode_t mode) {
    char perms[11] = "----------";
    
    // 파일 타입
    if (S_ISDIR(mode)) perms[0] = 'd';
    else if (S_ISLNK(mode)) perms[0] = 'l';
    else if (S_ISCHR(mode)) perms[0] = 'c';
    else if (S_ISBLK(mode)) perms[0] = 'b';
    else if (S_ISFIFO(mode)) perms[0] = 'p';
    else if (S_ISSOCK(mode)) perms[0] = 's';
    
    // 소유자 권한
    if (mode & S_IRUSR) perms[1] = 'r';
    if (mode & S_IWUSR) perms[2] = 'w';
    if (mode & S_IXUSR) perms[3] = 'x';
    
    // 그룹 권한
    if (mode & S_IRGRP) perms[4] = 'r';
    if (mode & S_IWGRP) perms[5] = 'w';
    if (mode & S_IXGRP) perms[6] = 'x';
    
    // 기타 권한
    if (mode & S_IROTH) perms[7] = 'r';
    if (mode & S_IWOTH) perms[8] = 'w';
    if (mode & S_IXOTH) perms[9] = 'x';
    
    printf("%s", perms);
}

char *format_size(off_t size, int human_readable) {
    char *result = malloc(20);
    
    if (human_readable && size >= 1024) {
        const char *units[] = {"B", "K", "M", "G", "T"};
        int unit = 0;
        double size_d = size;
        
        while (size_d >= 1024 && unit < 4) {
            size_d /= 1024;
            unit++;
        }
        
        if (size_d >= 10) {
            snprintf(result, 20, "%.0f%s", size_d, units[unit]);
        } else {
            snprintf(result, 20, "%.1f%s", size_d, units[unit]);
        }
    } else {
        snprintf(result, 20, "%ld", size);
    }
    
    return result;
}

char *format_time(time_t time) {
    char *result = malloc(20);
    struct tm *tm_info = localtime(&time);
    
    // 현재 시간과 비교하여 6개월 이내면 시간 표시, 아니면 연도 표시
    time_t now = time(time);
    time_t six_months_ago = now - (6 * 30 * 24 * 60 * 60);
    
    if (time > six_months_ago && time <= now) {
        strftime(result, 20, "%b %d %H:%M", tm_info);
    } else {
        strftime(result, 20, "%b %d  %Y", tm_info);
    }
    
    return result;
}
```

## cd: 디렉토리 변경
- ..: 상위 디렉토리로 이동
- ~: 홈 디렉토리로 이동
- -: 이전 디렉토리로 이동
```
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
```

### ls+cd 통합
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

// ls 관련 구조체 및 함수들
struct ls_options {
    int show_all;
    int long_format;
    int human_readable;
    int recursive;
};

struct file_info {
    char name[256];
    struct stat st;
    char *full_path;
};

// cd 관련 전역 변수
static char previous_dir[PATH_MAX] = "";
static int has_previous = 0;

// 함수 선언
// 터미널 관련
void print_prompt(void);
char *read_command(void);
char **parse_command(char *input);
int execute_command(char **args);
void free_args(char **args);

// ls 관련
void ls_directory(const char *path, struct ls_options *opts, int depth);
void parse_ls_options(char **args, struct ls_options *opts, char **directory);
int compare_files(const void *a, const void *b);
void print_file_info(struct file_info *file, struct ls_options *opts);
void print_permissions(mode_t mode);
char *format_size(off_t size, int human_readable);
char *format_time(time_t time);

// cd 관련
char *get_home_directory(void);
char *resolve_path(const char *path);
int change_directory(const char *path);
void init_cd(void);

// 기타
void print_current_directory(void);

int main(void) {
    char *input;
    char **args;
    int status = 1;
    
    init_cd();  // cd 초기화
    
    printf("간단한 터미널 시작 (종료: exit)\n");
    printf("지원 명령어: ls, cd, pwd, exit\n\n");
    
    do {
        print_prompt();
        input = read_command();
        args = parse_command(input);
        status = execute_command(args);
        
        free(input);
        free_args(args);
    } while (status);
    
    return 0;
}

void print_prompt(void) {
    char *cwd = getcwd(NULL, 0);
    char *home = get_home_directory();
    char *display_path = cwd;
    
    // 홈 디렉토리면 ~ 표시
    if (home && cwd && strncmp(cwd, home, strlen(home)) == 0) {
        if (strlen(cwd) == strlen(home)) {
            display_path = "~";
        } else {
            static char short_path[PATH_MAX];
            snprintf(short_path, PATH_MAX, "~%s", cwd + strlen(home));
            display_path = short_path;
        }
    }
    
    printf("myshell:%s$ ", display_path ? display_path : "unknown");
    free(cwd);
}

char *read_command(void) {
    char *input = NULL;
    size_t bufsize = 0;
    
    if (getline(&input, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            printf("\n");
            exit(0);
        } else {
            perror("readline");
            exit(1);
        }
    }
    
    return input;
}

char **parse_command(char *input) {
    int bufsize = 64;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;
    
    if (!tokens) {
        fprintf(stderr, "메모리 할당 오류\n");
        exit(1);
    }
    
    token = strtok(input, " \t\r\n\a");
    while (token != NULL) {
        tokens[position] = malloc(strlen(token) + 1);
        strcpy(tokens[position], token);
        position++;
        
        if (position >= bufsize) {
            bufsize += 64;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "메모리 할당 오류\n");
                exit(1);
            }
        }
        
        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[position] = NULL;
    return tokens;
}

int execute_command(char **args) {
    if (args[0] == NULL) {
        return 1;  // 빈 명령어
    }
    
    if (strcmp(args[0], "exit") == 0) {
        return 0;  // 종료
    }
    
    if (strcmp(args[0], "cd") == 0) {
        if (change_directory(args[1]) != 0) {
            // 오류 메시지는 change_directory에서 출력됨
        }
        return 1;
    }
    
    if (strcmp(args[0], "pwd") == 0) {
        print_current_directory();
        return 1;
    }
    
    if (strcmp(args[0], "ls") == 0) {
        struct ls_options opts = {0, 0, 0, 0};
        char *directory = ".";
        parse_ls_options(args, &opts, &directory);
        ls_directory(directory, &opts, 0);
        return 1;
    }
    
    printf("%s: 명령을 찾을 수 없습니다\n", args[0]);
    return 1;
}

void free_args(char **args) {
    if (args) {
        for (int i = 0; args[i]; i++) {
            free(args[i]);
        }
        free(args);
    }
}

// cd 관련 함수들
void init_cd(void) {
    char *pwd = getenv("PWD");
    if (pwd != NULL) {
        strcpy(previous_dir, pwd);
        has_previous = 1;
    }
}

char *get_home_directory(void) {
    char *home = getenv("HOME");
    if (home != NULL) {
        return home;
    }
    
    struct passwd *pw = getpwuid(getuid());
    if (pw != NULL) {
        return pw->pw_dir;
    }
    
    return NULL;
}

char *resolve_path(const char *path) {
    static char resolved[PATH_MAX];
    
    if (path == NULL) {
        char *home = get_home_directory();
        if (home == NULL) {
            fprintf(stderr, "cd: 홈 디렉토리를 찾을 수 없습니다\n");
            return NULL;
        }
        strcpy(resolved, home);
        return resolved;
    }
    
    if (strcmp(path, "~") == 0) {
        char *home = get_home_directory();
        if (home == NULL) {
            fprintf(stderr, "cd: 홈 디렉토리를 찾을 수 없습니다\n");
            return NULL;
        }
        strcpy(resolved, home);
        return resolved;
    }
    
    if (strcmp(path, "-") == 0) {
        if (!has_previous) {
            fprintf(stderr, "cd: OLDPWD가 설정되지 않았습니다\n");
            return NULL;
        }
        strcpy(resolved, previous_dir);
        printf("%s\n", resolved);
        return resolved;
    }
    
    if (path[0] == '~' && path[1] == '/') {
        char *home = get_home_directory();
        if (home == NULL) {
            fprintf(stderr, "cd: 홈 디렉토리를 찾을 수 없습니다\n");
            return NULL;
        }
        snprintf(resolved, PATH_MAX, "%s%s", home, path + 1);
        return resolved;
    }
    
    if (realpath(path, resolved) == NULL) {
        strcpy(resolved, path);
    }
    
    return resolved;
}

int change_directory(const char *path) {
    char current_dir[PATH_MAX];
    char *target_path;
    
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        perror("cd: 현재 디렉토리를 가져올 수 없습니다");
        return -1;
    }
    
    target_path = resolve_path(path);
    if (target_path == NULL) {
        return -1;
    }
    
    struct stat st;
    if (stat(target_path, &st) != 0) {
        perror(target_path);
        return -1;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "cd: %s: 디렉토리가 아닙니다\n", target_path);
        return -1;
    }
    
    if (chdir(target_path) != 0) {
        perror(target_path);
        return -1;
    }
    
    strcpy(previous_dir, current_dir);
    has_previous = 1;
    
    setenv("OLDPWD", current_dir, 1);
    
    char new_pwd[PATH_MAX];
    if (getcwd(new_pwd, sizeof(new_pwd)) != NULL) {
        setenv("PWD", new_pwd, 1);
    }
    
    return 0;
}

// ls 관련 함수들
void parse_ls_options(char **args, struct ls_options *opts, char **directory) {
    for (int i = 1; args[i] != NULL; i++) {
        if (args[i][0] == '-') {
            for (int j = 1; args[i][j] != '\0'; j++) {
                switch (args[i][j]) {
                    case 'a':
                        opts->show_all = 1;
                        break;
                    case 'l':
                        opts->long_format = 1;
                        break;
                    case 'h':
                        opts->human_readable = 1;
                        break;
                    case 'R':
                        opts->recursive = 1;
                        break;
                    default:
                        printf("ls: 알 수 없는 옵션 '-%c'\n", args[i][j]);
                        break;
                }
            }
        } else {
            *directory = args[i];
        }
    }
}

void ls_directory(const char *path, struct ls_options *opts, int depth) {
    DIR *dir;
    struct dirent *entry;
    struct file_info *files = NULL;
    int file_count = 0;
    int capacity = 10;
    
    if (opts->recursive && depth > 0) {
        printf("\n%s:\n", path);
    }
    
    dir = opendir(path);
    if (dir == NULL) {
        perror(path);
        return;
    }
    
    files = malloc(capacity * sizeof(struct file_info));
    if (files == NULL) {
        perror("메모리 할당 실패");
        closedir(dir);
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (!opts->show_all && entry->d_name[0] == '.') {
            continue;
        }
        
        if (file_count >= capacity) {
            capacity *= 2;
            files = realloc(files, capacity * sizeof(struct file_info));
            if (files == NULL) {
                perror("메모리 재할당 실패");
                closedir(dir);
                return;
            }
        }
        
        strcpy(files[file_count].name, entry->d_name);
        
        int path_len = strlen(path) + strlen(entry->d_name) + 2;
        files[file_count].full_path = malloc(path_len);
        snprintf(files[file_count].full_path, path_len, "%s/%s", path, entry->d_name);
        
        if (stat(files[file_count].full_path, &files[file_count].st) == -1) {
            perror(files[file_count].full_path);
            free(files[file_count].full_path);
            continue;
        }
        
        file_count++;
    }
    
    closedir(dir);
    
    qsort(files, file_count, sizeof(struct file_info), compare_files);
    
    for (int i = 0; i < file_count; i++) {
        print_file_info(&files[i], opts);
    }
    
    if (opts->recursive) {
        for (int i = 0; i < file_count; i++) {
            if (S_ISDIR(files[i].st.st_mode) && 
                strcmp(files[i].name, ".") != 0 && 
                strcmp(files[i].name, "..") != 0) {
                ls_directory(files[i].full_path, opts, depth + 1);
            }
        }
    }
    
    for (int i = 0; i < file_count; i++) {
        free(files[i].full_path);
    }
    free(files);
}

int compare_files(const void *a, const void *b) {
    struct file_info *file_a = (struct file_info *)a;
    struct file_info *file_b = (struct file_info *)b;
    return strcmp(file_a->name, file_b->name);
}

void print_file_info(struct file_info *file, struct ls_options *opts) {
    if (opts->long_format) {
        print_permissions(file->st.st_mode);
        
        printf(" %2ld", file->st.st_nlink);
        
        struct passwd *pw = getpwuid(file->st.st_uid);
        printf(" %-8s", pw ? pw->pw_name : "unknown");
        
        struct group *gr = getgrgid(file->st.st_gid);
        printf(" %-8s", gr ? gr->gr_name : "unknown");
        
        char *size_str = format_size(file->st.st_size, opts->human_readable);
        printf(" %8s", size_str);
        free(size_str);
        
        char *time_str = format_time(file->st.st_mtime);
        printf(" %s", time_str);
        free(time_str);
        
        printf(" %s", file->name);
        
        if (S_ISLNK(file->st.st_mode)) {
            char link_target[256];
            ssize_t len = readlink(file->full_path, link_target, sizeof(link_target) - 1);
            if (len != -1) {
                link_target[len] = '\0';
                printf(" -> %s", link_target);
            }
        }
        
        printf("\n");
    } else {
        printf("%s\n", file->name);
    }
}

void print_permissions(mode_t mode) {
    char perms[11] = "----------";
    
    if (S_ISDIR(mode)) perms[0] = 'd';
    else if (S_ISLNK(mode)) perms[0] = 'l';
    else if (S_ISCHR(mode)) perms[0] = 'c';
    else if (S_ISBLK(mode)) perms[0] = 'b';
    else if (S_ISFIFO(mode)) perms[0] = 'p';
    else if (S_ISSOCK(mode)) perms[0] = 's';
    
    if (mode & S_IRUSR) perms[1] = 'r';
    if (mode & S_IWUSR) perms[2] = 'w';
    if (mode & S_IXUSR) perms[3] = 'x';
    
    if (mode & S_IRGRP) perms[4] = 'r';
    if (mode & S_IWGRP) perms[5] = 'w';
    if (mode & S_IXGRP) perms[6] = 'x';
    
    if (mode & S_IROTH) perms[7] = 'r';
    if (mode & S_IWOTH) perms[8] = 'w';
    if (mode & S_IXOTH) perms[9] = 'x';
    
    printf("%s", perms);
}

char *format_size(off_t size, int human_readable) {
    char *result = malloc(20);
    
    if (human_readable && size >= 1024) {
        const char *units[] = {"B", "K", "M", "G", "T"};
        int unit = 0;
        double size_d = size;
        
        while (size_d >= 1024 && unit < 4) {
            size_d /= 1024;
            unit++;
        }
        
        if (size_d >= 10) {
            snprintf(result, 20, "%.0f%s", size_d, units[unit]);
        } else {
            snprintf(result, 20, "%.1f%s", size_d, units[unit]);
        }
    } else {
        snprintf(result, 20, "%ld", size);
    }
    
    return result;
}

char *format_time(time_t time) {
    char *result = malloc(20);
    struct tm *tm_info = localtime(&time);
    
    time_t now = time(time);
    time_t six_months_ago = now - (6 * 30 * 24 * 60 * 60);
    
    if (time > six_months_ago && time <= now) {
        strftime(result, 20, "%b %d %H:%M", tm_info);
    } else {
        strftime(result, 20, "%b %d  %Y", tm_info);
    }
    
    return result;
}

void print_current_directory(void) {
    char current_dir[PATH_MAX];
    if (getcwd(current_dir, sizeof(current_dir)) != NULL) {
        printf("%s\n", current_dir);
    } else {
        perror("pwd");
    }
}
```

## pwd: 현재 작업 디렉토리 경로 출력 
```
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
```

### 통합 ls+cd+pwd
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

// ls 관련 구조체 및 함수들
struct ls_options {
    int show_all;
    int long_format;
    int human_readable;
    int recursive;
};

struct file_info {
    char name[256];
    struct stat st;
    char *full_path;
};

// cd 관련 전역 변수
static char previous_dir[PATH_MAX] = "";
static int has_previous = 0;

// 함수 선언
// 터미널 관련
void print_prompt(void);
char *read_command(void);
char **parse_command(char *input);
int execute_command(char **args);
void free_args(char **args);

// ls 관련
void ls_directory(const char *path, struct ls_options *opts, int depth);
void parse_ls_options(char **args, struct ls_options *opts, char **directory);
int compare_files(const void *a, const void *b);
void print_file_info(struct file_info *file, struct ls_options *opts);
void print_permissions(mode_t mode);
char *format_size(off_t size, int human_readable);
char *format_time(time_t time);

// cd 관련
char *get_home_directory(void);
char *resolve_path(const char *path);
int change_directory(const char *path);
void init_cd(void);

// 기타
void print_current_directory(void);

int main(void) {
    char *input;
    char **args;
    int status = 1;
    
    init_cd();  // cd 초기화
    
    printf("간단한 터미널 시작 (종료: exit)\n");
    printf("지원 명령어: ls, cd, pwd, exit\n\n");
    
    do {
        print_prompt();
        input = read_command();
        args = parse_command(input);
        status = execute_command(args);
        
        free(input);
        free_args(args);
    } while (status);
    
    return 0;
}

void print_prompt(void) {
    char *cwd = getcwd(NULL, 0);
    char *home = get_home_directory();
    char *display_path = cwd;
    
    // 홈 디렉토리면 ~ 표시
    if (home && cwd && strncmp(cwd, home, strlen(home)) == 0) {
        if (strlen(cwd) == strlen(home)) {
            display_path = "~";
        } else {
            static char short_path[PATH_MAX];
            snprintf(short_path, PATH_MAX, "~%s", cwd + strlen(home));
            display_path = short_path;
        }
    }
    
    printf("myshell:%s$ ", display_path ? display_path : "unknown");
    free(cwd);
}

char *read_command(void) {
    char *input = NULL;
    size_t bufsize = 0;
    
    if (getline(&input, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            printf("\n");
            exit(0);
        } else {
            perror("readline");
            exit(1);
        }
    }
    
    return input;
}

char **parse_command(char *input) {
    int bufsize = 64;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;
    
    if (!tokens) {
        fprintf(stderr, "메모리 할당 오류\n");
        exit(1);
    }
    
    token = strtok(input, " \t\r\n\a");
    while (token != NULL) {
        tokens[position] = malloc(strlen(token) + 1);
        strcpy(tokens[position], token);
        position++;
        
        if (position >= bufsize) {
            bufsize += 64;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "메모리 할당 오류\n");
                exit(1);
            }
        }
        
        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[position] = NULL;
    return tokens;
}

int execute_command(char **args) {
    if (args[0] == NULL) {
        return 1;  // 빈 명령어
    }
    
    if (strcmp(args[0], "exit") == 0) {
        return 0;  // 종료
    }
    
    if (strcmp(args[0], "cd") == 0) {
        if (change_directory(args[1]) != 0) {
            // 오류 메시지는 change_directory에서 출력됨
        }
        return 1;
    }
    
    if (strcmp(args[0], "pwd") == 0) {
        int physical = 0;
        // -P 옵션 확인
        if (args[1] != NULL && strcmp(args[1], "-P") == 0) {
            physical = 1;
        } else if (args[1] != NULL && strcmp(args[1], "-L") == 0) {
            physical = 0;
        } else if (args[1] != NULL && strcmp(args[1], "--help") == 0) {
            printf("사용법: pwd [옵션]\n");
            printf("  -L    논리적 경로 출력 (기본값)\n");
            printf("  -P    물리적 경로 출력\n");
            return 1;
        }
        
        char *pwd_result = terminal_pwd(physical);
        if (pwd_result) {
            printf("%s\n", pwd_result);
            free(pwd_result);
        } else {
            fprintf(stderr, "pwd: 현재 디렉토리를 가져올 수 없습니다\n");
        }
        return 1;
    }
    
    if (strcmp(args[0], "ls") == 0) {
        struct ls_options opts = {0, 0, 0, 0};
        char *directory = ".";
        parse_ls_options(args, &opts, &directory);
        ls_directory(directory, &opts, 0);
        return 1;
    }
    
    printf("%s: 명령을 찾을 수 없습니다\n", args[0]);
    return 1;
}

void free_args(char **args) {
    if (args) {
        for (int i = 0; args[i]; i++) {
            free(args[i]);
        }
        free(args);
    }
}

// cd 관련 함수들
void init_cd(void) {
    char *pwd = getenv("PWD");
    if (pwd != NULL) {
        strcpy(previous_dir, pwd);
        has_previous = 1;
    }
}

char *get_home_directory(void) {
    char *home = getenv("HOME");
    if (home != NULL) {
        return home;
    }
    
    struct passwd *pw = getpwuid(getuid());
    if (pw != NULL) {
        return pw->pw_dir;
    }
    
    return NULL;
}

char *resolve_path(const char *path) {
    static char resolved[PATH_MAX];
    
    if (path == NULL) {
        char *home = get_home_directory();
        if (home == NULL) {
            fprintf(stderr, "cd: 홈 디렉토리를 찾을 수 없습니다\n");
            return NULL;
        }
        strcpy(resolved, home);
        return resolved;
    }
    
    if (strcmp(path, "~") == 0) {
        char *home = get_home_directory();
        if (home == NULL) {
            fprintf(stderr, "cd: 홈 디렉토리를 찾을 수 없습니다\n");
            return NULL;
        }
        strcpy(resolved, home);
        return resolved;
    }
    
    if (strcmp(path, "-") == 0) {
        if (!has_previous) {
            fprintf(stderr, "cd: OLDPWD가 설정되지 않았습니다\n");
            return NULL;
        }
        strcpy(resolved, previous_dir);
        printf("%s\n", resolved);
        return resolved;
    }
    
    if (path[0] == '~' && path[1] == '/') {
        char *home = get_home_directory();
        if (home == NULL) {
            fprintf(stderr, "cd: 홈 디렉토리를 찾을 수 없습니다\n");
            return NULL;
        }
        snprintf(resolved, PATH_MAX, "%s%s", home, path + 1);
        return resolved;
    }
    
    if (realpath(path, resolved) == NULL) {
        strcpy(resolved, path);
    }
    
    return resolved;
}

int change_directory(const char *path) {
    char current_dir[PATH_MAX];
    char *target_path;
    
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        perror("cd: 현재 디렉토리를 가져올 수 없습니다");
        return -1;
    }
    
    target_path = resolve_path(path);
    if (target_path == NULL) {
        return -1;
    }
    
    struct stat st;
    if (stat(target_path, &st) != 0) {
        perror(target_path);
        return -1;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "cd: %s: 디렉토리가 아닙니다\n", target_path);
        return -1;
    }
    
    if (chdir(target_path) != 0) {
        perror(target_path);
        return -1;
    }
    
    strcpy(previous_dir, current_dir);
    has_previous = 1;
    
    setenv("OLDPWD", current_dir, 1);
    
    char new_pwd[PATH_MAX];
    if (getcwd(new_pwd, sizeof(new_pwd)) != NULL) {
        setenv("PWD", new_pwd, 1);
    }
    
    return 0;
}

// ls 관련 함수들
void parse_ls_options(char **args, struct ls_options *opts, char **directory) {
    for (int i = 1; args[i] != NULL; i++) {
        if (args[i][0] == '-') {
            for (int j = 1; args[i][j] != '\0'; j++) {
                switch (args[i][j]) {
                    case 'a':
                        opts->show_all = 1;
                        break;
                    case 'l':
                        opts->long_format = 1;
                        break;
                    case 'h':
                        opts->human_readable = 1;
                        break;
                    case 'R':
                        opts->recursive = 1;
                        break;
                    default:
                        printf("ls: 알 수 없는 옵션 '-%c'\n", args[i][j]);
                        break;
                }
            }
        } else {
            *directory = args[i];
        }
    }
}

void ls_directory(const char *path, struct ls_options *opts, int depth) {
    DIR *dir;
    struct dirent *entry;
    struct file_info *files = NULL;
    int file_count = 0;
    int capacity = 10;
    
    if (opts->recursive && depth > 0) {
        printf("\n%s:\n", path);
    }
    
    dir = opendir(path);
    if (dir == NULL) {
        perror(path);
        return;
    }
    
    files = malloc(capacity * sizeof(struct file_info));
    if (files == NULL) {
        perror("메모리 할당 실패");
        closedir(dir);
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (!opts->show_all && entry->d_name[0] == '.') {
            continue;
        }
        
        if (file_count >= capacity) {
            capacity *= 2;
            files = realloc(files, capacity * sizeof(struct file_info));
            if (files == NULL) {
                perror("메모리 재할당 실패");
                closedir(dir);
                return;
            }
        }
        
        strcpy(files[file_count].name, entry->d_name);
        
        int path_len = strlen(path) + strlen(entry->d_name) + 2;
        files[file_count].full_path = malloc(path_len);
        snprintf(files[file_count].full_path, path_len, "%s/%s", path, entry->d_name);
        
        if (stat(files[file_count].full_path, &files[file_count].st) == -1) {
            perror(files[file_count].full_path);
            free(files[file_count].full_path);
            continue;
        }
        
        file_count++;
    }
    
    closedir(dir);
    
    qsort(files, file_count, sizeof(struct file_info), compare_files);
    
    for (int i = 0; i < file_count; i++) {
        print_file_info(&files[i], opts);
    }
    
    if (opts->recursive) {
        for (int i = 0; i < file_count; i++) {
            if (S_ISDIR(files[i].st.st_mode) && 
                strcmp(files[i].name, ".") != 0 && 
                strcmp(files[i].name, "..") != 0) {
                ls_directory(files[i].full_path, opts, depth + 1);
            }
        }
    }
    
    for (int i = 0; i < file_count; i++) {
        free(files[i].full_path);
    }
    free(files);
}

int compare_files(const void *a, const void *b) {
    struct file_info *file_a = (struct file_info *)a;
    struct file_info *file_b = (struct file_info *)b;
    return strcmp(file_a->name, file_b->name);
}

void print_file_info(struct file_info *file, struct ls_options *opts) {
    if (opts->long_format) {
        print_permissions(file->st.st_mode);
        
        printf(" %2ld", file->st.st_nlink);
        
        struct passwd *pw = getpwuid(file->st.st_uid);
        printf(" %-8s", pw ? pw->pw_name : "unknown");
        
        struct group *gr = getgrgid(file->st.st_gid);
        printf(" %-8s", gr ? gr->gr_name : "unknown");
        
        char *size_str = format_size(file->st.st_size, opts->human_readable);
        printf(" %8s", size_str);
        free(size_str);
        
        char *time_str = format_time(file->st.st_mtime);
        printf(" %s", time_str);
        free(time_str);
        
        printf(" %s", file->name);
        
        if (S_ISLNK(file->st.st_mode)) {
            char link_target[256];
            ssize_t len = readlink(file->full_path, link_target, sizeof(link_target) - 1);
            if (len != -1) {
                link_target[len] = '\0';
                printf(" -> %s", link_target);
            }
        }
        
        printf("\n");
    } else {
        printf("%s\n", file->name);
    }
}

void print_permissions(mode_t mode) {
    char perms[11] = "----------";
    
    if (S_ISDIR(mode)) perms[0] = 'd';
    else if (S_ISLNK(mode)) perms[0] = 'l';
    else if (S_ISCHR(mode)) perms[0] = 'c';
    else if (S_ISBLK(mode)) perms[0] = 'b';
    else if (S_ISFIFO(mode)) perms[0] = 'p';
    else if (S_ISSOCK(mode)) perms[0] = 's';
    
    if (mode & S_IRUSR) perms[1] = 'r';
    if (mode & S_IWUSR) perms[2] = 'w';
    if (mode & S_IXUSR) perms[3] = 'x';
    
    if (mode & S_IRGRP) perms[4] = 'r';
    if (mode & S_IWGRP) perms[5] = 'w';
    if (mode & S_IXGRP) perms[6] = 'x';
    
    if (mode & S_IROTH) perms[7] = 'r';
    if (mode & S_IWOTH) perms[8] = 'w';
    if (mode & S_IXOTH) perms[9] = 'x';
    
    printf("%s", perms);
}

char *format_size(off_t size, int human_readable) {
    char *result = malloc(20);
    
    if (human_readable && size >= 1024) {
        const char *units[] = {"B", "K", "M", "G", "T"};
        int unit = 0;
        double size_d = size;
        
        while (size_d >= 1024 && unit < 4) {
            size_d /= 1024;
            unit++;
        }
        
        if (size_d >= 10) {
            snprintf(result, 20, "%.0f%s", size_d, units[unit]);
        } else {
            snprintf(result, 20, "%.1f%s", size_d, units[unit]);
        }
    } else {
        snprintf(result, 20, "%ld", size);
    }
    
    return result;
}

char *format_time(time_t time) {
    char *result = malloc(20);
    struct tm *tm_info = localtime(&time);
    
    time_t now = time(time);
    time_t six_months_ago = now - (6 * 30 * 24 * 60 * 60);
    
    if (time > six_months_ago && time <= now) {
        strftime(result, 20, "%b %d %H:%M", tm_info);
    } else {
        strftime(result, 20, "%b %d  %Y", tm_info);
    }
    
    return result;
}

void print_current_directory(void) {
    char *pwd_result = terminal_pwd(0);  // 논리적 경로 사용
    if (pwd_result) {
        printf("%s\n", pwd_result);
        free(pwd_result);
    } else {
        perror("pwd");
    }
}

// pwd 관련 함수들
char *get_logical_pwd(void) {
    char *pwd_env = getenv("PWD");
    char *physical_pwd = NULL;
    
    if (pwd_env == NULL) {
        return NULL;
    }
    
    physical_pwd = get_physical_pwd();
    if (physical_pwd == NULL) {
        return NULL;
    }
    
    if (is_same_directory(pwd_env, physical_pwd)) {
        free(physical_pwd);
        char *result = malloc(strlen(pwd_env) + 1);
        if (result != NULL) {
            strcpy(result, pwd_env);
        }
        return result;
    } else {
        return physical_pwd;
    }
}

char *get_physical_pwd(void) {
    return getcwd(NULL, 0);
}

int is_same_directory(const char *path1, const char *path2) {
    struct stat st1, st2;
    
    if (stat(path1, &st1) != 0 || stat(path2, &st2) != 0) {
        return 0;
    }
    
    return (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino);
}

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
```

## mkdir: 새 디렉토리 생성
- -p: 상위 디렉토리가 없으면 함께 생성


```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

// 디렉토리 생성 함수 (단일 디렉토리)
int create_directory(const char *path) {
    if (mkdir(path, 0755) == 0) {
        printf("디렉토리 '%s' 생성 완료\n", path);
        return 0;
    } else {
        if (errno == EEXIST) {
            printf("디렉토리 '%s'는 이미 존재합니다\n", path);
            return 0;  // -p 옵션에서는 이미 존재해도 성공으로 처리
        } else {
            perror("mkdir");
            return -1;
        }
    }
}

// 상위 디렉토리까지 재귀적으로 생성하는 함수
int create_directory_recursive(const char *path) {
    char *path_copy = strdup(path);
    char *p = path_copy;
    
    // 절대 경로인 경우 첫 번째 '/'를 건너뛰기
    if (*p == '/') {
        p++;
    }
    
    // 경로를 '/'로 분할하며 순차적으로 디렉토리 생성
    while ((p = strchr(p, '/'))) {
        *p = '\0';  // 임시로 문자열 종료
        
        // 현재 경로까지 디렉토리 생성
        if (mkdir(path_copy, 0755) == -1 && errno != EEXIST) {
            perror("mkdir");
            free(path_copy);
            return -1;
        }
        
        *p = '/';   // 다시 '/' 복원
        p++;
    }
    
    // 최종 디렉토리 생성
    int result = create_directory(path);
    free(path_copy);
    return result;
}

// mkdir 명령어 구현
int cmd_mkdir(int argc, char *argv[]) {
    int p_flag = 0;  // -p 옵션 플래그
    int i;
    
    // 인수가 부족한 경우
    if (argc < 2) {
        printf("사용법: mkdir [-p] <디렉토리명> [디렉토리명...]\n");
        return -1;
    }
    
    // 옵션 파싱
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            p_flag = 1;
        } else {
            printf("알 수 없는 옵션: %s\n", argv[i]);
            printf("사용법: mkdir [-p] <디렉토리명> [디렉토리명...]\n");
            return -1;
        }
    }
    
    // 디렉토리명이 제공되지 않은 경우
    if (i >= argc) {
        printf("디렉토리명을 입력해주세요\n");
        printf("사용법: mkdir [-p] <디렉토리명> [디렉토리명...]\n");
        return -1;
    }
    
    // 각 디렉토리 생성
    for (; i < argc; i++) {
        if (p_flag) {
            // -p 옵션: 상위 디렉토리도 함께 생성
            if (create_directory_recursive(argv[i]) == -1) {
                printf("디렉토리 '%s' 생성 실패\n", argv[i]);
            }
        } else {
            // 일반 모드: 해당 디렉토리만 생성
            if (create_directory(argv[i]) == -1) {
                printf("디렉토리 '%s' 생성 실패\n", argv[i]);
            }
        }
    }
    
    return 0;
}

// 테스트용 메인 함수
int main(int argc, char *argv[]) {
    printf("=== mkdir 명령어 테스트 ===\n");
    
    // 명령행 인수가 있으면 그대로 실행
    if (argc > 1) {
        return cmd_mkdir(argc, argv);
    }
    
    // 테스트 케이스들
    printf("\n1. 단일 디렉토리 생성 테스트:\n");
    char *test1[] = {"mkdir", "test_dir"};
    cmd_mkdir(2, test1);
    
    printf("\n2. 여러 디렉토리 생성 테스트:\n");
    char *test2[] = {"mkdir", "dir1", "dir2", "dir3"};
    cmd_mkdir(4, test2);
    
    printf("\n3. -p 옵션으로 중첩 디렉토리 생성 테스트:\n");
    char *test3[] = {"mkdir", "-p", "parent/child/grandchild"};
    cmd_mkdir(3, test3);
    
    printf("\n4. -p 옵션으로 여러 중첩 디렉토리 생성 테스트:\n");
    char *test4[] = {"mkdir", "-p", "a/b/c", "x/y/z"};
    cmd_mkdir(4, test4);
    
    printf("\n5. 잘못된 사용법 테스트:\n");
    char *test5[] = {"mkdir"};
    cmd_mkdir(1, test5);
    
    return 0;
}
```

## rmdir: 빈 디렉토리 삭제 
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

// 디렉토리가 비어있는지 확인하는 함수
int is_directory_empty(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return -1;  // 디렉토리를 열 수 없음
    }
    
    struct dirent *entry;
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // "."과 ".."은 제외하고 카운트
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }
    
    closedir(dir);
    return (count == 0) ? 1 : 0;  // 1: 비어있음, 0: 비어있지 않음
}

// 디렉토리가 존재하는지 확인하는 함수
int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

// 단일 디렉토리 삭제 함수
int remove_directory(const char *path) {
    // 경로가 존재하는지 확인
    if (access(path, F_OK) != 0) {
        printf("rmdir: '%s': 그런 파일이나 디렉토리가 없습니다\n", path);
        return -1;
    }
    
    // 디렉토리인지 확인
    if (!is_directory(path)) {
        printf("rmdir: '%s': 디렉토리가 아닙니다\n", path);
        return -1;
    }
    
    // 디렉토리가 비어있는지 확인
    int empty_check = is_directory_empty(path);
    if (empty_check == -1) {
        printf("rmdir: '%s': 디렉토리를 읽을 수 없습니다\n", path);
        return -1;
    } else if (empty_check == 0) {
        printf("rmdir: '%s': 디렉토리가 비어있지 않습니다\n", path);
        return -1;
    }
    
    // 디렉토리 삭제
    if (rmdir(path) == 0) {
        printf("디렉토리 '%s' 삭제 완료\n", path);
        return 0;
    } else {
        switch (errno) {
            case ENOTEMPTY:
                printf("rmdir: '%s': 디렉토리가 비어있지 않습니다\n", path);
                break;
            case EACCES:
                printf("rmdir: '%s': 권한이 거부되었습니다\n", path);
                break;
            case EBUSY:
                printf("rmdir: '%s': 디렉토리가 사용 중입니다\n", path);
                break;
            case EINVAL:
                printf("rmdir: '%s': 잘못된 인수입니다\n", path);
                break;
            case ENOTDIR:
                printf("rmdir: '%s': 디렉토리가 아닙니다\n", path);
                break;
            default:
                perror("rmdir");
                break;
        }
        return -1;
    }
}

// rmdir 명령어 구현
int cmd_rmdir(int argc, char *argv[]) {
    int success_count = 0;
    int total_count = 0;
    
    // 인수가 부족한 경우
    if (argc < 2) {
        printf("사용법: rmdir <디렉토리명> [디렉토리명...]\n");
        return -1;
    }
    
    // 각 디렉토리에 대해 삭제 시도
    for (int i = 1; i < argc; i++) {
        // 옵션 처리 (현재는 옵션 없음)
        if (argv[i][0] == '-') {
            printf("rmdir: 알 수 없는 옵션 '%s'\n", argv[i]);
            printf("사용법: rmdir <디렉토리명> [디렉토리명...]\n");
            continue;
        }
        
        total_count++;
        if (remove_directory(argv[i]) == 0) {
            success_count++;
        }
    }
    
    // 결과 요약
    if (total_count > 1) {
        printf("\n총 %d개 디렉토리 중 %d개 삭제 완료\n", total_count, success_count);
    }
    
    return (success_count == total_count) ? 0 : -1;
}

// 현재 디렉토리의 내용을 보여주는 도우미 함수 (테스트용)
void show_directory_contents(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        printf("디렉토리 '%s'를 열 수 없습니다\n", path);
        return;
    }
    
    printf("디렉토리 '%s'의 내용:\n", path);
    struct dirent *entry;
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            printf("  - %s\n", entry->d_name);
            count++;
        }
    }
    
    if (count == 0) {
        printf("  (비어있음)\n");
    }
    
    closedir(dir);
}

// 테스트용 메인 함수
int main(int argc, char *argv[]) {
    printf("=== rmdir 명령어 테스트 ===\n");
    
    // 명령행 인수가 있으면 그대로 실행
    if (argc > 1) {
        return cmd_rmdir(argc, argv);
    }
    
    // 테스트를 위한 디렉토리 생성
    printf("\n테스트용 디렉토리 생성 중...\n");
    system("mkdir -p test_empty");
    system("mkdir -p test_nonempty");
    system("touch test_nonempty/file.txt");  // 비어있지 않은 디렉토리 생성
    system("mkdir -p dir1 dir2 dir3");
    
    printf("\n1. 비어있는 디렉토리 삭제 테스트:\n");
    char *test1[] = {"rmdir", "test_empty"};
    cmd_rmdir(2, test1);
    
    printf("\n2. 비어있지 않은 디렉토리 삭제 테스트 (실패해야 함):\n");
    show_directory_contents("test_nonempty");
    char *test2[] = {"rmdir", "test_nonempty"};
    cmd_rmdir(2, test2);
    
    printf("\n3. 여러 디렉토리 삭제 테스트:\n");
    char *test3[] = {"rmdir", "dir1", "dir2", "dir3"};
    cmd_rmdir(4, test3);
    
    printf("\n4. 존재하지 않는 디렉토리 삭제 테스트 (실패해야 함):\n");
    char *test4[] = {"rmdir", "nonexistent_dir"};
    cmd_rmdir(2, test4);
    
    printf("\n5. 잘못된 사용법 테스트:\n");
    char *test5[] = {"rmdir"};
    cmd_rmdir(1, test5);
    
    // 정리
    printf("\n테스트 정리 중...\n");
    system("rm -rf test_nonempty");  // 테스트용 파일 정리
    
    return 0;
}
```

## touch: 새 파일 생성 또는 파일의 접근/수정 시간 업데이트 
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <utime.h>

// 파일이 존재하는지 확인하는 함수
int file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

// 새 파일을 생성하는 함수
int create_file(const char *filename) {
    int fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        perror("touch: 파일 생성 실패");
        return -1;
    }
    close(fd);
    return 0;
}

// 파일의 접근/수정 시간을 현재 시간으로 업데이트하는 함수
int update_file_time(const char *filename) {
    if (utime(filename, NULL) == -1) {
        perror("touch: 시간 업데이트 실패");
        return -1;
    }
    return 0;
}

// 파일의 상태 정보를 출력하는 함수 (테스트용)
void show_file_info(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        printf("파일 '%s'의 정보를 가져올 수 없습니다\n", filename);
        return;
    }
    
    printf("파일: %s\n", filename);
    printf("  크기: %ld 바이트\n", st.st_size);
    printf("  접근 시간: %s", ctime(&st.st_atime));
    printf("  수정 시간: %s", ctime(&st.st_mtime));
    printf("  상태 변경 시간: %s", ctime(&st.st_ctime));
}

// 단일 파일 처리 함수
int touch_file(const char *filename) {
    if (file_exists(filename)) {
        // 파일이 존재하면 시간 업데이트
        if (update_file_time(filename) == 0) {
            printf("파일 '%s'의 시간을 업데이트했습니다\n", filename);
            return 0;
        }
    } else {
        // 파일이 존재하지 않으면 새로 생성
        if (create_file(filename) == 0) {
            printf("새 파일 '%s'을 생성했습니다\n", filename);
            return 0;
        }
    }
    return -1;
}

// 시간 문자열을 파싱하는 함수 (간단한 형태만 지원: YYYYMMDDHHMM)
int parse_time_string(const char *time_str, struct tm *tm_time) {
    if (strlen(time_str) != 12) {
        return -1;
    }
    
    // YYYYMMDDHHMM 형식 파싱
    char year[5], month[3], day[3], hour[3], minute[3];
    
    strncpy(year, time_str, 4); year[4] = '\0';
    strncpy(month, time_str + 4, 2); month[2] = '\0';
    strncpy(day, time_str + 6, 2); day[2] = '\0';
    strncpy(hour, time_str + 8, 2); hour[2] = '\0';
    strncpy(minute, time_str + 10, 2); minute[2] = '\0';
    
    tm_time->tm_year = atoi(year) - 1900;
    tm_time->tm_mon = atoi(month) - 1;
    tm_time->tm_mday = atoi(day);
    tm_time->tm_hour = atoi(hour);
    tm_time->tm_min = atoi(minute);
    tm_time->tm_sec = 0;
    tm_time->tm_isdst = -1;
    
    return 0;
}

// 지정된 시간으로 파일 시간을 설정하는 함수
int set_file_time(const char *filename, const char *time_str) {
    struct tm tm_time;
    if (parse_time_string(time_str, &tm_time) == -1) {
        printf("touch: 잘못된 시간 형식입니다 (YYYYMMDDHHMM 형식을 사용하세요)\n");
        return -1;
    }
    
    time_t new_time = mktime(&tm_time);
    if (new_time == -1) {
        printf("touch: 유효하지 않은 시간입니다\n");
        return -1;
    }
    
    struct utimbuf times;
    times.actime = new_time;
    times.modtime = new_time;
    
    if (utime(filename, &times) == -1) {
        perror("touch: 시간 설정 실패");
        return -1;
    }
    
    return 0;
}

// touch 명령어 구현
int cmd_touch(int argc, char *argv[]) {
    int c_flag = 0;  // -c: 파일이 없어도 생성하지 않음
    int t_flag = 0;  // -t: 특정 시간으로 설정
    char *time_str = NULL;
    int success_count = 0;
    int total_count = 0;
    
    // 인수가 부족한 경우
    if (argc < 2) {
        printf("사용법: touch [-c] [-t YYYYMMDDHHMM] <파일명> [파일명...]\n");
        printf("  -c: 파일이 존재하지 않아도 생성하지 않음\n");
        printf("  -t: 지정된 시간으로 설정 (YYYYMMDDHHMM 형식)\n");
        return -1;
    }
    
    // 옵션 파싱
    int i;
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            c_flag = 1;
        } else if (strcmp(argv[i], "-t") == 0) {
            if (i + 1 >= argc) {
                printf("touch: -t 옵션에는 시간 인수가 필요합니다\n");
                return -1;
            }
            t_flag = 1;
            time_str = argv[++i];
        } else {
            printf("touch: 알 수 없는 옵션 '%s'\n", argv[i]);
            printf("사용법: touch [-c] [-t YYYYMMDDHHMM] <파일명> [파일명...]\n");
            return -1;
        }
    }
    
    // 파일명이 제공되지 않은 경우
    if (i >= argc) {
        printf("touch: 파일명을 입력해주세요\n");
        return -1;
    }
    
    // 각 파일 처리
    for (; i < argc; i++) {
        total_count++;
        
        if (c_flag && !file_exists(argv[i])) {
            // -c 옵션: 파일이 없으면 생성하지 않음
            printf("touch: '%s' 파일이 존재하지 않습니다 (-c 옵션)\n", argv[i]);
            continue;
        }
        
        // 파일이 존재하지 않으면 생성
        if (!file_exists(argv[i])) {
            if (create_file(argv[i]) == -1) {
                printf("touch: '%s' 파일 생성 실패\n", argv[i]);
                continue;
            }
            printf("새 파일 '%s'을 생성했습니다\n", argv[i]);
        }
        
        // 시간 설정
        if (t_flag) {
            // 지정된 시간으로 설정
            if (set_file_time(argv[i], time_str) == 0) {
                printf("파일 '%s'의 시간을 설정했습니다\n", argv[i]);
                success_count++;
            }
        } else {
            // 현재 시간으로 업데이트
            if (update_file_time(argv[i]) == 0) {
                printf("파일 '%s'의 시간을 업데이트했습니다\n", argv[i]);
                success_count++;
            }
        }
    }
    
    // 결과 요약
    if (total_count > 1) {
        printf("\n총 %d개 파일 중 %d개 처리 완료\n", total_count, success_count);
    }
    
    return (success_count == total_count) ? 0 : -1;
}

// 테스트용 메인 함수
int main(int argc, char *argv[]) {
    printf("=== touch 명령어 테스트 ===\n");
    
    // 명령행 인수가 있으면 그대로 실행
    if (argc > 1) {
        return cmd_touch(argc, argv);
    }
    
    printf("\n1. 새 파일 생성 테스트:\n");
    char *test1[] = {"touch", "test_new_file.txt"};
    cmd_touch(2, test1);
    
    printf("\n파일 정보 확인:\n");
    show_file_info("test_new_file.txt");
    
    printf("\n2. 기존 파일 시간 업데이트 테스트:\n");
    printf("2초 대기 후 시간 업데이트...\n");
    sleep(2);
    char *test2[] = {"touch", "test_new_file.txt"};
    cmd_touch(2, test2);
    
    printf("\n업데이트된 파일 정보:\n");
    show_file_info("test_new_file.txt");
    
    printf("\n3. 여러 파일 생성 테스트:\n");
    char *test3[] = {"touch", "file1.txt", "file2.txt", "file3.txt"};
    cmd_touch(4, test3);
    
    printf("\n4. -c 옵션 테스트 (존재하지 않는 파일):\n");
    char *test4[] = {"touch", "-c", "nonexistent.txt"};
    cmd_touch(3, test4);
    
    printf("\n5. -t 옵션 테스트 (특정 시간 설정):\n");
    char *test5[] = {"touch", "-t", "202312251430", "test_new_file.txt"};
    cmd_touch(4, test5);
    
    printf("\n시간 설정 후 파일 정보:\n");
    show_file_info("test_new_file.txt");
    
    printf("\n6. 잘못된 사용법 테스트:\n");
    char *test6[] = {"touch"};
    cmd_touch(1, test6);
    
    // 정리
    printf("\n테스트 파일 정리 중...\n");
    system("rm -f test_new_file.txt file1.txt file2.txt file3.txt");
    
    return 0;
}
```

## cp: 파일 또는 디렉토리 복사
- -r / -R: 디렉토리 재귀적 복사
- -f: 강제 덮어쓰기
- -i: 덮어쓰기 전 확인

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>

#define BUFFER_SIZE 4096

// 옵션 구조체
typedef struct {
    int recursive;    // -r, -R 옵션
    int force;        // -f 옵션
    int interactive;  // -i 옵션
} cp_options;

// 파일인지 디렉토리인지 확인하는 함수
int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

// 파일이 존재하는지 확인하는 함수
int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

// 사용자에게 확인을 요청하는 함수
int ask_user_confirmation(const char *message) {
    printf("%s (y/n): ", message);
    char response;
    scanf(" %c", &response);
    return (response == 'y' || response == 'Y');
}

// 단일 파일 복사 함수
int copy_file(const char *src, const char *dest, cp_options *opts) {
    int src_fd, dest_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    struct stat src_stat;
    
    // 소스 파일 열기
    src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        printf("cp: '%s'를 열 수 없습니다: %s\n", src, strerror(errno));
        return -1;
    }
    
    // 소스 파일 정보 가져오기
    if (fstat(src_fd, &src_stat) == -1) {
        printf("cp: '%s'의 정보를 가져올 수 없습니다: %s\n", src, strerror(errno));
        close(src_fd);
        return -1;
    }
    
    // 대상 파일이 존재하는 경우 처리
    if (file_exists(dest)) {
        if (opts->interactive && !opts->force) {
            char msg[512];
            snprintf(msg, sizeof(msg), "cp: '%s'를 덮어쓰시겠습니까?", dest);
            if (!ask_user_confirmation(msg)) {
                printf("cp: '%s' 복사를 건너뜁니다\n", dest);
                close(src_fd);
                return 0;
            }
        } else if (!opts->force && !opts->interactive) {
            printf("cp: '%s'가 이미 존재합니다 (-f 옵션 없음)\n", dest);
            close(src_fd);
            return -1;
        }
    }
    
    // 대상 파일 생성/열기
    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);
    if (dest_fd == -1) {
        printf("cp: '%s'를 생성할 수 없습니다: %s\n", dest, strerror(errno));
        close(src_fd);
        return -1;
    }
    
    // 파일 내용 복사
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            printf("cp: '%s' 쓰기 오류: %s\n", dest, strerror(errno));
            close(src_fd);
            close(dest_fd);
            unlink(dest);  // 실패한 파일 삭제
            return -1;
        }
    }
    
    if (bytes_read == -1) {
        printf("cp: '%s' 읽기 오류: %s\n", src, strerror(errno));
        close(src_fd);
        close(dest_fd);
        unlink(dest);
        return -1;
    }
    
    close(src_fd);
    close(dest_fd);
    
    // 파일 권한 복사
    chmod(dest, src_stat.st_mode);
    
    printf("'%s' -> '%s'\n", src, dest);
    return 0;
}

// 경로 결합 함수
char* join_path(const char *dir, const char *file) {
    size_t dir_len = strlen(dir);
    size_t file_len = strlen(file);
    char *result = malloc(dir_len + file_len + 2);
    
    strcpy(result, dir);
    if (dir_len > 0 && dir[dir_len - 1] != '/') {
        strcat(result, "/");
    }
    strcat(result, file);
    
    return result;
}

// 디렉토리 재귀적 복사 함수
int copy_directory(const char *src, const char *dest, cp_options *opts) {
    DIR *dir;
    struct dirent *entry;
    struct stat src_stat;
    char *src_path, *dest_path;
    int result = 0;
    
    // 소스 디렉토리 열기
    dir = opendir(src);
    if (dir == NULL) {
        printf("cp: '%s' 디렉토리를 열 수 없습니다: %s\n", src, strerror(errno));
        return -1;
    }
    
    // 소스 디렉토리 정보 가져오기
    if (stat(src, &src_stat) == -1) {
        printf("cp: '%s'의 정보를 가져올 수 없습니다: %s\n", src, strerror(errno));
        closedir(dir);
        return -1;
    }
    
    // 대상 디렉토리 생성
    if (!file_exists(dest)) {
        if (mkdir(dest, src_stat.st_mode) == -1) {
            printf("cp: '%s' 디렉토리를 생성할 수 없습니다: %s\n", dest, strerror(errno));
            closedir(dir);
            return -1;
        }
        printf("디렉토리 '%s' 생성\n", dest);
    }
    
    // 디렉토리 내용 복사
    while ((entry = readdir(dir)) != NULL) {
        // "."과 ".." 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        src_path = join_path(src, entry->d_name);
        dest_path = join_path(dest, entry->d_name);
        
        if (is_directory(src_path)) {
            // 하위 디렉토리 재귀적 복사
            if (copy_directory(src_path, dest_path, opts) == -1) {
                result = -1;
            }
        } else {
            // 파일 복사
            if (copy_file(src_path, dest_path, opts) == -1) {
                result = -1;
            }
        }
        
        free(src_path);
        free(dest_path);
    }
    
    closedir(dir);
    return result;
}

// 대상 경로 생성 함수 (파일명 추출)
char* build_dest_path(const char *src, const char *dest) {
    if (is_directory(dest)) {
        // 대상이 디렉토리인 경우, 소스 파일명을 추가
        char *src_copy = strdup(src);
        char *filename = basename(src_copy);
        char *result = join_path(dest, filename);
        free(src_copy);
        return result;
    } else {
        // 대상이 파일인 경우, 그대로 사용
        return strdup(dest);
    }
}

// cp 명령어 구현
int cmd_cp(int argc, char *argv[]) {
    cp_options opts = {0, 0, 0};  // recursive, force, interactive
    int i;
    
    // 인수가 부족한 경우
    if (argc < 3) {
        printf("사용법: cp [-r|-R] [-f] [-i] <소스> [소스...] <대상>\n");
        printf("  -r, -R: 디렉토리 재귀적 복사\n");
        printf("  -f: 강제 덮어쓰기\n");
        printf("  -i: 덮어쓰기 전 확인\n");
        return -1;
    }
    
    // 옵션 파싱
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        char *opt = argv[i];
        if (strcmp(opt, "-r") == 0 || strcmp(opt, "-R") == 0) {
            opts.recursive = 1;
        } else if (strcmp(opt, "-f") == 0) {
            opts.force = 1;
        } else if (strcmp(opt, "-i") == 0) {
            opts.interactive = 1;
        } else if (strcmp(opt, "-rf") == 0 || strcmp(opt, "-fr") == 0 ||
                   strcmp(opt, "-Rf") == 0 || strcmp(opt, "-fR") == 0) {
            opts.recursive = 1;
            opts.force = 1;
        } else if (strcmp(opt, "-ri") == 0 || strcmp(opt, "-ir") == 0 ||
                   strcmp(opt, "-Ri") == 0 || strcmp(opt, "-iR") == 0) {
            opts.recursive = 1;
            opts.interactive = 1;
        } else if (strcmp(opt, "-fi") == 0 || strcmp(opt, "-if") == 0) {
            opts.force = 1;
            opts.interactive = 1;
        } else {
            printf("cp: 알 수 없는 옵션 '%s'\n", opt);
            return -1;
        }
    }
    
    // 소스와 대상 확인
    if (i >= argc - 1) {
        printf("cp: 소스와 대상을 모두 지정해야 합니다\n");
        return -1;
    }
    
    char *dest = argv[argc - 1];  // 마지막 인수가 대상
    int success_count = 0;
    int total_count = argc - i - 1;
    
    // 각 소스 파일/디렉토리 복사
    for (; i < argc - 1; i++) {
        char *src = argv[i];
        
        // 소스 존재 확인
        if (!file_exists(src)) {
            printf("cp: '%s'가 존재하지 않습니다\n", src);
            continue;
        }
        
        // 대상 경로 생성
        char *actual_dest = build_dest_path(src, dest);
        
        if (is_directory(src)) {
            if (!opts.recursive) {
                printf("cp: '%s'는 디렉토리입니다 (-r 옵션이 필요합니다)\n", src);
                free(actual_dest);
                continue;
            }
            
            if (copy_directory(src, actual_dest, &opts) == 0) {
                success_count++;
            }
        } else {
            if (copy_file(src, actual_dest, &opts) == 0) {
                success_count++;
            }
        }
        
        free(actual_dest);
    }
    
    // 결과 요약
    if (total_count > 1) {
        printf("\n총 %d개 항목 중 %d개 복사 완료\n", total_count, success_count);
    }
    
    return (success_count == total_count) ? 0 : -1;
}

// 테스트용 메인 함수
int main(int argc, char *argv[]) {
    printf("=== cp 명령어 테스트 ===\n");
    
    // 명령행 인수가 있으면 그대로 실행
    if (argc > 1) {
        return cmd_cp(argc, argv);
    }
    
    // 테스트용 파일/디렉토리 생성
    printf("\n테스트 환경 설정 중...\n");
    system("echo 'Hello World' > test_file.txt");
    system("mkdir -p test_dir");
    system("echo 'File in directory' > test_dir/subfile.txt");
    system("mkdir -p test_dir/subdir");
    system("echo 'Nested file' > test_dir/subdir/nested.txt");
    
    printf("\n1. 단일 파일 복사 테스트:\n");
    char *test1[] = {"cp", "test_file.txt", "copy_file.txt"};
    cmd_cp(3, test1);
    
    printf("\n2. 파일을 디렉토리로 복사 테스트:\n");
    system("mkdir -p dest_dir");
    char *test2[] = {"cp", "test_file.txt", "dest_dir/"};
    cmd_cp(3, test2);
    
    printf("\n3. 디렉토리 재귀적 복사 테스트:\n");
    char *test3[] = {"cp", "-r", "test_dir", "copy_dir"};
    cmd_cp(4, test3);
    
    printf("\n4. 강제 덮어쓰기 테스트:\n");
    char *test4[] = {"cp", "-f", "test_file.txt", "copy_file.txt"};
    cmd_cp(4, test4);
    
    printf("\n5. 여러 파일 복사 테스트:\n");
    system("echo 'File 2' > file2.txt");
    system("echo 'File 3' > file3.txt");
    char *test5[] = {"cp", "file2.txt", "file3.txt", "dest_dir/"};
    cmd_cp(4, test5);
    
    printf("\n6. 잘못된 사용법 테스트:\n");
    char *test6[] = {"cp", "test_dir", "copy_fail"};  // -r 없이 디렉토리 복사
    cmd_cp(3, test6);
    
    printf("\n생성된 파일들 확인:\n");
    system("ls -la copy_file.txt dest_dir/ copy_dir/ 2>/dev/null");
    
    // 정리
    printf("\n테스트 파일 정리 중...\n");
    system("rm -rf test_file.txt test_dir copy_file.txt dest_dir copy_dir file2.txt file3.txt");
    
    return 0;
}
```

## mv: 파일 또는 디렉토리 이동 또는 이름 변경
- -f: 강제 덮어쓰기
- -i: 덮어쓰기 전 확인

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <fcntl.h>
#include <dirent.h>

#define BUFFER_SIZE 4096

// 옵션 구조체
typedef struct {
    int force;        // -f 옵션
    int interactive;  // -i 옵션
} mv_options;

// 파일인지 디렉토리인지 확인하는 함수
int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

// 파일이 존재하는지 확인하는 함수
int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

// 사용자에게 확인을 요청하는 함수
int ask_user_confirmation(const char *message) {
    printf("%s (y/n): ", message);
    char response;
    scanf(" %c", &response);
    return (response == 'y' || response == 'Y');
}

// 경로 결합 함수
char* join_path(const char *dir, const char *file) {
    size_t dir_len = strlen(dir);
    size_t file_len = strlen(file);
    char *result = malloc(dir_len + file_len + 2);
    
    strcpy(result, dir);
    if (dir_len > 0 && dir[dir_len - 1] != '/') {
        strcat(result, "/");
    }
    strcat(result, file);
    
    return result;
}

// 대상 경로 생성 함수
char* build_dest_path(const char *src, const char *dest) {
    if (is_directory(dest)) {
        // 대상이 디렉토리인 경우, 소스 파일명을 추가
        char *src_copy = strdup(src);
        char *filename = basename(src_copy);
        char *result = join_path(dest, filename);
        free(src_copy);
        return result;
    } else {
        // 대상이 파일인 경우, 그대로 사용
        return strdup(dest);
    }
}

// 파일 복사 함수 (크로스 파일시스템 이동용)
int copy_file_content(const char *src, const char *dest) {
    int src_fd, dest_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    struct stat src_stat;
    
    // 소스 파일 열기
    src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        printf("mv: '%s'를 열 수 없습니다: %s\n", src, strerror(errno));
        return -1;
    }
    
    // 소스 파일 정보 가져오기
    if (fstat(src_fd, &src_stat) == -1) {
        printf("mv: '%s'의 정보를 가져올 수 없습니다: %s\n", src, strerror(errno));
        close(src_fd);
        return -1;
    }
    
    // 대상 파일 생성/열기
    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);
    if (dest_fd == -1) {
        printf("mv: '%s'를 생성할 수 없습니다: %s\n", dest, strerror(errno));
        close(src_fd);
        return -1;
    }
    
    // 파일 내용 복사
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            printf("mv: '%s' 쓰기 오류: %s\n", dest, strerror(errno));
            close(src_fd);
            close(dest_fd);
            unlink(dest);
            return -1;
        }
    }
    
    if (bytes_read == -1) {
        printf("mv: '%s' 읽기 오류: %s\n", src, strerror(errno));
        close(src_fd);
        close(dest_fd);
        unlink(dest);
        return -1;
    }
    
    close(src_fd);
    close(dest_fd);
    
    // 파일 권한 복사
    chmod(dest, src_stat.st_mode);
    
    return 0;
}

// 디렉토리 재귀적 복사 함수
int copy_directory_recursive(const char *src, const char *dest) {
    DIR *dir;
    struct dirent *entry;
    struct stat src_stat;
    char *src_path, *dest_path;
    int result = 0;
    
    // 소스 디렉토리 열기
    dir = opendir(src);
    if (dir == NULL) {
        printf("mv: '%s' 디렉토리를 열 수 없습니다: %s\n", src, strerror(errno));
        return -1;
    }
    
    // 소스 디렉토리 정보 가져오기
    if (stat(src, &src_stat) == -1) {
        printf("mv: '%s'의 정보를 가져올 수 없습니다: %s\n", src, strerror(errno));
        closedir(dir);
        return -1;
    }
    
    // 대상 디렉토리 생성
    if (mkdir(dest, src_stat.st_mode) == -1) {
        printf("mv: '%s' 디렉토리를 생성할 수 없습니다: %s\n", dest, strerror(errno));
        closedir(dir);
        return -1;
    }
    
    // 디렉토리 내용 복사
    while ((entry = readdir(dir)) != NULL) {
        // "."과 ".." 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        src_path = join_path(src, entry->d_name);
        dest_path = join_path(dest, entry->d_name);
        
        if (is_directory(src_path)) {
            if (copy_directory_recursive(src_path, dest_path) == -1) {
                result = -1;
            }
        } else {
            if (copy_file_content(src_path, dest_path) == -1) {
                result = -1;
            }
        }
        
        free(src_path);
        free(dest_path);
    }
    
    closedir(dir);
    return result;
}

// 디렉토리 재귀적 삭제 함수
int remove_directory_recursive(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char *full_path;
    int result = 0;
    
    dir = opendir(path);
    if (dir == NULL) {
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        full_path = join_path(path, entry->d_name);
        
        if (is_directory(full_path)) {
            if (remove_directory_recursive(full_path) == -1) {
                result = -1;
            }
        } else {
            if (unlink(full_path) == -1) {
                result = -1;
            }
        }
        
        free(full_path);
    }
    
    closedir(dir);
    
    if (rmdir(path) == -1) {
        result = -1;
    }
    
    return result;
}

// 단일 파일/디렉토리 이동 함수
int move_item(const char *src, const char *dest, mv_options *opts) {
    struct stat src_stat, dest_stat;
    
    // 소스 존재 확인
    if (stat(src, &src_stat) == -1) {
        printf("mv: '%s'가 존재하지 않습니다: %s\n", src, strerror(errno));
        return -1;
    }
    
    // 대상이 존재하는 경우 처리
    if (stat(dest, &dest_stat) == 0) {
        // 같은 파일인지 확인
        if (src_stat.st_dev == dest_stat.st_dev && src_stat.st_ino == dest_stat.st_ino) {
            printf("mv: '%s'와 '%s'는 같은 파일입니다\n", src, dest);
            return -1;
        }
        
        // 덮어쓰기 확인
        if (opts->interactive && !opts->force) {
            char msg[512];
            snprintf(msg, sizeof(msg), "mv: '%s'를 덮어쓰시겠습니까?", dest);
            if (!ask_user_confirmation(msg)) {
                printf("mv: '%s' 이동을 건너뜁니다\n", src);
                return 0;
            }
        } else if (!opts->force && !opts->interactive) {
            printf("mv: '%s'가 이미 존재합니다 (-f 옵션 없음)\n", dest);
            return -1;
        }
        
        // 대상이 디렉토리이고 비어있지 않은 경우
        if (S_ISDIR(dest_stat.st_mode) && !S_ISDIR(src_stat.st_mode)) {
            printf("mv: '%s'는 디렉토리입니다\n", dest);
            return -1;
        }
    }
    
    // rename()으로 이동 시도 (같은 파일시스템 내에서)
    if (rename(src, dest) == 0) {
        printf("'%s' -> '%s'\n", src, dest);
        return 0;
    }
    
    // 크로스 파일시스템 이동인 경우 복사 후 삭제
    if (errno == EXDEV) {
        printf("크로스 파일시스템 이동: '%s' -> '%s'\n", src, dest);
        
        if (S_ISDIR(src_stat.st_mode)) {
            // 디렉토리 복사
            if (copy_directory_recursive(src, dest) == 0) {
                if (remove_directory_recursive(src) == 0) {
                    printf("'%s' -> '%s' (디렉토리)\n", src, dest);
                    return 0;
                } else {
                    printf("mv: 소스 디렉토리 '%s' 삭제 실패\n", src);
                    return -1;
                }
            }
        } else {
            // 파일 복사
            if (copy_file_content(src, dest) == 0) {
                if (unlink(src) == 0) {
                    printf("'%s' -> '%s'\n", src, dest);
                    return 0;
                } else {
                    printf("mv: 소스 파일 '%s' 삭제 실패\n", src);
                    return -1;
                }
            }
        }
    } else {
        printf("mv: '%s'를 '%s'로 이동할 수 없습니다: %s\n", src, dest, strerror(errno));
    }
    
    return -1;
}

// mv 명령어 구현
int cmd_mv(int argc, char *argv[]) {
    mv_options opts = {0, 0};  // force, interactive
    int i;
    
    // 인수가 부족한 경우
    if (argc < 3) {
        printf("사용법: mv [-f] [-i] <소스> [소스...] <대상>\n");
        printf("  -f: 강제 덮어쓰기\n");
        printf("  -i: 덮어쓰기 전 확인\n");
        return -1;
    }
    
    // 옵션 파싱
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        char *opt = argv[i];
        if (strcmp(opt, "-f") == 0) {
            opts.force = 1;
        } else if (strcmp(opt, "-i") == 0) {
            opts.interactive = 1;
        } else if (strcmp(opt, "-fi") == 0 || strcmp(opt, "-if") == 0) {
            opts.force = 1;
            opts.interactive = 1;
        } else {
            printf("mv: 알 수 없는 옵션 '%s'\n", opt);
            return -1;
        }
    }
    
    // 소스와 대상 확인
    if (i >= argc - 1) {
        printf("mv: 소스와 대상을 모두 지정해야 합니다\n");
        return -1;
    }
    
    char *dest = argv[argc - 1];  // 마지막 인수가 대상
    int success_count = 0;
    int total_count = argc - i - 1;
    
    // 여러 소스가 있는 경우 대상이 디렉토리여야 함
    if (total_count > 1 && !is_directory(dest)) {
        printf("mv: 여러 소스를 이동할 때 대상은 디렉토리여야 합니다\n");
        return -1;
    }
    
    // 각 소스 파일/디렉토리 이동
    for (; i < argc - 1; i++) {
        char *src = argv[i];
        
        // 소스 존재 확인
        if (!file_exists(src)) {
            printf("mv: '%s'가 존재하지 않습니다\n", src);
            continue;
        }
        
        // 대상 경로 생성
        char *actual_dest = build_dest_path(src, dest);
        
        if (move_item(src, actual_dest, &opts) == 0) {
            success_count++;
        }
        
        free(actual_dest);
    }
    
    // 결과 요약
    if (total_count > 1) {
        printf("\n총 %d개 항목 중 %d개 이동 완료\n", total_count, success_count);
    }
    
    return (success_count == total_count) ? 0 : -1;
}

// 테스트용 메인 함수
int main(int argc, char *argv[]) {
    printf("=== mv 명령어 테스트 ===\n");
    
    // 명령행 인수가 있으면 그대로 실행
    if (argc > 1) {
        return cmd_mv(argc, argv);
    }
    
    // 테스트용 파일/디렉토리 생성
    printf("\n테스트 환경 설정 중...\n");
    system("echo 'Original content' > original.txt");
    system("echo 'Test file 1' > test1.txt");
    system("echo 'Test file 2' > test2.txt");
    system("mkdir -p test_dir");
    system("echo 'File in directory' > test_dir/file.txt");
    system("mkdir -p dest_dir");
    
    printf("\n1. 파일 이름 변경 테스트:\n");
    char *test1[] = {"mv", "original.txt", "renamed.txt"};
    cmd_mv(3, test1);
    
    printf("\n2. 파일을 디렉토리로 이동 테스트:\n");
    char *test2[] = {"mv", "test1.txt", "dest_dir/"};
    cmd_mv(3, test2);
    
    printf("\n3. 디렉토리 이동 테스트:\n");
    char *test3[] = {"mv", "test_dir", "moved_dir"};
    cmd_mv(3, test3);
    
    printf("\n4. 여러 파일을 디렉토리로 이동 테스트:\n");
    system("echo 'File A' > fileA.txt");
    system("echo 'File B' > fileB.txt");
    char *test4[] = {"mv", "fileA.txt", "fileB.txt", "dest_dir/"};
    cmd_mv(4, test4);
    
    printf("\n5. 강제 덮어쓰기 테스트:\n");
    system("echo 'New content' > new_file.txt");
    system("echo 'Existing content' > dest_dir/existing.txt");
    char *test5[] = {"mv", "-f", "new_file.txt", "dest_dir/existing.txt"};
    cmd_mv(4, test5);
    
    printf("\n6. 잘못된 사용법 테스트:\n");
    char *test6[] = {"mv", "nonexistent.txt", "somewhere.txt"};
    cmd_mv(3, test6);
    
    printf("\n현재 디렉토리 상태:\n");
    system("ls -la");
    printf("\ndest_dir 내용:\n");
    system("ls -la dest_dir/");
    
    // 정리
    printf("\n테스트 파일 정리 중...\n");
    system("rm -rf renamed.txt moved_dir dest_dir test2.txt");
    
    return 0;
}
```

## rm: 파일 또는 디렉토리 삭제
- -r / -R: 디렉토리 재귀적 삭제
- -f: 강제 삭제 (경고 없이)
- -i: 삭제 전 확인
```
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
```

## cat: 파일 내용 출력 및 파일 연결
- -n: 줄 번호 출력
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

typedef struct {
    int number_lines;   // -n 옵션: 줄 번호 출력
} cat_options;

// 함수 선언
int cat_file(const char *filename, cat_options *opts, int *line_number);
int cat_stdin(cat_options *opts, int *line_number);
void print_usage(void);

// 사용법 출력
void print_usage(void) {
    printf("Usage: cat [OPTION]... [FILE]...\n");
    printf("Concatenate FILE(s) to standard output.\n\n");
    printf("With no FILE, or when FILE is -, read standard input.\n\n");
    printf("  -n, --number     number all output lines\n");
    printf("      --help       display this help and exit\n");
    printf("\nExamples:\n");
    printf("  cat f - g        Output f's contents, then standard input, then g's contents.\n");
    printf("  cat              Copy standard input to standard output.\n");
}

// 단일 파일의 내용을 출력하는 함수
int cat_file(const char *filename, cat_options *opts, int *line_number) {
    FILE *file;
    int ch;
    int at_line_start = 1;  // 줄의 시작인지 확인
    
    // 파일명이 "-"이면 표준 입력 사용
    if (strcmp(filename, "-") == 0) {
        return cat_stdin(opts, line_number);
    }
    
    // 파일 열기
    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "cat: %s: %s\n", filename, strerror(errno));
        return -1;
    }
    
    // 파일이 디렉토리인지 확인
    struct stat st;
    if (fstat(fileno(file), &st) == 0 && S_ISDIR(st.st_mode)) {
        fprintf(stderr, "cat: %s: Is a directory\n", filename);
        fclose(file);
        return -1;
    }
    
    // 파일 내용 읽기 및 출력
    while ((ch = fgetc(file)) != EOF) {
        // 줄 번호 출력 (줄의 시작에서)
        if (opts->number_lines && at_line_start) {
            printf("%6d\t", (*line_number)++);
            at_line_start = 0;
        }
        
        // 문자 출력
        putchar(ch);
        
        // 개행 문자면 다음 줄의 시작으로 표시
        if (ch == '\n') {
            at_line_start = 1;
        }
    }
    
    fclose(file);
    return 0;
}

// 표준 입력에서 읽어서 출력하는 함수
int cat_stdin(cat_options *opts, int *line_number) {
    int ch;
    int at_line_start = 1;  // 줄의 시작인지 확인
    
    // 표준 입력에서 읽기
    while ((ch = getchar()) != EOF) {
        // 줄 번호 출력 (줄의 시작에서)
        if (opts->number_lines && at_line_start) {
            printf("%6d\t", (*line_number)++);
            at_line_start = 0;
        }
        
        // 문자 출력
        putchar(ch);
        
        // 개행 문자면 다음 줄의 시작으로 표시
        if (ch == '\n') {
            at_line_start = 1;
        }
    }
    
    return 0;
}

// 옵션 파싱 함수
int parse_options(int argc, char *argv[], cat_options *opts, int *file_start) {
    int i;
    
    // 옵션 초기화
    opts->number_lines = 0;
    
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            break; // 옵션이 아닌 첫 번째 인수
        }
        
        // "--" 처리
        if (strcmp(argv[i], "--") == 0) {
            i++;
            break; // 옵션 끝
        }
        
        // 긴 옵션들
        if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 1;
        }
        
        if (strcmp(argv[i], "--number") == 0) {
            opts->number_lines = 1;
            continue;
        }
        
        // "-" 단독으로 사용되면 표준 입력을 의미
        if (strcmp(argv[i], "-") == 0) {
            break;
        }
        
        // 짧은 옵션들 처리
        int j;
        for (j = 1; argv[i][j] != '\0'; j++) {
            switch (argv[i][j]) {
                case 'n':
                    opts->number_lines = 1;
                    break;
                default:
                    fprintf(stderr, "cat: invalid option -- '%c'\n", argv[i][j]);
                    fprintf(stderr, "Try 'cat --help' for more information.\n");
                    return -1;
            }
        }
    }
    
    *file_start = i;
    return 0;
}

int main(int argc, char *argv[]) {
    cat_options opts;
    int file_start;
    int result = 0;
    int line_number = 1;  // 줄 번호 (전체 파일에 걸쳐 연속)
    int i;
    
    // 옵션 파싱
    int parse_result = parse_options(argc, argv, &opts, &file_start);
    if (parse_result == 1) {
        return 0; // help 출력 후 정상 종료
    }
    if (parse_result == -1) {
        return 1; // 옵션 파싱 오류
    }
    
    // 파일 인수가 없으면 표준 입력 사용
    if (file_start >= argc) {
        if (cat_stdin(&opts, &line_number) == -1) {
            result = 1;
        }
    } else {
        // 각 파일 처리
        for (i = file_start; i < argc; i++) {
            if (cat_file(argv[i], &opts, &line_number) == -1) {
                result = 1;
            }
        }
    }
    
    return result;
}

// 컴파일 방법:
// gcc -o cat cat.c
//
// 사용 예시:
// ./cat file.txt                    # 파일 내용 출력
// ./cat -n file.txt                 # 줄 번호와 함께 출력
// ./cat file1.txt file2.txt         # 여러 파일 연결하여 출력
// ./cat -n file1.txt file2.txt      # 여러 파일 줄 번호와 함께 출력
// ./cat < input.txt                 # 표준 입력에서 읽기
// ./cat                             # 표준 입력을 표준 출력으로 복사 (Ctrl+D로 종료)
// echo "Hello World" | ./cat -n     # 파이프를 통한 입력
```

## more / less: 파일 내용을 페이지 단위로 출력 (스크롤 가능) 
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#define MAX_LINE_LENGTH 1024
#define DEFAULT_LINES 24
#define DEFAULT_COLS 80

typedef struct {
    int lines;          // 터미널 높이
    int cols;           // 터미널 너비
    int current_line;   // 현재 줄 번호
    int total_lines;    // 전체 줄 수
    char **content;     // 파일 내용 저장
    char *filename;     // 현재 파일명
} more_state;

// 전역 변수
static struct termios original_termios;
static int termios_saved = 0;

// 함수 선언
void setup_terminal(void);
void restore_terminal(void);
void get_terminal_size(int *rows, int *cols);
int load_file_content(const char *filename, more_state *state);
void display_page(more_state *state);
void display_status(more_state *state);
int handle_input(more_state *state);
void free_content(more_state *state);
void print_usage(void);
void signal_handler(int sig);
char get_char(void);

// 신호 핸들러
void signal_handler(int sig) {
    restore_terminal();
    exit(0);
}

// 사용법 출력
void print_usage(void) {
    printf("Usage: more [file...]\n");
    printf("View file contents page by page.\n\n");
    printf("Commands while viewing:\n");
    printf("  SPACE     Display next page\n");
    printf("  ENTER     Display next line\n");
    printf("  q         Quit\n");
    printf("  h         Show this help\n");
    printf("  b         Go back one page\n");
    printf("  f         Go forward one page\n");
    printf("  /pattern  Search for pattern\n");
    printf("  n         Find next occurrence\n");
    printf("  g         Go to beginning\n");
    printf("  G         Go to end\n");
}

// 터미널 설정
void setup_terminal(void) {
    struct termios new_termios;
    
    // 현재 터미널 설정 저장
    if (tcgetattr(STDIN_FILENO, &original_termios) == 0) {
        termios_saved = 1;
        
        // 새로운 설정 복사
        new_termios = original_termios;
        
        // canonical 모드 비활성화, echo 비활성화
        new_termios.c_lflag &= ~(ICANON | ECHO);
        new_termios.c_cc[VMIN] = 1;
        new_termios.c_cc[VTIME] = 0;
        
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    }
    
    // 신호 핸들러 설정
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

// 터미널 복원
void restore_terminal(void) {
    if (termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
        termios_saved = 0;
    }
}

// 터미널 크기 얻기
void get_terminal_size(int *rows, int *cols) {
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
    } else {
        *rows = DEFAULT_LINES;
        *cols = DEFAULT_COLS;
    }
    
    // 상태 표시를 위해 한 줄 빼기
    if (*rows > 1) {
        (*rows)--;
    }
}

// 문자 하나 읽기 (non-blocking)
char get_char(void) {
    char ch;
    if (read(STDIN_FILENO, &ch, 1) == 1) {
        return ch;
    }
    return 0;
}

// 파일 내용 로드
int load_file_content(const char *filename, more_state *state) {
    FILE *file;
    char line[MAX_LINE_LENGTH];
    int capacity = 1000;  // 초기 용량
    int line_count = 0;
    
    // 파일명이 "-"이면 표준 입력
    if (strcmp(filename, "-") == 0) {
        file = stdin;
        state->filename = "stdin";
    } else {
        file = fopen(filename, "r");
        if (file == NULL) {
            fprintf(stderr, "more: %s: %s\n", filename, strerror(errno));
            return -1;
        }
        state->filename = strdup(filename);
    }
    
    // 파일이 디렉토리인지 확인
    if (file != stdin) {
        struct stat st;
        if (fstat(fileno(file), &st) == 0 && S_ISDIR(st.st_mode)) {
            fprintf(stderr, "more: %s: Is a directory\n", filename);
            fclose(file);
            return -1;
        }
    }
    
    // 메모리 할당
    state->content = malloc(capacity * sizeof(char*));
    if (state->content == NULL) {
        fprintf(stderr, "more: Memory allocation failed\n");
        if (file != stdin) fclose(file);
        return -1;
    }
    
    // 파일 내용 읽기
    while (fgets(line, sizeof(line), file) != NULL) {
        // 용량 확장 필요시
        if (line_count >= capacity) {
            capacity *= 2;
            char **new_content = realloc(state->content, capacity * sizeof(char*));
            if (new_content == NULL) {
                fprintf(stderr, "more: Memory allocation failed\n");
                free_content(state);
                if (file != stdin) fclose(file);
                return -1;
            }
            state->content = new_content;
        }
        
        // 줄 복사 (개행 문자 제거)
        int len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        state->content[line_count] = strdup(line);
        if (state->content[line_count] == NULL) {
            fprintf(stderr, "more: Memory allocation failed\n");
            free_content(state);
            if (file != stdin) fclose(file);
            return -1;
        }
        line_count++;
    }
    
    if (file != stdin) {
        fclose(file);
    }
    
    state->total_lines = line_count;
    state->current_line = 0;
    
    return 0;
}

// 메모리 해제
void free_content(more_state *state) {
    if (state->content != NULL) {
        for (int i = 0; i < state->total_lines; i++) {
            if (state->content[i] != NULL) {
                free(state->content[i]);
            }
        }
        free(state->content);
        state->content = NULL;
    }
    
    if (state->filename != NULL && strcmp(state->filename, "stdin") != 0) {
        free(state->filename);
        state->filename = NULL;
    }
}

// 페이지 표시
void display_page(more_state *state) {
    // 화면 지우기
    printf("\033[2J\033[H");
    
    // 현재 페이지의 줄들 출력
    int lines_displayed = 0;
    int line = state->current_line;
    
    while (line < state->total_lines && lines_displayed < state->lines) {
        // 긴 줄 처리 (터미널 너비에 맞춰 잘라서 표시)
        char *content = state->content[line];
        int len = strlen(content);
        int pos = 0;
        
        while (pos < len && lines_displayed < state->lines) {
            int chars_to_print = (len - pos > state->cols) ? state->cols : (len - pos);
            printf("%.*s", chars_to_print, content + pos);
            
            if (chars_to_print == state->cols && pos + chars_to_print < len) {
                // 줄이 잘렸으면 다음 줄로
                printf("\n");
                lines_displayed++;
            } else {
                printf("\n");
                lines_displayed++;
                break;
            }
            pos += chars_to_print;
        }
        line++;
    }
    
    // 남은 공간 채우기
    while (lines_displayed < state->lines) {
        printf("\n");
        lines_displayed++;
    }
}

// 상태 표시
void display_status(more_state *state) {
    int percent = (state->total_lines == 0) ? 100 : 
                  (state->current_line * 100) / state->total_lines;
    
    printf("\033[7m"); // 역상 표시
    if (state->current_line + state->lines >= state->total_lines) {
        printf("--More-- (END) ");
    } else {
        printf("--More-- (%d%%) ", percent);
    }
    
    if (state->filename && strcmp(state->filename, "stdin") != 0) {
        printf("%s", state->filename);
    }
    
    printf("\033[0m"); // 정상 표시로 복원
    fflush(stdout);
}

// 입력 처리
int handle_input(more_state *state) {
    char ch;
    static char search_pattern[256] = "";
    
    display_status(state);
    ch = get_char();
    
    // 상태 줄 지우기
    printf("\r\033[K");
    
    switch (ch) {
        case ' ':  // 다음 페이지
        case 'f':
            if (state->current_line + state->lines < state->total_lines) {
                state->current_line += state->lines;
            }
            break;
            
        case '\n':  // 다음 줄
        case '\r':
            if (state->current_line < state->total_lines - 1) {
                state->current_line++;
            }
            break;
            
        case 'b':  // 이전 페이지
            state->current_line -= state->lines;
            if (state->current_line < 0) {
                state->current_line = 0;
            }
            break;
            
        case 'g':  // 처음으로
            state->current_line = 0;
            break;
            
        case 'G':  // 끝으로
            state->current_line = state->total_lines - state->lines;
            if (state->current_line < 0) {
                state->current_line = 0;
            }
            break;
            
        case 'h':  // 도움말
            printf("\n");
            printf("Commands:\n");
            printf("  SPACE, f  - Next page\n");
            printf("  ENTER     - Next line\n");
            printf("  b         - Previous page\n");
            printf("  g         - Go to beginning\n");
            printf("  G         - Go to end\n");
            printf("  q         - Quit\n");
            printf("  h         - This help\n");
            printf("\nPress any key to continue...");
            get_char();
            break;
            
        case 'q':  // 종료
        case 'Q':
            return 1;
            
        case '\033':  // ESC 키 (종료)
            return 1;
            
        default:
            break;
    }
    
    return 0;  // 계속
}

int main(int argc, char *argv[]) {
    more_state state = {0};
    int result = 0;
    
    // 도움말 출력
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        print_usage();
        return 0;
    }
    
    // 터미널 크기 얻기
    get_terminal_size(&state.lines, &state.cols);
    
    // 터미널 설정
    setup_terminal();
    
    // 파일이 지정되지 않으면 표준 입력 사용
    if (argc < 2) {
        if (load_file_content("-", &state) == 0) {
            // 메인 루프
            while (1) {
                display_page(&state);
                
                // 파일 끝에 도달했으면 종료
                if (state.current_line + state.lines >= state.total_lines) {
                    display_status(&state);
                    printf(" (END - Press q to quit)");
                    if (get_char() == 'q') break;
                    printf("\r\033[K");
                    continue;
                }
                
                if (handle_input(&state)) {
                    break;  // 종료
                }
            }
        } else {
            result = 1;
        }
        free_content(&state);
    } else {
        // 여러 파일 처리
        for (int i = 1; i < argc; i++) {
            if (load_file_content(argv[i], &state) == 0) {
                // 파일이 여러 개면 파일명 표시
                if (argc > 2) {
                    printf("\n::::::::::::::\n");
                    printf("%s\n", argv[i]);
                    printf("::::::::::::::\n");
                }
                
                // 메인 루프
                while (1) {
                    display_page(&state);
                    
                    // 파일 끝에 도달했으면 다음 파일로
                    if (state.current_line + state.lines >= state.total_lines) {
                        if (i < argc - 1) {
                            display_status(&state);
                            printf(" (Next file: %s - Press SPACE or q to quit)", 
                                   i + 1 < argc ? argv[i + 1] : "");
                            char ch = get_char();
                            printf("\r\033[K");
                            if (ch == 'q') {
                                i = argc;  // 모든 파일 처리 중단
                                break;
                            }
                            break;  // 다음 파일로
                        } else {
                            display_status(&state);
                            printf(" (END - Press q to quit)");
                            get_char();
                            break;
                        }
                    }
                    
                    if (handle_input(&state)) {
                        i = argc;  // 모든 파일 처리 중단
                        break;
                    }
                }
            } else {
                result = 1;
            }
            free_content(&state);
        }
    }
    
    // 터미널 복원
    restore_terminal();
    
    // 화면 지우기
    printf("\033[2J\033[H");
    
    return result;
}

// 컴파일 방법:
// gcc -o more more.c
//
// 사용 예시:
// ./more file.txt                   # 파일을 페이지 단위로 보기
// ./more file1.txt file2.txt        # 여러 파일 보기
// cat large_file.txt | ./more       # 표준 입력에서 읽기
// ./more < input.txt                # 리다이렉션으로 읽기
```

## head: 파일의 처음 N줄 출력 (기본 10줄)
- -n NUM: 처음 NUM줄 출력
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

#define DEFAULT_LINES 10
#define MAX_LINE_LENGTH 4096

typedef struct {
    int num_lines;      // 출력할 줄 수
    int quiet;          // -q 옵션: 파일명 헤더 출력 안함
    int verbose;        // -v 옵션: 항상 파일명 헤더 출력
} head_options;

// 함수 선언
int head_file(const char *filename, head_options *opts);
int head_stdin(head_options *opts);
void print_usage(void);
int parse_number(const char *str);

// 사용법 출력
void print_usage(void) {
    printf("Usage: head [OPTION]... [FILE]...\n");
    printf("Print the first 10 lines of each FILE to standard output.\n");
    printf("With more than one FILE, precede each with a header giving the file name.\n");
    printf("With no FILE, or when FILE is -, read standard input.\n\n");
    printf("  -n, --lines=NUM      print the first NUM lines instead of the first 10;\n");
    printf("                       with the leading '-', print all but the last NUM lines\n");
    printf("  -q, --quiet, --silent never print headers giving file names\n");
    printf("  -v, --verbose        always print headers giving file names\n");
    printf("      --help           display this help and exit\n");
    printf("\nNUM may have a multiplier suffix:\n");
    printf("b 512, kB 1000, K 1024, MB 1000*1000, M 1024*1024,\n");
    printf("GB 1000*1000*1000, G 1024*1024*1024, and so on for T, P, E, Z, Y.\n");
    printf("\nExamples:\n");
    printf("  head -n 5 file.txt    Output first 5 lines of file.txt\n");
    printf("  head -20 file.txt     Output first 20 lines of file.txt\n");
    printf("  head file1 file2      Output first 10 lines of each file\n");
}

// 숫자 파싱 함수 (multiplier suffix 지원)
int parse_number(const char *str) {
    char *endptr;
    long num = strtol(str, &endptr, 10);
    
    if (num < 0) {
        fprintf(stderr, "head: invalid number of lines: '%s'\n", str);
        return -1;
    }
    
    if (num == 0 && endptr == str) {
        fprintf(stderr, "head: invalid number of lines: '%s'\n", str);
        return -1;
    }
    
    // multiplier suffix 처리
    if (*endptr != '\0') {
        switch (*endptr) {
            case 'b':
                num *= 512;
                break;
            case 'k':
                if (*(endptr + 1) == 'B') {
                    num *= 1000;
                } else {
                    num *= 1024;
                }
                break;
            case 'K':
                num *= 1024;
                break;
            case 'M':
                if (*(endptr + 1) == 'B') {
                    num *= 1000000;
                } else {
                    num *= 1024 * 1024;
                }
                break;
            case 'G':
                if (*(endptr + 1) == 'B') {
                    num *= 1000000000;
                } else {
                    num *= 1024 * 1024 * 1024;
                }
                break;
            default:
                fprintf(stderr, "head: invalid suffix in number of lines: '%s'\n", str);
                return -1;
        }
    }
    
    if (num > INT_MAX) {
        fprintf(stderr, "head: number of lines too large: '%s'\n", str);
        return -1;
    }
    
    return (int)num;
}

// 표준 입력에서 읽어서 head 처리
int head_stdin(head_options *opts) {
    char line[MAX_LINE_LENGTH];
    int line_count = 0;
    
    while (line_count < opts->num_lines && fgets(line, sizeof(line), stdin) != NULL) {
        printf("%s", line);
        line_count++;
    }
    
    return 0;
}

// 파일에서 head 처리
int head_file(const char *filename, head_options *opts) {
    FILE *file;
    char line[MAX_LINE_LENGTH];
    int line_count = 0;
    
    // 파일명이 "-"이면 표준 입력 사용
    if (strcmp(filename, "-") == 0) {
        return head_stdin(opts);
    }
    
    // 파일 열기
    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "head: cannot open '%s' for reading: %s\n", 
                filename, strerror(errno));
        return -1;
    }
    
    // 파일이 디렉토리인지 확인
    struct stat st;
    if (fstat(fileno(file), &st) == 0 && S_ISDIR(st.st_mode)) {
        fprintf(stderr, "head: error reading '%s': Is a directory\n", filename);
        fclose(file);
        return -1;
    }
    
    // 파일 내용 읽기 및 출력
    while (line_count < opts->num_lines && fgets(line, sizeof(line), file) != NULL) {
        printf("%s", line);
        line_count++;
    }
    
    fclose(file);
    return 0;
}

// 옵션 파싱 함수
int parse_options(int argc, char *argv[], head_options *opts, int *file_start) {
    int i;
    
    // 옵션 초기화
    opts->num_lines = DEFAULT_LINES;
    opts->quiet = 0;
    opts->verbose = 0;
    
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            break; // 옵션이 아닌 첫 번째 인수
        }
        
        // "--" 처리
        if (strcmp(argv[i], "--") == 0) {
            i++;
            break; // 옵션 끝
        }
        
        // 긴 옵션들
        if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 1;
        }
        
        if (strncmp(argv[i], "--lines=", 8) == 0) {
            int num = parse_number(argv[i] + 8);
            if (num == -1) return -1;
            opts->num_lines = num;
            continue;
        }
        
        if (strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "--silent") == 0) {
            opts->quiet = 1;
            continue;
        }
        
        if (strcmp(argv[i], "--verbose") == 0) {
            opts->verbose = 1;
            continue;
        }
        
        // "-" 단독으로 사용되면 표준 입력을 의미
        if (strcmp(argv[i], "-") == 0) {
            break;
        }
        
        // 숫자로 시작하는 옵션 (예: -20, -n20)
        if (argv[i][1] != '\0' && (isdigit(argv[i][1]) || 
            (argv[i][1] == 'n' && argc > i + 1))) {
            
            if (argv[i][1] == 'n') {
                // -n 옵션
                if (argv[i][2] != '\0') {
                    // -n20 형태
                    int num = parse_number(argv[i] + 2);
                    if (num == -1) return -1;
                    opts->num_lines = num;
                } else {
                    // -n 20 형태
                    if (i + 1 >= argc) {
                        fprintf(stderr, "head: option requires an argument -- 'n'\n");
                        return -1;
                    }
                    int num = parse_number(argv[i + 1]);
                    if (num == -1) return -1;
                    opts->num_lines = num;
                    i++; // 다음 인수 건너뛰기
                }
            } else {
                // -20 형태
                int num = parse_number(argv[i] + 1);
                if (num == -1) return -1;
                opts->num_lines = num;
            }
            continue;
        }
        
        // 짧은 옵션들 처리
        int j;
        for (j = 1; argv[i][j] != '\0'; j++) {
            switch (argv[i][j]) {
                case 'n':
                    if (argv[i][j + 1] != '\0') {
                        // -n20 형태
                        int num = parse_number(argv[i] + j + 1);
                        if (num == -1) return -1;
                        opts->num_lines = num;
                        j = strlen(argv[i]) - 1; // 루프 종료
                    } else {
                        // -n 20 형태
                        if (i + 1 >= argc) {
                            fprintf(stderr, "head: option requires an argument -- 'n'\n");
                            return -1;
                        }
                        int num = parse_number(argv[i + 1]);
                        if (num == -1) return -1;
                        opts->num_lines = num;
                        i++; // 다음 인수 건너뛰기
                        j = strlen(argv[i]) - 1; // 루프 종료
                    }
                    break;
                case 'q':
                    opts->quiet = 1;
                    break;
                case 'v':
                    opts->verbose = 1;
                    break;
                default:
                    fprintf(stderr, "head: invalid option -- '%c'\n", argv[i][j]);
                    fprintf(stderr, "Try 'head --help' for more information.\n");
                    return -1;
            }
        }
    }
    
    *file_start = i;
    return 0;
}

int main(int argc, char *argv[]) {
    head_options opts;
    int file_start;
    int result = 0;
    int i;
    int num_files;
    
    // 옵션 파싱
    int parse_result = parse_options(argc, argv, &opts, &file_start);
    if (parse_result == 1) {
        return 0; // help 출력 후 정상 종료
    }
    if (parse_result == -1) {
        return 1; // 옵션 파싱 오류
    }
    
    // 파일 개수 계산
    num_files = argc - file_start;
    
    // 파일 인수가 없으면 표준 입력 사용
    if (num_files == 0) {
        if (head_stdin(&opts) == -1) {
            result = 1;
        }
    } else {
        // 각 파일 처리
        for (i = file_start; i < argc; i++) {
            int is_first_file = (i == file_start);
            int is_last_file = (i == argc - 1);
            
            // 파일명 헤더 출력 여부 결정
            int print_header = 0;
            if (opts.verbose) {
                print_header = 1;
            } else if (!opts.quiet && num_files > 1) {
                print_header = 1;
            }
            
            // 파일명 헤더 출력
            if (print_header) {
                if (!is_first_file) {
                    printf("\n");
                }
                printf("==> %s <==\n", argv[i]);
            }
            
            // 파일 처리
            if (head_file(argv[i], &opts) == -1) {
                result = 1;
            }
        }
    }
    
    return result;
}

// 컴파일 방법:
// gcc -o head head.c
//
// 사용 예시:
// ./head file.txt                   # 첫 10줄 출력
// ./head -n 5 file.txt              # 첫 5줄 출력
// ./head -20 file.txt               # 첫 20줄 출력
// ./head -n5 file.txt               # 첫 5줄 출력 (공백 없음)
// ./head file1.txt file2.txt        # 여러 파일의 첫 10줄씩 출력
// ./head -n 3 *.txt                 # 모든 .txt 파일의 첫 3줄씩 출력
// cat file.txt | ./head -n 15       # 파이프를 통한 입력
// ./head < input.txt                # 리다이렉션으로 입력
// ./head -q file1.txt file2.txt     # 파일명 헤더 없이 출력
// ./head -v file.txt                # 파일 하나여도 헤더 출력
```

## tail: 파일의 마지막 N줄 출력 (기본 10줄)
- -n NUM: 마지막 NUM줄 출력
- -f: 파일 내용이 추가될 때 실시간으로 출력 (로그 파일 모니터링)
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>

#define MAX_LINE_LENGTH 4096
#define DEFAULT_LINES 10

typedef struct {
    char **lines;
    int count;
    int capacity;
} LineBuffer;

// 라인 버퍼 초기화
LineBuffer* create_line_buffer(int capacity) {
    LineBuffer *buf = malloc(sizeof(LineBuffer));
    buf->lines = malloc(sizeof(char*) * capacity);
    buf->count = 0;
    buf->capacity = capacity;
    return buf;
}

// 라인 버퍼 해제
void free_line_buffer(LineBuffer *buf) {
    for (int i = 0; i < buf->count; i++) {
        free(buf->lines[i]);
    }
    free(buf->lines);
    free(buf);
}

// 라인 추가 (링 버퍼 방식)
void add_line(LineBuffer *buf, const char *line) {
    if (buf->count < buf->capacity) {
        buf->lines[buf->count] = strdup(line);
        buf->count++;
    } else {
        // 가장 오래된 라인 제거하고 새 라인 추가
        free(buf->lines[0]);
        for (int i = 0; i < buf->capacity - 1; i++) {
            buf->lines[i] = buf->lines[i + 1];
        }
        buf->lines[buf->capacity - 1] = strdup(line);
    }
}

// 파일의 마지막 n줄 읽기
int read_last_lines(const char *filename, int n) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "tail: cannot open '%s' for reading: %s\n", 
                filename, strerror(errno));
        return -1;
    }

    LineBuffer *buf = create_line_buffer(n);
    char line[MAX_LINE_LENGTH];

    // 파일의 모든 라인을 읽으면서 마지막 n줄만 버퍼에 유지
    while (fgets(line, sizeof(line), file)) {
        add_line(buf, line);
    }

    // 버퍼의 내용 출력
    for (int i = 0; i < buf->count; i++) {
        printf("%s", buf->lines[i]);
    }

    free_line_buffer(buf);
    fclose(file);
    return 0;
}

// 파일 크기 가져오기
long get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

// 파일 실시간 모니터링
int follow_file(const char *filename, int n) {
    // 먼저 마지막 n줄 출력
    if (read_last_lines(filename, n) < 0) {
        return -1;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "tail: cannot open '%s' for reading: %s\n", 
                filename, strerror(errno));
        return -1;
    }

    // 파일 끝으로 이동
    fseek(file, 0, SEEK_END);
    long last_size = ftell(file);

    char line[MAX_LINE_LENGTH];
    
    while (1) {
        // 파일 크기 확인
        long current_size = get_file_size(filename);
        
        if (current_size > last_size) {
            // 새로운 내용이 추가됨
            while (fgets(line, sizeof(line), file)) {
                printf("%s", line);
                fflush(stdout);
            }
            last_size = current_size;
        } else if (current_size < last_size) {
            // 파일이 잘렸거나 새로 생성됨
            fclose(file);
            file = fopen(filename, "r");
            if (!file) {
                fprintf(stderr, "tail: '%s' has been replaced\n", filename);
                return -1;
            }
            fseek(file, 0, SEEK_END);
            last_size = ftell(file);
        }
        
        // 100ms 대기
        usleep(100000);
    }

    fclose(file);
    return 0;
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTION]... [FILE]...\n", prog_name);
    printf("Print the last 10 lines of each FILE to standard output.\n");
    printf("With more than one FILE, precede each with a header giving the file name.\n\n");
    printf("Options:\n");
    printf("  -n, --lines=NUM     output the last NUM lines, instead of the last 10\n");
    printf("  -f, --follow        output appended data as the file grows\n");
    printf("  -h, --help          display this help and exit\n");
}

int main(int argc, char *argv[]) {
    int n = DEFAULT_LINES;
    int follow = 0;
    int opt;
    
    static struct option long_options[] = {
        {"lines", required_argument, 0, 'n'},
        {"follow", no_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    // 옵션 파싱
    while ((opt = getopt_long(argc, argv, "n:fh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'n':
                n = atoi(optarg);
                if (n <= 0) {
                    fprintf(stderr, "tail: invalid number of lines: '%s'\n", optarg);
                    return 1;
                }
                break;
            case 'f':
                follow = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    // 파일명이 없으면 표준입력 사용
    if (optind >= argc) {
        fprintf(stderr, "tail: missing file operand\n");
        print_usage(argv[0]);
        return 1;
    }

    // 여러 파일 처리
    int num_files = argc - optind;
    int result = 0;

    for (int i = optind; i < argc; i++) {
        const char *filename = argv[i];
        
        // 여러 파일일 때 헤더 출력
        if (num_files > 1) {
            if (i > optind) printf("\n");
            printf("==> %s <==\n", filename);
        }

        if (follow && i == argc - 1) {
            // -f 옵션은 마지막 파일에만 적용
            result = follow_file(filename, n);
        } else {
            result = read_last_lines(filename, n);
        }

        if (result < 0) {
            result = 1; // 에러 발생
        }
    }

    return result;
}
```

## find: 파일 또는 디렉토리 검색
- -name PATTERN: 이름으로 검색
- -type TYPE: 파일 종류 (d: 디렉토리, f: 파일)
- -size N[cwbkMG]: 크기로 검색

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>

#define MAX_PATH_LENGTH 4096

// 검색 조건을 저장하는 구조체
typedef struct {
    char *name_pattern;
    char file_type;      // 'f': 파일, 'd': 디렉토리, 0: 모든 타입
    long size_bytes;     // 파일 크기 (바이트)
    char size_operator;  // '+': 이상, '-': 이하, '=': 정확히
} SearchCriteria;

// 크기 단위를 바이트로 변환
long parse_size(const char *size_str, char *operator) {
    if (!size_str || strlen(size_str) == 0) {
        return -1;
    }
    
    char *str = strdup(size_str);
    long size = 0;
    char unit = 'c';  // 기본 단위: 바이트
    
    // 연산자 확인
    *operator = '=';  // 기본값: 정확히
    if (str[0] == '+') {
        *operator = '+';
        memmove(str, str + 1, strlen(str));
    } else if (str[0] == '-') {
        *operator = '-';
        memmove(str, str + 1, strlen(str));
    }
    
    int len = strlen(str);
    if (len > 0) {
        // 마지막 문자가 단위인지 확인
        char last_char = str[len - 1];
        if (last_char == 'c' || last_char == 'w' || last_char == 'b' ||
            last_char == 'k' || last_char == 'M' || last_char == 'G') {
            unit = last_char;
            str[len - 1] = '\0';
        }
    }
    
    size = atol(str);
    free(str);
    
    // 단위에 따른 바이트 변환
    switch (unit) {
        case 'c': return size;              // 바이트
        case 'w': return size * 2;          // 워드 (2바이트)
        case 'b': return size * 512;        // 블록 (512바이트)
        case 'k': return size * 1024;       // 킬로바이트
        case 'M': return size * 1024 * 1024; // 메가바이트
        case 'G': return size * 1024 * 1024 * 1024; // 기가바이트
        default: return size;
    }
}

// 크기 조건 확인
int check_size_condition(long file_size, long target_size, char operator) {
    switch (operator) {
        case '+': return file_size > target_size;
        case '-': return file_size < target_size;
        case '=': return file_size == target_size;
        default: return 1;
    }
}

// 파일/디렉토리가 검색 조건에 맞는지 확인
int matches_criteria(const char *path, const struct stat *st, const SearchCriteria *criteria) {
    // 파일 타입 확인
    if (criteria->file_type != 0) {
        if (criteria->file_type == 'f' && !S_ISREG(st->st_mode)) {
            return 0;
        }
        if (criteria->file_type == 'd' && !S_ISDIR(st->st_mode)) {
            return 0;
        }
    }
    
    // 이름 패턴 확인
    if (criteria->name_pattern) {
        const char *filename = strrchr(path, '/');
        filename = filename ? filename + 1 : path;
        
        if (fnmatch(criteria->name_pattern, filename, 0) != 0) {
            return 0;
        }
    }
    
    // 크기 확인
    if (criteria->size_bytes >= 0) {
        if (!check_size_condition(st->st_size, criteria->size_bytes, criteria->size_operator)) {
            return 0;
        }
    }
    
    return 1;
}

// 재귀적으로 디렉토리 탐색
void find_recursive(const char *dir_path, const SearchCriteria *criteria) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "find: '%s': %s\n", dir_path, strerror(errno));
        return;
    }
    
    struct dirent *entry;
    struct stat st;
    char full_path[MAX_PATH_LENGTH];
    
    while ((entry = readdir(dir)) != NULL) {
        // . 과 .. 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 전체 경로 생성
        int path_len = snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        if (path_len >= sizeof(full_path)) {
            fprintf(stderr, "find: path too long: %s/%s\n", dir_path, entry->d_name);
            continue;
        }
        
        // 파일 정보 가져오기
        if (lstat(full_path, &st) != 0) {
            fprintf(stderr, "find: '%s': %s\n", full_path, strerror(errno));
            continue;
        }
        
        // 조건 확인 후 출력
        if (matches_criteria(full_path, &st, criteria)) {
            printf("%s\n", full_path);
        }
        
        // 디렉토리면 재귀 탐색 (심볼릭 링크 제외)
        if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
            find_recursive(full_path, criteria);
        }
    }
    
    closedir(dir);
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [path...] [expression]\n", prog_name);
    printf("Search for files and directories.\n\n");
    printf("Options:\n");
    printf("  -name PATTERN    find files/directories matching PATTERN\n");
    printf("  -type TYPE       find files of type TYPE:\n");
    printf("                   f: regular file, d: directory\n");
    printf("  -size N[cwbkMG]  find files of size N:\n");
    printf("                   c: bytes, w: words (2 bytes), b: blocks (512 bytes)\n");
    printf("                   k: kilobytes, M: megabytes, G: gigabytes\n");
    printf("                   +N: greater than N, -N: less than N\n");
    printf("  -h, --help       display this help and exit\n");
    printf("\nExamples:\n");
    printf("  %s /home -name '*.txt'        # find all .txt files in /home\n", prog_name);
    printf("  %s . -type d                  # find all directories\n", prog_name);
    printf("  %s /var -size +1M             # find files larger than 1MB\n", prog_name);
    printf("  %s . -name '*.log' -size -10k # find .log files smaller than 10KB\n", prog_name);
}

int main(int argc, char *argv[]) {
    SearchCriteria criteria = {0};
    criteria.file_type = 0;
    criteria.size_bytes = -1;
    criteria.size_operator = '=';
    
    char *search_path = ".";  // 기본 검색 경로
    int path_specified = 0;
    
    static struct option long_options[] = {
        {"name", required_argument, 0, 'n'},
        {"type", required_argument, 0, 't'},
        {"size", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "n:t:s:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'n':
                criteria.name_pattern = strdup(optarg);
                break;
            case 't':
                if (strcmp(optarg, "f") == 0) {
                    criteria.file_type = 'f';
                } else if (strcmp(optarg, "d") == 0) {
                    criteria.file_type = 'd';
                } else {
                    fprintf(stderr, "find: invalid file type '%s'\n", optarg);
                    return 1;
                }
                break;
            case 's':
                criteria.size_bytes = parse_size(optarg, &criteria.size_operator);
                if (criteria.size_bytes < 0) {
                    fprintf(stderr, "find: invalid size '%s'\n", optarg);
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // 경로가 지정되었는지 확인
    if (optind < argc) {
        // 첫 번째 비옵션 인수가 경로인지 확인
        struct stat st;
        if (stat(argv[optind], &st) == 0) {
            search_path = argv[optind];
            path_specified = 1;
            optind++;
        }
    }
    
    // 나머지 인수들을 옵션으로 처리 (GNU find 스타일)
    while (optind < argc) {
        char *arg = argv[optind];
        
        if (strcmp(arg, "-name") == 0) {
            if (optind + 1 >= argc) {
                fprintf(stderr, "find: missing argument for -name\n");
                return 1;
            }
            criteria.name_pattern = strdup(argv[++optind]);
        } else if (strcmp(arg, "-type") == 0) {
            if (optind + 1 >= argc) {
                fprintf(stderr, "find: missing argument for -type\n");
                return 1;
            }
            char *type_arg = argv[++optind];
            if (strcmp(type_arg, "f") == 0) {
                criteria.file_type = 'f';
            } else if (strcmp(type_arg, "d") == 0) {
                criteria.file_type = 'd';
            } else {
                fprintf(stderr, "find: invalid file type '%s'\n", type_arg);
                return 1;
            }
        } else if (strcmp(arg, "-size") == 0) {
            if (optind + 1 >= argc) {
                fprintf(stderr, "find: missing argument for -size\n");
                return 1;
            }
            criteria.size_bytes = parse_size(argv[++optind], &criteria.size_operator);
            if (criteria.size_bytes < 0) {
                fprintf(stderr, "find: invalid size '%s'\n", argv[optind]);
                return 1;
            }
        } else {
            fprintf(stderr, "find: unknown option '%s'\n", arg);
            return 1;
        }
        optind++;
    }
    
    // 시작 디렉토리 자체도 조건 확인
    struct stat st;
    if (stat(search_path, &st) == 0) {
        if (matches_criteria(search_path, &st, &criteria)) {
            printf("%s\n", search_path);
        }
        
        // 디렉토리면 재귀 탐색
        if (S_ISDIR(st.st_mode)) {
            find_recursive(search_path, &criteria);
        }
    } else {
        fprintf(stderr, "find: '%s': %s\n", search_path, strerror(errno));
        return 1;
    }
    
    // 메모리 해제
    if (criteria.name_pattern) {
        free(criteria.name_pattern);
    }
    
    return 0;
}
```

## grep: 파일 내용에서 패턴 검색
- -i: 대소문자 무시
- -r: 하위 디렉토리 재귀적 검색
- -l: 패턴이 포함된 파일 이름만 출력
- -v: 패턴이 포함되지 않은 줄 출력

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <regex.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 4096
#define MAX_PATH_LENGTH 4096

// grep 옵션을 저장하는 구조체
typedef struct {
    int ignore_case;     // -i: 대소문자 무시
    int recursive;       // -r: 재귀 검색
    int files_only;      // -l: 파일명만 출력
    int invert_match;    // -v: 패턴 불일치 줄 출력
    int line_number;     // -n: 줄 번호 출력
    int count_only;      // -c: 매칭된 줄 수만 출력
    char *pattern;       // 검색 패턴
} GrepOptions;

// 문자열을 소문자로 변환
void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

// 단순 문자열 검색 (정규식 없이)
int simple_match(const char *line, const char *pattern, int ignore_case) {
    if (ignore_case) {
        char *line_copy = strdup(line);
        char *pattern_copy = strdup(pattern);
        to_lowercase(line_copy);
        to_lowercase(pattern_copy);
        
        int result = (strstr(line_copy, pattern_copy) != NULL);
        free(line_copy);
        free(pattern_copy);
        return result;
    } else {
        return (strstr(line, pattern) != NULL);
    }
}

// 정규식 검색
int regex_match(const char *line, regex_t *regex) {
    return regexec(regex, line, 0, NULL, 0) == 0;
}

// 파일에서 패턴 검색
int grep_file(const char *filename, const GrepOptions *opts, regex_t *regex, int use_regex) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "grep: %s: %s\n", filename, strerror(errno));
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    int line_num = 0;
    int match_count = 0;
    int found_match = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        
        // 줄 끝의 개행 문자 제거 (출력 형식 조정용)
        int len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // 패턴 매칭 확인
        int matches;
        if (use_regex) {
            matches = regex_match(line, regex);
        } else {
            matches = simple_match(line, opts->pattern, opts->ignore_case);
        }
        
        // -v 옵션이면 매칭 결과 반전
        if (opts->invert_match) {
            matches = !matches;
        }
        
        if (matches) {
            match_count++;
            found_match = 1;
            
            // -l 옵션: 파일명만 출력하고 종료
            if (opts->files_only) {
                printf("%s\n", filename);
                fclose(file);
                return 1;
            }
            
            // -c 옵션이 아니면 실제 줄 출력
            if (!opts->count_only) {
                // 파일명 출력 (표준입력이 아닌 경우)
                if (strcmp(filename, "-") != 0) {
                    printf("%s:", filename);
                }
                
                // 줄 번호 출력
                if (opts->line_number) {
                    printf("%d:", line_num);
                }
                
                printf("%s\n", line);
            }
        }
    }
    
    // -c 옵션: 매칭된 줄 수 출력
    if (opts->count_only) {
        if (strcmp(filename, "-") != 0) {
            printf("%s:", filename);
        }
        printf("%d\n", match_count);
    }
    
    fclose(file);
    return found_match ? 1 : 0;
}

// 디렉토리 재귀 검색
int grep_directory(const char *dir_path, const GrepOptions *opts, regex_t *regex, int use_regex) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "grep: %s: %s\n", dir_path, strerror(errno));
        return -1;
    }
    
    struct dirent *entry;
    struct stat st;
    char full_path[MAX_PATH_LENGTH];
    int found_any = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // . 과 .. 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 숨김 파일 건너뛰기 (선택사항)
        if (entry->d_name[0] == '.') {
            continue;
        }
        
        // 전체 경로 생성
        int path_len = snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        if (path_len >= sizeof(full_path)) {
            fprintf(stderr, "grep: path too long: %s/%s\n", dir_path, entry->d_name);
            continue;
        }
        
        // 파일 정보 가져오기
        if (lstat(full_path, &st) != 0) {
            fprintf(stderr, "grep: %s: %s\n", full_path, strerror(errno));
            continue;
        }
        
        // 일반 파일이면 검색
        if (S_ISREG(st.st_mode)) {
            int result = grep_file(full_path, opts, regex, use_regex);
            if (result > 0) {
                found_any = 1;
            }
        }
        // 디렉토리이고 재귀 옵션이 활성화되어 있으면 재귀 검색
        else if (S_ISDIR(st.st_mode) && opts->recursive) {
            int result = grep_directory(full_path, opts, regex, use_regex);
            if (result > 0) {
                found_any = 1;
            }
        }
    }
    
    closedir(dir);
    return found_any ? 1 : 0;
}

// 표준입력에서 검색
int grep_stdin(const GrepOptions *opts, regex_t *regex, int use_regex) {
    return grep_file("-", opts, regex, use_regex);
}

// 파일인지 디렉토리인지 확인하고 적절한 함수 호출
int grep_path(const char *path, const GrepOptions *opts, regex_t *regex, int use_regex) {
    struct stat st;
    
    if (stat(path, &st) != 0) {
        fprintf(stderr, "grep: %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    if (S_ISREG(st.st_mode)) {
        return grep_file(path, opts, regex, use_regex);
    } else if (S_ISDIR(st.st_mode)) {
        if (opts->recursive) {
            return grep_directory(path, opts, regex, use_regex);
        } else {
            fprintf(stderr, "grep: %s: Is a directory\n", path);
            return -1;
        }
    } else {
        fprintf(stderr, "grep: %s: Not a regular file or directory\n", path);
        return -1;
    }
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTION]... PATTERN [FILE]...\n", prog_name);
    printf("Search for PATTERN in each FILE.\n");
    printf("Example: %s -i 'hello world' menu.h main.c\n\n", prog_name);
    printf("Pattern selection and interpretation:\n");
    printf("  -E, --extended-regexp     PATTERN is an extended regular expression\n");
    printf("  -i, --ignore-case         ignore case distinctions\n");
    printf("  -v, --invert-match        select non-matching lines\n\n");
    printf("Output control:\n");
    printf("  -c, --count               print only a count of matching lines per FILE\n");
    printf("  -l, --files-with-matches  print only names of FILEs containing matches\n");
    printf("  -n, --line-number         print line number with output lines\n\n");
    printf("File and directory selection:\n");
    printf("  -r, --recursive           search directories recursively\n\n");
    printf("  -h, --help                display this help and exit\n");
    printf("\nWith no FILE, or when FILE is -, read standard input.\n");
}

int main(int argc, char *argv[]) {
    GrepOptions opts = {0};
    int use_regex = 0;
    regex_t regex;
    int regex_flags = REG_NOSUB;
    
    static struct option long_options[] = {
        {"extended-regexp", no_argument, 0, 'E'},
        {"ignore-case", no_argument, 0, 'i'},
        {"recursive", no_argument, 0, 'r'},
        {"files-with-matches", no_argument, 0, 'l'},
        {"invert-match", no_argument, 0, 'v'},
        {"line-number", no_argument, 0, 'n'},
        {"count", no_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "Eirlvnch", long_options, NULL)) != -1) {
        switch (opt) {
            case 'E':
                use_regex = 1;
                break;
            case 'i':
                opts.ignore_case = 1;
                regex_flags |= REG_ICASE;
                break;
            case 'r':
                opts.recursive = 1;
                break;
            case 'l':
                opts.files_only = 1;
                break;
            case 'v':
                opts.invert_match = 1;
                break;
            case 'n':
                opts.line_number = 1;
                break;
            case 'c':
                opts.count_only = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // 패턴이 없으면 에러
    if (optind >= argc) {
        fprintf(stderr, "grep: missing pattern\n");
        print_usage(argv[0]);
        return 1;
    }
    
    opts.pattern = argv[optind++];
    
    // 정규식 컴파일 (필요시)
    if (use_regex) {
        int reg_result = regcomp(&regex, opts.pattern, regex_flags);
        if (reg_result != 0) {
            char error_buf[256];
            regerror(reg_result, &regex, error_buf, sizeof(error_buf));
            fprintf(stderr, "grep: invalid regex '%s': %s\n", opts.pattern, error_buf);
            return 1;
        }
    }
    
    int exit_status = 1;  // 매치되는 것이 없으면 1
    
    // 파일이 지정되지 않았으면 표준입력 사용
    if (optind >= argc) {
        int result = grep_stdin(&opts, &regex, use_regex);
        if (result > 0) {
            exit_status = 0;
        }
    } else {
        // 지정된 파일들 처리
        for (int i = optind; i < argc; i++) {
            int result = grep_path(argv[i], &opts, &regex, use_regex);
            if (result > 0) {
                exit_status = 0;
            }
        }
    }
    
    // 정규식 해제
    if (use_regex) {
        regfree(&regex);
    }
    
    return exit_status;
}
```

## chmod: 파일/디렉토리 권한 변경 (예: chmod 755 file.sh)
- 표기법 (숫자, 심볼릭) 지원

```
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
```

## chown: 파일/디렉토리 소유자 변경 (루트 권한 필요) 
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

// 사용법 출력
void print_usage(const char* program_name) {
    printf("Usage: %s [OPTION]... [OWNER][:[GROUP]] FILE...\n", program_name);
    printf("       %s [OPTION]... --reference=RFILE FILE...\n", program_name);
    printf("\n");
    printf("Change the owner and/or group of each FILE to OWNER and/or GROUP.\n");
    printf("\n");
    printf("Options:\n");
    printf("  -R, --recursive    operate on files and directories recursively\n");
    printf("  -v, --verbose      output a diagnostic for every file processed\n");
    printf("  -h, --help         display this help and exit\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s root file.txt           # Change owner to root\n", program_name);
    printf("  %s root:admin file.txt     # Change owner to root, group to admin\n", program_name);
    printf("  %s :admin file.txt         # Change group to admin only\n", program_name);
    printf("  %s -R user:group dir/      # Recursively change ownership\n", program_name);
}

// 사용자 이름을 UID로 변환
uid_t get_uid_from_name(const char* name) {
    if (!name || *name == '\0') return -1;
    
    // 숫자인지 확인
    char* endptr;
    long uid = strtol(name, &endptr, 10);
    if (*endptr == '\0') {
        return (uid_t)uid;
    }
    
    // 사용자 이름으로 검색
    struct passwd* pwd = getpwnam(name);
    if (pwd == NULL) {
        return -1;
    }
    return pwd->pw_uid;
}

// 그룹 이름을 GID로 변환
gid_t get_gid_from_name(const char* name) {
    if (!name || *name == '\0') return -1;
    
    // 숫자인지 확인
    char* endptr;
    long gid = strtol(name, &endptr, 10);
    if (*endptr == '\0') {
        return (gid_t)gid;
    }
    
    // 그룹 이름으로 검색
    struct group* grp = getgrnam(name);
    if (grp == NULL) {
        return -1;
    }
    return grp->gr_gid;
}

// 소유자:그룹 문자열 파싱
int parse_owner_group(const char* spec, uid_t* uid, gid_t* gid, int* change_uid, int* change_gid) {
    *change_uid = 0;
    *change_gid = 0;
    
    char* spec_copy = strdup(spec);
    if (!spec_copy) {
        perror("strdup");
        return -1;
    }
    
    char* colon = strchr(spec_copy, ':');
    
    if (colon) {
        *colon = '\0';
        
        // 소유자 부분
        if (strlen(spec_copy) > 0) {
            *uid = get_uid_from_name(spec_copy);
            if (*uid == (uid_t)-1) {
                fprintf(stderr, "chown: invalid user: '%s'\n", spec_copy);
                free(spec_copy);
                return -1;
            }
            *change_uid = 1;
        }
        
        // 그룹 부분
        if (strlen(colon + 1) > 0) {
            *gid = get_gid_from_name(colon + 1);
            if (*gid == (gid_t)-1) {
                fprintf(stderr, "chown: invalid group: '%s'\n", colon + 1);
                free(spec_copy);
                return -1;
            }
            *change_gid = 1;
        }
    } else {
        // 콜론이 없으면 소유자만 변경
        *uid = get_uid_from_name(spec_copy);
        if (*uid == (uid_t)-1) {
            fprintf(stderr, "chown: invalid user: '%s'\n", spec_copy);
            free(spec_copy);
            return -1;
        }
        *change_uid = 1;
    }
    
    free(spec_copy);
    return 0;
}

// 단일 파일/디렉토리 소유권 변경
int chown_single(const char* path, uid_t uid, gid_t gid, int change_uid, int change_gid, int verbose) {
    struct stat st;
    if (lstat(path, &st) == -1) {
        fprintf(stderr, "chown: cannot access '%s': %s\n", path, strerror(errno));
        return -1;
    }
    
    uid_t new_uid = change_uid ? uid : st.st_uid;
    gid_t new_gid = change_gid ? gid : st.st_gid;
    
    if (chown(path, new_uid, new_gid) == -1) {
        fprintf(stderr, "chown: changing ownership of '%s': %s\n", path, strerror(errno));
        return -1;
    }
    
    if (verbose) {
        struct passwd* pwd = getpwuid(new_uid);
        struct group* grp = getgrgid(new_gid);
        printf("ownership of '%s' changed to %s:%s\n", 
               path,
               pwd ? pwd->pw_name : "unknown",
               grp ? grp->gr_name : "unknown");
    }
    
    return 0;
}

// 재귀적으로 디렉토리 처리
int chown_recursive(const char* path, uid_t uid, gid_t gid, int change_uid, int change_gid, int verbose) {
    // 현재 디렉토리/파일 소유권 변경
    if (chown_single(path, uid, gid, change_uid, change_gid, verbose) == -1) {
        return -1;
    }
    
    struct stat st;
    if (lstat(path, &st) == -1) {
        return -1;
    }
    
    // 디렉토리가 아니면 종료
    if (!S_ISDIR(st.st_mode)) {
        return 0;
    }
    
    DIR* dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "chown: cannot open directory '%s': %s\n", path, strerror(errno));
        return -1;
    }
    
    struct dirent* entry;
    int result = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // . 과 .. 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (chown_recursive(full_path, uid, gid, change_uid, change_gid, verbose) == -1) {
            result = -1;
        }
    }
    
    closedir(dir);
    return result;
}

int main(int argc, char* argv[]) {
    int recursive = 0;
    int verbose = 0;
    int opt_index = 1;
    
    // 옵션 파싱
    while (opt_index < argc && argv[opt_index][0] == '-') {
        if (strcmp(argv[opt_index], "-R") == 0 || strcmp(argv[opt_index], "--recursive") == 0) {
            recursive = 1;
        } else if (strcmp(argv[opt_index], "-v") == 0 || strcmp(argv[opt_index], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[opt_index], "-h") == 0 || strcmp(argv[opt_index], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "chown: invalid option -- '%s'\n", argv[opt_index]);
            print_usage(argv[0]);
            return 1;
        }
        opt_index++;
    }
    
    // 최소 2개 인자 필요 (소유자:그룹, 파일)
    if (argc - opt_index < 2) {
        fprintf(stderr, "chown: missing operand\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // 소유자:그룹 파싱
    uid_t uid;
    gid_t gid;
    int change_uid, change_gid;
    
    if (parse_owner_group(argv[opt_index], &uid, &gid, &change_uid, &change_gid) == -1) {
        return 1;
    }
    
    if (!change_uid && !change_gid) {
        fprintf(stderr, "chown: no owner or group specified\n");
        return 1;
    }
    
    opt_index++; // 소유자:그룹 인자 넘어가기
    
    // 권한 확인 (실제 시스템에서는 root 권한이 필요할 수 있음)
    if (getuid() != 0 && (change_uid || change_gid)) {
        fprintf(stderr, "chown: changing ownership requires superuser privileges\n");
        // 경고만 출력하고 계속 진행 (테스트 목적)
    }
    
    int result = 0;
    
    // 각 파일에 대해 작업 수행
    for (int i = opt_index; i < argc; i++) {
        if (recursive) {
            if (chown_recursive(argv[i], uid, gid, change_uid, change_gid, verbose) == -1) {
                result = 1;
            }
        } else {
            if (chown_single(argv[i], uid, gid, change_uid, change_gid, verbose) == -1) {
                result = 1;
            }
        }
    }
    
    return result;
}
```

## diff: 두 파일의 차이점 비교 
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_LINE_LENGTH 1024
#define MAX_LINES 10000

typedef struct {
    char** lines;
    int count;
    int capacity;
} FileContent;

typedef struct {
    int type;  // 0: 같음, 1: 추가, 2: 삭제, 3: 변경
    int line1, line2;  // 원본 파일에서의 라인 번호
    int count1, count2;  // 변경된 라인 수
} DiffResult;

// 사용법 출력
void print_usage(const char* program_name) {
    printf("Usage: %s [OPTION]... FILE1 FILE2\n", program_name);
    printf("Compare FILE1 and FILE2 line by line.\n");
    printf("\n");
    printf("Options:\n");
    printf("  -u, --unified[=NUM]   output NUM (default 3) lines of unified context\n");
    printf("  -c, --context[=NUM]   output NUM (default 3) lines of copied context\n");
    printf("  -i, --ignore-case     ignore case differences in file contents\n");
    printf("  -w, --ignore-all-space ignore all white space\n");
    printf("  -b, --ignore-space-change ignore changes in the amount of white space\n");
    printf("  -q, --brief           report only when files differ\n");
    printf("  -s, --report-identical-files report when two files are the same\n");
    printf("  -h, --help            display this help and exit\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s file1.txt file2.txt\n", program_name);
    printf("  %s -u file1.txt file2.txt    # unified format\n", program_name);
    printf("  %s -i file1.txt file2.txt    # ignore case\n", program_name);
}

// 파일 내용을 메모리에 로드
int load_file(const char* filename, FileContent* content) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "diff: %s: %s\n", filename, strerror(errno));
        return -1;
    }
    
    content->lines = malloc(MAX_LINES * sizeof(char*));
    content->count = 0;
    content->capacity = MAX_LINES;
    
    char buffer[MAX_LINE_LENGTH];
    while (fgets(buffer, sizeof(buffer), file)) {
        if (content->count >= content->capacity) {
            content->capacity *= 2;
            content->lines = realloc(content->lines, content->capacity * sizeof(char*));
            if (!content->lines) {
                perror("realloc");
                fclose(file);
                return -1;
            }
        }
        
        // 줄 끝의 개행 문자 제거
        int len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        
        content->lines[content->count] = strdup(buffer);
        content->count++;
    }
    
    fclose(file);
    return 0;
}

// 메모리 해제
void free_file_content(FileContent* content) {
    if (content->lines) {
        for (int i = 0; i < content->count; i++) {
            free(content->lines[i]);
        }
        free(content->lines);
    }
    content->lines = NULL;
    content->count = 0;
    content->capacity = 0;
}

// 문자열 비교 (옵션에 따라)
int compare_lines(const char* line1, const char* line2, int ignore_case, int ignore_space, int ignore_all_space) {
    if (ignore_case) {
        return strcasecmp(line1, line2);
    } else {
        return strcmp(line1, line2);
    }
}

// LCS (Longest Common Subsequence) 알고리즘을 사용한 diff 계산
void calculate_diff(FileContent* file1, FileContent* file2, int ignore_case, int ignore_space, int ignore_all_space) {
    int m = file1->count;
    int n = file2->count;
    
    // DP 테이블 생성
    int** dp = malloc((m + 1) * sizeof(int*));
    for (int i = 0; i <= m; i++) {
        dp[i] = malloc((n + 1) * sizeof(int));
        memset(dp[i], 0, (n + 1) * sizeof(int));
    }
    
    // LCS 계산
    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            if (compare_lines(file1->lines[i-1], file2->lines[j-1], ignore_case, ignore_space, ignore_all_space) == 0) {
                dp[i][j] = dp[i-1][j-1] + 1;
            } else {
                dp[i][j] = (dp[i-1][j] > dp[i][j-1]) ? dp[i-1][j] : dp[i][j-1];
            }
        }
    }
    
    // 역추적하여 차이점 출력
    int i = m, j = n;
    int changes = 0;
    
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && compare_lines(file1->lines[i-1], file2->lines[j-1], ignore_case, ignore_space, ignore_all_space) == 0) {
            i--;
            j--;
        } else if (i > 0 && (j == 0 || dp[i-1][j] >= dp[i][j-1])) {
            printf("%dd%d\n", i, j);
            printf("< %s\n", file1->lines[i-1]);
            i--;
            changes++;
        } else {
            printf("%da%d\n", i, j);
            printf("> %s\n", file2->lines[j-1]);
            j--;
            changes++;
        }
    }
    
    // 메모리 해제
    for (int i = 0; i <= m; i++) {
        free(dp[i]);
    }
    free(dp);
}

// 통합 diff 형식 출력
void print_unified_diff(const char* file1_name, const char* file2_name, 
                       FileContent* file1, FileContent* file2, 
                       int context_lines, int ignore_case, int ignore_space, int ignore_all_space) {
    printf("--- %s\n", file1_name);
    printf("+++ %s\n", file2_name);
    
    int m = file1->count;
    int n = file2->count;
    
    // 간단한 라인별 비교
    int max_lines = (m > n) ? m : n;
    int line1 = 0, line2 = 0;
    int hunk_start = -1;
    int has_changes = 0;
    
    for (int i = 0; i < max_lines; i++) {
        int diff_found = 0;
        
        if (line1 < m && line2 < n) {
            if (compare_lines(file1->lines[line1], file2->lines[line2], ignore_case, ignore_space, ignore_all_space) != 0) {
                diff_found = 1;
            }
        } else if (line1 < m || line2 < n) {
            diff_found = 1;
        }
        
        if (diff_found) {
            if (hunk_start == -1) {
                hunk_start = i;
                printf("@@ -%d,%d +%d,%d @@\n", 
                       line1 + 1, (m - line1 > 0) ? m - line1 : 1,
                       line2 + 1, (n - line2 > 0) ? n - line2 : 1);
            }
            
            if (line1 < m && line2 < n) {
                printf("-%s\n", file1->lines[line1]);
                printf("+%s\n", file2->lines[line2]);
                line1++;
                line2++;
            } else if (line1 < m) {
                printf("-%s\n", file1->lines[line1]);
                line1++;
            } else if (line2 < n) {
                printf("+%s\n", file2->lines[line2]);
                line2++;
            }
            has_changes = 1;
        } else {
            if (line1 < m && line2 < n) {
                if (hunk_start != -1) {
                    printf(" %s\n", file1->lines[line1]);
                }
                line1++;
                line2++;
            }
        }
    }
    
    if (!has_changes) {
        // 파일이 동일함 - 헤더만 출력됨
    }
}

// 컨텍스트 diff 형식 출력
void print_context_diff(const char* file1_name, const char* file2_name,
                       FileContent* file1, FileContent* file2,
                       int context_lines, int ignore_case, int ignore_space, int ignore_all_space) {
    printf("*** %s\n", file1_name);
    printf("--- %s\n", file2_name);
    
    // 간단한 구현: 모든 다른 라인 출력
    int m = file1->count;
    int n = file2->count;
    int max_lines = (m > n) ? m : n;
    
    for (int i = 0; i < max_lines; i++) {
        if (i < m && i < n) {
            if (compare_lines(file1->lines[i], file2->lines[i], ignore_case, ignore_space, ignore_all_space) != 0) {
                printf("***************\n");
                printf("*** %d ****\n", i + 1);
                printf("! %s\n", file1->lines[i]);
                printf("--- %d ----\n", i + 1);
                printf("! %s\n", file2->lines[i]);
            }
        } else if (i < m) {
            printf("***************\n");
            printf("*** %d ****\n", i + 1);
            printf("- %s\n", file1->lines[i]);
        } else if (i < n) {
            printf("***************\n");
            printf("--- %d ----\n", i + 1);
            printf("+ %s\n", file2->lines[i]);
        }
    }
}

// 파일이 동일한지 확인
int files_identical(FileContent* file1, FileContent* file2, int ignore_case, int ignore_space, int ignore_all_space) {
    if (file1->count != file2->count) {
        return 0;
    }
    
    for (int i = 0; i < file1->count; i++) {
        if (compare_lines(file1->lines[i], file2->lines[i], ignore_case, ignore_space, ignore_all_space) != 0) {
            return 0;
        }
    }
    
    return 1;
}

int main(int argc, char* argv[]) {
    int unified_format = 0;
    int context_format = 0;
    int context_lines = 3;
    int ignore_case = 0;
    int ignore_space = 0;
    int ignore_all_space = 0;
    int brief = 0;
    int report_identical = 0;
    int opt_index = 1;
    
    // 옵션 파싱
    while (opt_index < argc && argv[opt_index][0] == '-') {
        if (strcmp(argv[opt_index], "-u") == 0 || strcmp(argv[opt_index], "--unified") == 0) {
            unified_format = 1;
        } else if (strcmp(argv[opt_index], "-c") == 0 || strcmp(argv[opt_index], "--context") == 0) {
            context_format = 1;
        } else if (strcmp(argv[opt_index], "-i") == 0 || strcmp(argv[opt_index], "--ignore-case") == 0) {
            ignore_case = 1;
        } else if (strcmp(argv[opt_index], "-w") == 0 || strcmp(argv[opt_index], "--ignore-all-space") == 0) {
            ignore_all_space = 1;
        } else if (strcmp(argv[opt_index], "-b") == 0 || strcmp(argv[opt_index], "--ignore-space-change") == 0) {
            ignore_space = 1;
        } else if (strcmp(argv[opt_index], "-q") == 0 || strcmp(argv[opt_index], "--brief") == 0) {
            brief = 1;
        } else if (strcmp(argv[opt_index], "-s") == 0 || strcmp(argv[opt_index], "--report-identical-files") == 0) {
            report_identical = 1;
        } else if (strcmp(argv[opt_index], "-h") == 0 || strcmp(argv[opt_index], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "diff: invalid option -- '%s'\n", argv[opt_index]);
            print_usage(argv[0]);
            return 1;
        }
        opt_index++;
    }
    
    // 파일 인자 확인
    if (argc - opt_index != 2) {
        fprintf(stderr, "diff: missing operand after '%s'\n", argc > opt_index ? argv[opt_index] : "");
        print_usage(argv[0]);
        return 1;
    }
    
    const char* file1_name = argv[opt_index];
    const char* file2_name = argv[opt_index + 1];
    
    // 파일 존재 확인
    struct stat st1, st2;
    if (stat(file1_name, &st1) == -1) {
        fprintf(stderr, "diff: %s: %s\n", file1_name, strerror(errno));
        return 2;
    }
    if (stat(file2_name, &st2) == -1) {
        fprintf(stderr, "diff: %s: %s\n", file2_name, strerror(errno));
        return 2;
    }
    
    // 디렉토리인지 확인
    if (S_ISDIR(st1.st_mode) || S_ISDIR(st2.st_mode)) {
        fprintf(stderr, "diff: directory comparison not implemented\n");
        return 2;
    }
    
    // 파일 로드
    FileContent file1_content = {0};
    FileContent file2_content = {0};
    
    if (load_file(file1_name, &file1_content) == -1) {
        return 2;
    }
    
    if (load_file(file2_name, &file2_content) == -1) {
        free_file_content(&file1_content);
        return 2;
    }
    
    // 파일 비교
    int identical = files_identical(&file1_content, &file2_content, ignore_case, ignore_space, ignore_all_space);
    
    if (identical) {
        if (report_identical) {
            printf("Files %s and %s are identical\n", file1_name, file2_name);
        }
        free_file_content(&file1_content);
        free_file_content(&file2_content);
        return 0;
    }
    
    if (brief) {
        printf("Files %s and %s differ\n", file1_name, file2_name);
    } else if (unified_format) {
        print_unified_diff(file1_name, file2_name, &file1_content, &file2_content, 
                          context_lines, ignore_case, ignore_space, ignore_all_space);
    } else if (context_format) {
        print_context_diff(file1_name, file2_name, &file1_content, &file2_content,
                          context_lines, ignore_case, ignore_space, ignore_all_space);
    } else {
        // 기본 diff 형식
        calculate_diff(&file1_content, &file2_content, ignore_case, ignore_space, ignore_all_space);
    }
    
    free_file_content(&file1_content);
    free_file_content(&file2_content);
    
    return 1; // 파일이 다름
}
```

# II. 시스템 및 프로세스 관리 (System and Process Management) 

## ps: 현재 실행 중인 프로세스 목록 출력
- -aux: 모든 사용자 프로세스 상세 정보
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>

#define MAX_CMD_LEN 256
#define MAX_PATH_LEN 512

// 프로세스 정보 구조체
typedef struct {
    int pid;
    char user[32];
    char state;
    float cpu_percent;
    float mem_percent;
    char cmd[256];
    long vsz;  // Virtual memory size
    long rss;  // Resident set size
    char tty[16];
    char stat[16];
    char start_time[16];
} ProcessInfo;

// 숫자인지 확인하는 함수 (PID 디렉토리 구분용)
int is_number(const char *str) {
    while (*str) {
        if (*str < '0' || *str > '9') return 0;
        str++;
    }
    return 1;
}

// /proc/pid/stat 파일에서 프로세스 정보 읽기
int read_proc_stat(int pid, ProcessInfo *proc) {
    char path[MAX_PATH_LEN];
    FILE *file;
    char comm[256];
    char state;
    int ppid, pgrp, session, tty_nr;
    unsigned long utime, stime, vsize;
    long rss;
    
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    file = fopen(path, "r");
    if (!file) return -1;
    
    // stat 파일의 주요 필드들 읽기
    fscanf(file, "%d %s %c %d %d %d %d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %*u %lu %ld",
           &proc->pid, comm, &state, &ppid, &pgrp, &session, &tty_nr,
           &utime, &stime, &vsize, &rss);
    
    proc->state = state;
    proc->vsz = vsize / 1024; // KB로 변환
    proc->rss = rss * 4; // 페이지 크기 4KB로 가정
    
    // 명령어 이름에서 괄호 제거
    if (comm[0] == '(') {
        strncpy(proc->cmd, comm + 1, sizeof(proc->cmd) - 1);
        proc->cmd[strlen(proc->cmd) - 1] = '\0'; // 마지막 ')' 제거
    } else {
        strncpy(proc->cmd, comm, sizeof(proc->cmd) - 1);
    }
    
    fclose(file);
    return 0;
}

// /proc/pid/status 파일에서 사용자 정보 읽기
int read_proc_status(int pid, ProcessInfo *proc) {
    char path[MAX_PATH_LEN];
    FILE *file;
    char line[256];
    uid_t uid = -1;
    
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    file = fopen(path, "r");
    if (!file) return -1;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line, "Uid:\t%d", &uid);
            break;
        }
    }
    fclose(file);
    
    if (uid != -1) {
        struct passwd *pw = getpwuid(uid);
        if (pw) {
            strncpy(proc->user, pw->pw_name, sizeof(proc->user) - 1);
        } else {
            snprintf(proc->user, sizeof(proc->user), "%d", uid);
        }
    } else {
        strcpy(proc->user, "unknown");
    }
    
    return 0;
}

// 메모리 사용률 계산
float calculate_mem_percent(long rss) {
    FILE *file = fopen("/proc/meminfo", "r");
    if (!file) return 0.0;
    
    char line[256];
    long total_mem = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line, "MemTotal: %ld kB", &total_mem);
            break;
        }
    }
    fclose(file);
    
    if (total_mem > 0) {
        return ((float)rss / total_mem) * 100.0;
    }
    return 0.0;
}

// 프로세스 정보 수집
int get_process_info(int pid, ProcessInfo *proc) {
    if (read_proc_stat(pid, proc) != 0) return -1;
    if (read_proc_status(pid, proc) != 0) return -1;
    
    proc->cpu_percent = 0.0; // 간단한 구현에서는 0으로 설정
    proc->mem_percent = calculate_mem_percent(proc->rss);
    strcpy(proc->tty, "?"); // 간단한 구현에서는 ? 로 설정
    snprintf(proc->stat, sizeof(proc->stat), "%c", proc->state);
    strcpy(proc->start_time, "00:00"); // 간단한 구현에서는 기본값
    
    return 0;
}

// ps 명령어 구현
void ps_command(int show_all) {
    DIR *proc_dir;
    struct dirent *entry;
    ProcessInfo proc;
    
    proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("opendir /proc");
        return;
    }
    
    if (show_all) {
        printf("USER       PID %%CPU %%MEM    VSZ   RSS TTY      STAT START   TIME COMMAND\n");
    } else {
        printf("  PID TTY          TIME CMD\n");
    }
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (!is_number(entry->d_name)) continue;
        
        int pid = atoi(entry->d_name);
        if (get_process_info(pid, &proc) == 0) {
            if (show_all) {
                printf("%-8s %5d %4.1f %4.1f %6ld %5ld %-8s %-4s %5s %7s %s\n",
                       proc.user, proc.pid, proc.cpu_percent, proc.mem_percent,
                       proc.vsz, proc.rss, proc.tty, proc.stat, 
                       proc.start_time, "00:00:00", proc.cmd);
            } else {
                printf("%5d %-12s %8s %s\n", proc.pid, proc.tty, "00:00:00", proc.cmd);
            }
        }
    }
    
    closedir(proc_dir);
}

// 명령어 파싱 및 실행
void execute_command(char *cmd) {
    char *token;
    char *args[10];
    int argc = 0;
    
    // 명령어를 공백으로 분리
    token = strtok(cmd, " \t\n");
    while (token != NULL && argc < 9) {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;
    
    if (argc == 0) return;
    
    // ps 명령어 처리
    if (strcmp(args[0], "ps") == 0) {
        int show_all = 0;
        
        // 옵션 확인
        for (int i = 1; i < argc; i++) {
            if (strcmp(args[i], "-aux") == 0 || strcmp(args[i], "aux") == 0) {
                show_all = 1;
                break;
            }
        }
        
        ps_command(show_all);
    }
    // exit 명령어
    else if (strcmp(args[0], "exit") == 0) {
        printf("Goodbye!\n");
        exit(0);
    }
    // help 명령어
    else if (strcmp(args[0], "help") == 0) {
        printf("Available commands:\n");
        printf("  ps        - Show running processes\n");
        printf("  ps -aux   - Show all processes with detailed info\n");
        printf("  help      - Show this help message\n");
        printf("  exit      - Exit the terminal\n");
    }
    // 알 수 없는 명령어
    else {
        printf("Unknown command: %s\n", args[0]);
        printf("Type 'help' for available commands.\n");
    }
}

int main() {
    char input[MAX_CMD_LEN];
    
    printf("Simple Terminal with PS Command\n");
    printf("Type 'help' for available commands, 'exit' to quit.\n\n");
    
    while (1) {
        printf("simple-shell$ ");
        fflush(stdout);
        
        // 사용자 입력 받기
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // 빈 줄 처리
        if (strlen(input) <= 1) continue;
        
        // 명령어 실행
        execute_command(input);
    }
    
    return 0;
}
```

## kill: 프로세스 종료 (PID 사용) - ps도 겉들인인
- -SIGTERM: 정상 종료 요청 (기본)
- -SIGKILL: 강제 종료
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#define MAX_CMD_LEN 256
#define MAX_PATH_LEN 512

// 프로세스 정보 구조체
typedef struct {
    int pid;
    char user[32];
    char state;
    float cpu_percent;
    float mem_percent;
    char cmd[256];
    long vsz;  // Virtual memory size
    long rss;  // Resident set size
    char tty[16];
    char stat[16];
    char start_time[16];
} ProcessInfo;

// 숫자인지 확인하는 함수 (PID 디렉토리 구분용)
int is_number(const char *str) {
    while (*str) {
        if (*str < '0' || *str > '9') return 0;
        str++;
    // kill 명령어 처리
    else if (strcmp(args[0], "kill") == 0) {
        kill_command(argc, args);
    }
    // pgrep 명령어 (프로세스 이름으로 검색)
    else if (strcmp(args[0], "pgrep") == 0) {
        if (argc < 2) {
            printf("Usage: pgrep <process_name>\n");
        } else {
            find_processes_by_name(args[1]);
        }
    }
    return 1;
}

// /proc/pid/stat 파일에서 프로세스 정보 읽기
int read_proc_stat(int pid, ProcessInfo *proc) {
    char path[MAX_PATH_LEN];
    FILE *file;
    char comm[256];
    char state;
    int ppid, pgrp, session, tty_nr;
    unsigned long utime, stime, vsize;
    long rss;
    
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    file = fopen(path, "r");
    if (!file) return -1;
    
    // stat 파일의 주요 필드들 읽기
    fscanf(file, "%d %s %c %d %d %d %d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %*u %lu %ld",
           &proc->pid, comm, &state, &ppid, &pgrp, &session, &tty_nr,
           &utime, &stime, &vsize, &rss);
    
    proc->state = state;
    proc->vsz = vsize / 1024; // KB로 변환
    proc->rss = rss * 4; // 페이지 크기 4KB로 가정
    
    // 명령어 이름에서 괄호 제거
    if (comm[0] == '(') {
        strncpy(proc->cmd, comm + 1, sizeof(proc->cmd) - 1);
        proc->cmd[strlen(proc->cmd) - 1] = '\0'; // 마지막 ')' 제거
    } else {
        strncpy(proc->cmd, comm, sizeof(proc->cmd) - 1);
    }
    
    fclose(file);
    return 0;
}

// /proc/pid/status 파일에서 사용자 정보 읽기
int read_proc_status(int pid, ProcessInfo *proc) {
    char path[MAX_PATH_LEN];
    FILE *file;
    char line[256];
    uid_t uid = -1;
    
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    file = fopen(path, "r");
    if (!file) return -1;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line, "Uid:\t%d", &uid);
            break;
        }
    }
    fclose(file);
    
    if (uid != -1) {
        struct passwd *pw = getpwuid(uid);
        if (pw) {
            strncpy(proc->user, pw->pw_name, sizeof(proc->user) - 1);
        } else {
            snprintf(proc->user, sizeof(proc->user), "%d", uid);
        }
    } else {
        strcpy(proc->user, "unknown");
    }
    
    return 0;
}

// 메모리 사용률 계산
float calculate_mem_percent(long rss) {
    FILE *file = fopen("/proc/meminfo", "r");
    if (!file) return 0.0;
    
    char line[256];
    long total_mem = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line, "MemTotal: %ld kB", &total_mem);
            break;
        }
    }
    fclose(file);
    
    if (total_mem > 0) {
        return ((float)rss / total_mem) * 100.0;
    }
    return 0.0;
}

// kill 명령어 구현
void kill_command(int argc, char **args) {
    if (argc < 2) {
        printf("Usage: kill [signal] <PID>\n");
        printf("       kill <PID>          (sends SIGTERM by default)\n");
        printf("       kill -SIGTERM <PID> (sends SIGTERM - normal termination)\n");
        printf("       kill -SIGKILL <PID> (sends SIGKILL - force kill)\n");
        printf("       kill -9 <PID>       (sends SIGKILL - force kill)\n");
        printf("       kill -15 <PID>      (sends SIGTERM - normal termination)\n");
        return;
    }
    
    int signal_num = SIGTERM; // 기본값은 SIGTERM
    int pid;
    int pid_index = 1; // PID가 있는 인덱스
    
    // 시그널 옵션 파싱
    if (argc >= 3 && args[1][0] == '-') {
        char *signal_str = args[1] + 1; // '-' 제거
        
        if (strcmp(signal_str, "SIGTERM") == 0 || strcmp(signal_str, "15") == 0) {
            signal_num = SIGTERM;
        }
        else if (strcmp(signal_str, "SIGKILL") == 0 || strcmp(signal_str, "9") == 0) {
            signal_num = SIGKILL;
        }
        else if (strcmp(signal_str, "SIGINT") == 0 || strcmp(signal_str, "2") == 0) {
            signal_num = SIGINT;
        }
        else if (strcmp(signal_str, "SIGHUP") == 0 || strcmp(signal_str, "1") == 0) {
            signal_num = SIGHUP;
        }
        else {
            // 숫자로 직접 시그널 지정
            int sig = atoi(signal_str);
            if (sig > 0 && sig < 32) {
                signal_num = sig;
            } else {
                printf("kill: invalid signal specification '%s'\n", signal_str);
                return;
            }
        }
        pid_index = 2;
    }
    
    // PID 파싱
    if (pid_index >= argc) {
        printf("kill: missing PID\n");
        return;
    }
    
    pid = atoi(args[pid_index]);
    if (pid <= 0) {
        printf("kill: invalid PID '%s'\n", args[pid_index]);
        return;
    }
    
    // 프로세스가 존재하는지 확인
    char proc_path[MAX_PATH_LEN];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d", pid);
    
    struct stat st;
    if (stat(proc_path, &st) != 0) {
        printf("kill: (%d) - No such process\n", pid);
        return;
    }
    
    // 시그널 전송
    if (kill(pid, signal_num) == 0) {
        char *signal_name;
        switch (signal_num) {
            case SIGTERM: signal_name = "SIGTERM"; break;
            case SIGKILL: signal_name = "SIGKILL"; break;
            case SIGINT:  signal_name = "SIGINT"; break;
            case SIGHUP:  signal_name = "SIGHUP"; break;
            default:      signal_name = "signal"; break;
        }
        
        if (signal_num == SIGKILL) {
            printf("Process %d force killed with %s\n", pid, signal_name);
        } else {
            printf("Process %d sent %s signal\n", pid, signal_name);
        }
    } else {
        switch (errno) {
            case ESRCH:
                printf("kill: (%d) - No such process\n", pid);
                break;
            case EPERM:
                printf("kill: (%d) - Operation not permitted\n", pid);
                break;
            default:
                printf("kill: (%d) - %s\n", pid, strerror(errno));
                break;
        }
    }
}

// 프로세스 이름으로 PID 찾기 (killall 기능을 위한 헬퍼 함수)
void find_processes_by_name(const char *name) {
    DIR *proc_dir;
    struct dirent *entry;
    ProcessInfo proc;
    int found = 0;
    
    proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("opendir /proc");
        return;
    }
    
    printf("Found processes matching '%s':\n", name);
    printf("  PID USER     COMMAND\n");
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (!is_number(entry->d_name)) continue;
        
        int pid = atoi(entry->d_name);
        if (get_process_info(pid, &proc) == 0) {
            if (strstr(proc.cmd, name) != NULL) {
                printf("%5d %-8s %s\n", proc.pid, proc.user, proc.cmd);
                found = 1;
            }
        }
    }
    
    if (!found) {
        printf("No processes found matching '%s'\n", name);
    }
    
    closedir(proc_dir);
}
int get_process_info(int pid, ProcessInfo *proc) {
    if (read_proc_stat(pid, proc) != 0) return -1;
    if (read_proc_status(pid, proc) != 0) return -1;
    
    proc->cpu_percent = 0.0; // 간단한 구현에서는 0으로 설정
    proc->mem_percent = calculate_mem_percent(proc->rss);
    strcpy(proc->tty, "?"); // 간단한 구현에서는 ? 로 설정
    snprintf(proc->stat, sizeof(proc->stat), "%c", proc->state);
    strcpy(proc->start_time, "00:00"); // 간단한 구현에서는 기본값
    
    return 0;
}

// ps 명령어 구현
void ps_command(int show_all) {
    DIR *proc_dir;
    struct dirent *entry;
    ProcessInfo proc;
    
    proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("opendir /proc");
        return;
    }
    
    if (show_all) {
        printf("USER       PID %%CPU %%MEM    VSZ   RSS TTY      STAT START   TIME COMMAND\n");
    } else {
        printf("  PID TTY          TIME CMD\n");
    }
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (!is_number(entry->d_name)) continue;
        
        int pid = atoi(entry->d_name);
        if (get_process_info(pid, &proc) == 0) {
            if (show_all) {
                printf("%-8s %5d %4.1f %4.1f %6ld %5ld %-8s %-4s %5s %7s %s\n",
                       proc.user, proc.pid, proc.cpu_percent, proc.mem_percent,
                       proc.vsz, proc.rss, proc.tty, proc.stat, 
                       proc.start_time, "00:00:00", proc.cmd);
            } else {
                printf("%5d %-12s %8s %s\n", proc.pid, proc.tty, "00:00:00", proc.cmd);
            }
        }
    }
    
    closedir(proc_dir);
}

// 명령어 파싱 및 실행
void execute_command(char *cmd) {
    char *token;
    char *args[10];
    int argc = 0;
    
    // 명령어를 공백으로 분리
    token = strtok(cmd, " \t\n");
    while (token != NULL && argc < 9) {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;
    
    if (argc == 0) return;
    
    // ps 명령어 처리
    if (strcmp(args[0], "ps") == 0) {
        int show_all = 0;
        
        // 옵션 확인
        for (int i = 1; i < argc; i++) {
            if (strcmp(args[i], "-aux") == 0 || strcmp(args[i], "aux") == 0) {
                show_all = 1;
                break;
            }
        }
        
        ps_command(show_all);
    }
    // exit 명령어
    else if (strcmp(args[0], "exit") == 0) {
        printf("Goodbye!\n");
        exit(0);
    }
    // help 명령어
    else if (strcmp(args[0], "help") == 0) {
        printf("Available commands:\n");
        printf("  ps              - Show running processes\n");
        printf("  ps -aux         - Show all processes with detailed info\n");
        printf("  kill <PID>      - Send SIGTERM to process (normal termination)\n");
        printf("  kill -SIGTERM <PID> - Send SIGTERM to process\n");
        printf("  kill -SIGKILL <PID> - Send SIGKILL to process (force kill)\n");
        printf("  kill -9 <PID>   - Send SIGKILL to process (force kill)\n");
        printf("  kill -15 <PID>  - Send SIGTERM to process\n");
        printf("  pgrep <name>    - Find processes by name\n");
        printf("  help            - Show this help message\n");
        printf("  exit            - Exit the terminal\n");
    }
    // 알 수 없는 명령어
    else {
        printf("Unknown command: %s\n", args[0]);
        printf("Type 'help' for available commands.\n");
    }
}

int main() {
    char input[MAX_CMD_LEN];
    
    printf("Simple Terminal with PS and KILL Commands\n");
    printf("Type 'help' for available commands, 'exit' to quit.\n");
    printf("Warning: Use kill commands carefully!\n\n");
    
    while (1) {
        printf("simple-shell$ ");
        fflush(stdout);
        
        // 사용자 입력 받기
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // 빈 줄 처리
        if (strlen(input) <= 1) continue;
        
        // 명령어 실행
        execute_command(input);
    }
    
    return 0;
}
```

## top (간략화): 실시간 프로세스 모니터링 (CPU, 메모리 사용량 등) - ps와 kill을 겉들인
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>

#define MAX_CMD_LEN 256
#define MAX_PATH_LEN 512

// 프로세스 정보 구조체
typedef struct {
    int pid;
    char user[32];
    char state;
    float cpu_percent;
    float mem_percent;
    char cmd[256];
    long vsz;  // Virtual memory size
    long rss;  // Resident set size
    char tty[16];
    char stat[16];
    char start_time[16];
    unsigned long utime;  // user time
    unsigned long stime;  // system time
    unsigned long total_time; // total time
} ProcessInfo;

// 시스템 정보 구조체
typedef struct {
    long total_mem;
    long free_mem;
    long used_mem;
    long cached_mem;
    float load_avg[3];
    int num_processes;
    int num_running;
    int num_sleeping;
    int num_zombie;
    unsigned long total_cpu_time;
    unsigned long idle_cpu_time;
} SystemInfo;

// 숫자인지 확인하는 함수 (PID 디렉토리 구분용)
int is_number(const char *str) {
    while (*str) {
        if (*str < '0' || *str > '9') return 0;
        str++;
    // kill 명령어 처리
    else if (strcmp(args[0], "kill") == 0) {
        kill_command(argc, args);
    // top 명령어 처리
    else if (strcmp(args[0], "top") == 0) {
        top_command();
    }
    // pgrep 명령어 (프로세스 이름으로 검색)
    else if (strcmp(args[0], "pgrep") == 0) {
        if (argc < 2) {
            printf("Usage: pgrep <process_name>\n");
        } else {
            find_processes_by_name(args[1]);
        }
    }
    return 1;
}

// /proc/pid/stat 파일에서 프로세스 정보 읽기
int read_proc_stat(int pid, ProcessInfo *proc) {
    char path[MAX_PATH_LEN];
    FILE *file;
    char comm[256];
    char state;
    int ppid, pgrp, session, tty_nr;
    unsigned long utime, stime, vsize;
    long rss;
    
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    file = fopen(path, "r");
    if (!file) return -1;
    
    // stat 파일의 주요 필드들 읽기
    fscanf(file, "%d %s %c %d %d %d %d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %*u %lu %ld",
           &proc->pid, comm, &state, &ppid, &pgrp, &session, &tty_nr,
           &utime, &stime, &vsize, &rss);
    
    proc->state = state;
    proc->vsz = vsize / 1024; // KB로 변환
    proc->rss = rss * 4; // 페이지 크기 4KB로 가정
    proc->utime = utime;
    proc->stime = stime;
    proc->total_time = utime + stime;
    
    // 명령어 이름에서 괄호 제거
    if (comm[0] == '(') {
        strncpy(proc->cmd, comm + 1, sizeof(proc->cmd) - 1);
        proc->cmd[strlen(proc->cmd) - 1] = '\0'; // 마지막 ')' 제거
    } else {
        strncpy(proc->cmd, comm, sizeof(proc->cmd) - 1);
    }
    
    fclose(file);
    return 0;
}

// /proc/pid/status 파일에서 사용자 정보 읽기
int read_proc_status(int pid, ProcessInfo *proc) {
    char path[MAX_PATH_LEN];
    FILE *file;
    char line[256];
    uid_t uid = -1;
    
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    file = fopen(path, "r");
    if (!file) return -1;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line, "Uid:\t%d", &uid);
            break;
        }
    }
    fclose(file);
    
    if (uid != -1) {
        struct passwd *pw = getpwuid(uid);
        if (pw) {
            strncpy(proc->user, pw->pw_name, sizeof(proc->user) - 1);
        } else {
            snprintf(proc->user, sizeof(proc->user), "%d", uid);
        }
    } else {
        strcpy(proc->user, "unknown");
    }
    
    return 0;
}

// 시스템 정보 읽기
int get_system_info(SystemInfo *sys_info) {
    FILE *file;
    char line[256];
    
    // 메모리 정보 읽기
    file = fopen("/proc/meminfo", "r");
    if (!file) return -1;
    
    sys_info->total_mem = 0;
    sys_info->free_mem = 0;
    sys_info->cached_mem = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line, "MemTotal: %ld kB", &sys_info->total_mem);
        } else if (strncmp(line, "MemFree:", 8) == 0) {
            sscanf(line, "MemFree: %ld kB", &sys_info->free_mem);
        } else if (strncmp(line, "Cached:", 7) == 0) {
            sscanf(line, "Cached: %ld kB", &sys_info->cached_mem);
        }
    }
    fclose(file);
    
    sys_info->used_mem = sys_info->total_mem - sys_info->free_mem - sys_info->cached_mem;
    
    // Load average 읽기
    file = fopen("/proc/loadavg", "r");
    if (file) {
        fscanf(file, "%f %f %f", &sys_info->load_avg[0], &sys_info->load_avg[1], &sys_info->load_avg[2]);
        fclose(file);
    }
    
    // CPU 정보 읽기
    file = fopen("/proc/stat", "r");
    if (file) {
        fgets(line, sizeof(line), file);
        unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
        sscanf(line, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
        
        sys_info->total_cpu_time = user + nice + system + idle + iowait + irq + softirq + steal;
        sys_info->idle_cpu_time = idle + iowait;
        fclose(file);
    }
    
    return 0;
}

// 프로세스 수 계산
void count_processes(SystemInfo *sys_info) {
    DIR *proc_dir;
    struct dirent *entry;
    ProcessInfo proc;
    
    sys_info->num_processes = 0;
    sys_info->num_running = 0;
    sys_info->num_sleeping = 0;
    sys_info->num_zombie = 0;
    
    proc_dir = opendir("/proc");
    if (!proc_dir) return;
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (!is_number(entry->d_name)) continue;
        
        int pid = atoi(entry->d_name);
        if (read_proc_stat(pid, &proc) == 0) {
            sys_info->num_processes++;
            switch (proc.state) {
                case 'R': sys_info->num_running++; break;
                case 'S': case 'D': sys_info->num_sleeping++; break;
                case 'Z': sys_info->num_zombie++; break;
            }
        }
    }
    
    closedir(proc_dir);
}

// 터미널 설정 변경 (non-blocking input)
void set_terminal_mode(int enable) {
    static struct termios old_termios;
    static int is_set = 0;
    
    if (enable && !is_set) {
        tcgetattr(STDIN_FILENO, &old_termios);
        struct termios new_termios = old_termios;
        new_termios.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
        is_set = 1;
    } else if (!enable && is_set) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
        fcntl(STDIN_FILENO, F_SETFL, 0);
        is_set = 0;
    }
}

// 화면 지우기
void clear_screen() {
    printf("\033[2J\033[H");
    fflush(stdout);
}

// CPU 사용률 계산을 위한 이전 값들 저장
static unsigned long prev_total_time[1000] = {0};
static unsigned long prev_cpu_total = 0;
static unsigned long prev_cpu_idle = 0;

// CPU 사용률 계산
float calculate_cpu_usage(ProcessInfo *proc, unsigned long total_cpu_time, unsigned long idle_cpu_time) {
    static int first_call = 1;
    
    if (first_call) {
        prev_total_time[proc->pid % 1000] = proc->total_time;
        prev_cpu_total = total_cpu_time;
        prev_cpu_idle = idle_cpu_time;
        first_call = 0;
        return 0.0;
    }
    
    unsigned long proc_time_diff = proc->total_time - prev_total_time[proc->pid % 1000];
    unsigned long cpu_time_diff = total_cpu_time - prev_cpu_total;
    
    prev_total_time[proc->pid % 1000] = proc->total_time;
    
    if (cpu_time_diff > 0) {
        return ((float)proc_time_diff / cpu_time_diff) * 100.0;
    }
    return 0.0;
}

// top 명령어 구현
void top_command() {
    SystemInfo sys_info;
    ProcessInfo processes[100];  // 최대 100개 프로세스 표시
    int process_count = 0;
    
    set_terminal_mode(1);  // non-blocking 모드 활성화
    
    printf("Press 'q' to quit top\n");
    sleep(1);
    
    while (1) {
        clear_screen();
        
        // 시스템 정보 수집
        get_system_info(&sys_info);
        count_processes(&sys_info);
        
        // 헤더 정보 출력
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        printf("top - %02d:%02d:%02d up time, load average: %.2f, %.2f, %.2f\n",
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
               sys_info.load_avg[0], sys_info.load_avg[1], sys_info.load_avg[2]);
        
        printf("Tasks: %d total, %d running, %d sleeping, %d zombie\n",
               sys_info.num_processes, sys_info.num_running, 
               sys_info.num_sleeping, sys_info.num_zombie);
        
        float cpu_usage = 100.0 - ((float)sys_info.idle_cpu_time / sys_info.total_cpu_time * 100.0);
        printf("%%Cpu(s): %.1f us, %.1f sy, %.1f id\n", cpu_usage * 0.7, cpu_usage * 0.3, 100.0 - cpu_usage);
        
        printf("KiB Mem: %8ld total, %8ld free, %8ld used, %8ld buff/cache\n",
               sys_info.total_mem, sys_info.free_mem, sys_info.used_mem, sys_info.cached_mem);
        
        printf("\n");
        printf("  PID USER      PR  NI    VIRT    RES    SHR S  %%CPU %%MEM     TIME+ COMMAND\n");
        
        // 프로세스 정보 수집
        DIR *proc_dir = opendir("/proc");
        if (proc_dir) {
            struct dirent *entry;
            process_count = 0;
            
            while ((entry = readdir(proc_dir)) != NULL && process_count < 100) {
                if (!is_number(entry->d_name)) continue;
                
                int pid = atoi(entry->d_name);
                ProcessInfo *proc = &processes[process_count];
                
                if (get_process_info(pid, proc) == 0) {
                    proc->cpu_percent = calculate_cpu_usage(proc, sys_info.total_cpu_time, sys_info.idle_cpu_time);
                    process_count++;
                }
            }
            closedir(proc_dir);
        }
        
        // CPU 사용률 기준으로 정렬 (간단한 버블 정렬)
        for (int i = 0; i < process_count - 1; i++) {
            for (int j = 0; j < process_count - i - 1; j++) {
                if (processes[j].cpu_percent < processes[j + 1].cpu_percent) {
                    ProcessInfo temp = processes[j];
                    processes[j] = processes[j + 1];
                    processes[j + 1] = temp;
                }
            }
        }
        
        // 프로세스 목록 출력 (상위 20개)
        int display_count = (process_count > 20) ? 20 : process_count;
        for (int i = 0; i < display_count; i++) {
            ProcessInfo *proc = &processes[i];
            printf("%5d %-8s 20   0 %7ld %6ld      0 %c %5.1f %4.1f %8s %s\n",
                   proc->pid, proc->user, proc->vsz, proc->rss,
                   proc->state, proc->cpu_percent, proc->mem_percent,
                   "00:00:00", proc->cmd);
        }
        
        printf("\nPress 'q' to quit, any other key to refresh...");
        fflush(stdout);
        
        // 키 입력 확인 (non-blocking)
        char ch;
        if (read(STDIN_FILENO, &ch, 1) > 0) {
            if (ch == 'q' || ch == 'Q') {
                break;
            }
        }
        
        sleep(2);  // 2초마다 갱신
    }
    
    set_terminal_mode(0);  // 터미널 모드 복원
    clear_screen();
    printf("Exited top mode.\n");
}
float calculate_mem_percent(long rss) {
    FILE *file = fopen("/proc/meminfo", "r");
    if (!file) return 0.0;
    
    char line[256];
    long total_mem = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line, "MemTotal: %ld kB", &total_mem);
            break;
        }
    }
    fclose(file);
    
    if (total_mem > 0) {
        return ((float)rss / total_mem) * 100.0;
    }
    return 0.0;
}

// kill 명령어 구현
void kill_command(int argc, char **args) {
    if (argc < 2) {
        printf("Usage: kill [signal] <PID>\n");
        printf("       kill <PID>          (sends SIGTERM by default)\n");
        printf("       kill -SIGTERM <PID> (sends SIGTERM - normal termination)\n");
        printf("       kill -SIGKILL <PID> (sends SIGKILL - force kill)\n");
        printf("       kill -9 <PID>       (sends SIGKILL - force kill)\n");
        printf("       kill -15 <PID>      (sends SIGTERM - normal termination)\n");
        return;
    }
    
    int signal_num = SIGTERM; // 기본값은 SIGTERM
    int pid;
    int pid_index = 1; // PID가 있는 인덱스
    
    // 시그널 옵션 파싱
    if (argc >= 3 && args[1][0] == '-') {
        char *signal_str = args[1] + 1; // '-' 제거
        
        if (strcmp(signal_str, "SIGTERM") == 0 || strcmp(signal_str, "15") == 0) {
            signal_num = SIGTERM;
        }
        else if (strcmp(signal_str, "SIGKILL") == 0 || strcmp(signal_str, "9") == 0) {
            signal_num = SIGKILL;
        }
        else if (strcmp(signal_str, "SIGINT") == 0 || strcmp(signal_str, "2") == 0) {
            signal_num = SIGINT;
        }
        else if (strcmp(signal_str, "SIGHUP") == 0 || strcmp(signal_str, "1") == 0) {
            signal_num = SIGHUP;
        }
        else {
            // 숫자로 직접 시그널 지정
            int sig = atoi(signal_str);
            if (sig > 0 && sig < 32) {
                signal_num = sig;
            } else {
                printf("kill: invalid signal specification '%s'\n", signal_str);
                return;
            }
        }
        pid_index = 2;
    }
    
    // PID 파싱
    if (pid_index >= argc) {
        printf("kill: missing PID\n");
        return;
    }
    
    pid = atoi(args[pid_index]);
    if (pid <= 0) {
        printf("kill: invalid PID '%s'\n", args[pid_index]);
        return;
    }
    
    // 프로세스가 존재하는지 확인
    char proc_path[MAX_PATH_LEN];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d", pid);
    
    struct stat st;
    if (stat(proc_path, &st) != 0) {
        printf("kill: (%d) - No such process\n", pid);
        return;
    }
    
    // 시그널 전송
    if (kill(pid, signal_num) == 0) {
        char *signal_name;
        switch (signal_num) {
            case SIGTERM: signal_name = "SIGTERM"; break;
            case SIGKILL: signal_name = "SIGKILL"; break;
            case SIGINT:  signal_name = "SIGINT"; break;
            case SIGHUP:  signal_name = "SIGHUP"; break;
            default:      signal_name = "signal"; break;
        }
        
        if (signal_num == SIGKILL) {
            printf("Process %d force killed with %s\n", pid, signal_name);
        } else {
            printf("Process %d sent %s signal\n", pid, signal_name);
        }
    } else {
        switch (errno) {
            case ESRCH:
                printf("kill: (%d) - No such process\n", pid);
                break;
            case EPERM:
                printf("kill: (%d) - Operation not permitted\n", pid);
                break;
            default:
                printf("kill: (%d) - %s\n", pid, strerror(errno));
                break;
        }
    }
}

// 프로세스 이름으로 PID 찾기 (killall 기능을 위한 헬퍼 함수)
void find_processes_by_name(const char *name) {
    DIR *proc_dir;
    struct dirent *entry;
    ProcessInfo proc;
    int found = 0;
    
    proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("opendir /proc");
        return;
    }
    
    printf("Found processes matching '%s':\n", name);
    printf("  PID USER     COMMAND\n");
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (!is_number(entry->d_name)) continue;
        
        int pid = atoi(entry->d_name);
        if (get_process_info(pid, &proc) == 0) {
            if (strstr(proc.cmd, name) != NULL) {
                printf("%5d %-8s %s\n", proc.pid, proc.user, proc.cmd);
                found = 1;
            }
        }
    }
    
    if (!found) {
        printf("No processes found matching '%s'\n", name);
    }
    
    closedir(proc_dir);
}
int get_process_info(int pid, ProcessInfo *proc) {
    if (read_proc_stat(pid, proc) != 0) return -1;
    if (read_proc_status(pid, proc) != 0) return -1;
    
    proc->cpu_percent = 0.0; // 간단한 구현에서는 0으로 설정
    proc->mem_percent = calculate_mem_percent(proc->rss);
    strcpy(proc->tty, "?"); // 간단한 구현에서는 ? 로 설정
    snprintf(proc->stat, sizeof(proc->stat), "%c", proc->state);
    strcpy(proc->start_time, "00:00"); // 간단한 구현에서는 기본값
    
    return 0;
}

// ps 명령어 구현
void ps_command(int show_all) {
    DIR *proc_dir;
    struct dirent *entry;
    ProcessInfo proc;
    
    proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("opendir /proc");
        return;
    }
    
    if (show_all) {
        printf("USER       PID %%CPU %%MEM    VSZ   RSS TTY      STAT START   TIME COMMAND\n");
    } else {
        printf("  PID TTY          TIME CMD\n");
    }
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (!is_number(entry->d_name)) continue;
        
        int pid = atoi(entry->d_name);
        if (get_process_info(pid, &proc) == 0) {
            if (show_all) {
                printf("%-8s %5d %4.1f %4.1f %6ld %5ld %-8s %-4s %5s %7s %s\n",
                       proc.user, proc.pid, proc.cpu_percent, proc.mem_percent,
                       proc.vsz, proc.rss, proc.tty, proc.stat, 
                       proc.start_time, "00:00:00", proc.cmd);
            } else {
                printf("%5d %-12s %8s %s\n", proc.pid, proc.tty, "00:00:00", proc.cmd);
            }
        }
    }
    
    closedir(proc_dir);
}

// 명령어 파싱 및 실행
void execute_command(char *cmd) {
    char *token;
    char *args[10];
    int argc = 0;
    
    // 명령어를 공백으로 분리
    token = strtok(cmd, " \t\n");
    while (token != NULL && argc < 9) {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;
    
    if (argc == 0) return;
    
    // ps 명령어 처리
    if (strcmp(args[0], "ps") == 0) {
        int show_all = 0;
        
        // 옵션 확인
        for (int i = 1; i < argc; i++) {
            if (strcmp(args[i], "-aux") == 0 || strcmp(args[i], "aux") == 0) {
                show_all = 1;
                break;
            }
        }
        
        ps_command(show_all);
    }
    // exit 명령어
    else if (strcmp(args[0], "exit") == 0) {
        printf("Goodbye!\n");
        exit(0);
    }
    // help 명령어
    else if (strcmp(args[0], "help") == 0) {
        printf("Available commands:\n");
        printf("  ps              - Show running processes\n");
        printf("  ps -aux         - Show all processes with detailed info\n");
        printf("  kill <PID>      - Send SIGTERM to process (normal termination)\n");
        printf("  kill -SIGTERM <PID> - Send SIGTERM to process\n");
        printf("  kill -SIGKILL <PID> - Send SIGKILL to process (force kill)\n");
        printf("  kill -9 <PID>   - Send SIGKILL to process (force kill)\n");
        printf("  kill -15 <PID>  - Send SIGTERM to process\n");
        printf("  pgrep <name>    - Find processes by name\n");
        printf("  help            - Show this help message\n");
        printf("  exit            - Exit the terminal\n");
    }
    // 알 수 없는 명령어
    else {
        printf("Unknown command: %s\n", args[0]);
        printf("Type 'help' for available commands.\n");
    }
}

int main() {
    char input[MAX_CMD_LEN];
    
    printf("Simple Terminal with PS, KILL, and TOP Commands\n");
    printf("Type 'help' for available commands, 'exit' to quit.\n");
    printf("Warning: Use kill commands carefully!\n\n");
    
    while (1) {
        printf("simple-shell$ ");
        fflush(stdout);
        
        // 사용자 입력 받기
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // 빈 줄 처리
        if (strlen(input) <= 1) continue;
        
        // 명령어 실행
        execute_command(input);
    }
    
    return 0;
}
```

## free: 메모리 사용량 정보 출력 
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 메모리 정보를 저장할 구조체
typedef struct {
    long total;
    long free;
    long available;
    long buffers;
    long cached;
    long swap_total;
    long swap_free;
} MemInfo;

// /proc/meminfo에서 특정 값을 추출하는 함수
long extract_value(const char* line, const char* key) {
    if (strncmp(line, key, strlen(key)) == 0) {
        char* ptr = strchr(line, ':');
        if (ptr) {
            ptr++; // ':' 다음으로 이동
            while (*ptr == ' ' || *ptr == '\t') ptr++; // 공백 건너뛰기
            return atol(ptr);
        }
    }
    return -1;
}

// /proc/meminfo 파일을 읽어서 메모리 정보 파싱
int get_memory_info(MemInfo* mem_info) {
    FILE* file = fopen("/proc/meminfo", "r");
    if (!file) {
        perror("Cannot open /proc/meminfo");
        return -1;
    }

    char line[256];
    long value;

    // 구조체 초기화
    memset(mem_info, 0, sizeof(MemInfo));

    while (fgets(line, sizeof(line), file)) {
        if ((value = extract_value(line, "MemTotal")) != -1) {
            mem_info->total = value;
        } else if ((value = extract_value(line, "MemFree")) != -1) {
            mem_info->free = value;
        } else if ((value = extract_value(line, "MemAvailable")) != -1) {
            mem_info->available = value;
        } else if ((value = extract_value(line, "Buffers")) != -1) {
            mem_info->buffers = value;
        } else if ((value = extract_value(line, "Cached")) != -1) {
            mem_info->cached = value;
        } else if ((value = extract_value(line, "SwapTotal")) != -1) {
            mem_info->swap_total = value;
        } else if ((value = extract_value(line, "SwapFree")) != -1) {
            mem_info->swap_free = value;
        }
    }

    fclose(file);
    return 0;
}

// KB를 다른 단위로 변환하는 함수
void print_memory_size(long kb, int human_readable) {
    if (human_readable) {
        if (kb >= 1024 * 1024) {
            printf("%6.1fG", (double)kb / (1024 * 1024));
        } else if (kb >= 1024) {
            printf("%6.1fM", (double)kb / 1024);
        } else {
            printf("%6ldK", kb);
        }
    } else {
        printf("%12ld", kb);
    }
}

// 메모리 정보를 출력하는 함수
void print_memory_info(MemInfo* mem_info, int human_readable, int show_buffers) {
    long used = mem_info->total - mem_info->free;
    long used_without_cache = used - mem_info->buffers - mem_info->cached;
    long swap_used = mem_info->swap_total - mem_info->swap_free;

    // 헤더 출력
    if (human_readable) {
        printf("%-12s %7s %7s %7s %7s %7s %7s\n", 
               "", "total", "used", "free", "shared", "buff/cache", "available");
    } else {
        printf("%-12s %12s %12s %12s %12s %12s %12s\n", 
               "", "total", "used", "free", "shared", "buff/cache", "available");
    }

    // 메모리 정보 출력
    printf("%-12s ", "Mem:");
    print_memory_size(mem_info->total, human_readable);
    print_memory_size(used, human_readable);
    print_memory_size(mem_info->free, human_readable);
    print_memory_size(0, human_readable); // shared (정확한 값을 얻기 어려우므로 0으로 표시)
    print_memory_size(mem_info->buffers + mem_info->cached, human_readable);
    print_memory_size(mem_info->available, human_readable);
    printf("\n");

    // 버퍼/캐시 정보 출력 (옵션)
    if (show_buffers) {
        printf("%-12s ", "Buffers:");
        print_memory_size(0, human_readable);
        print_memory_size(0, human_readable);
        print_memory_size(mem_info->buffers, human_readable);
        printf("\n");

        printf("%-12s ", "Cache:");
        print_memory_size(0, human_readable);
        print_memory_size(0, human_readable);
        print_memory_size(mem_info->cached, human_readable);
        printf("\n");
    }

    // 스왑 정보 출력
    printf("%-12s ", "Swap:");
    print_memory_size(mem_info->swap_total, human_readable);
    print_memory_size(swap_used, human_readable);
    print_memory_size(mem_info->swap_free, human_readable);
    printf("\n");
}

// 도움말 출력
void print_help() {
    printf("Usage: free [options]\n");
    printf("Options:\n");
    printf("  -h, --human     show human-readable output\n");
    printf("  -b, --bytes     show output in bytes\n");
    printf("  -k, --kilo      show output in kilobytes (default)\n");
    printf("  -m, --mega      show output in megabytes\n");
    printf("  -g, --giga      show output in gigabytes\n");
    printf("  -w, --wide      show buffers and cache separately\n");
    printf("  --help          display this help and exit\n");
}

int main(int argc, char* argv[]) {
    MemInfo mem_info;
    int human_readable = 0;
    int show_buffers = 0;
    int unit_divider = 1; // 1=KB (기본값), 1024=MB, 1024*1024=GB

    // 명령행 인수 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--human") == 0) {
            human_readable = 1;
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bytes") == 0) {
            unit_divider = 1;
            human_readable = 0;
        } else if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--kilo") == 0) {
            unit_divider = 1;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mega") == 0) {
            unit_divider = 1024;
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--giga") == 0) {
            unit_divider = 1024 * 1024;
        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--wide") == 0) {
            show_buffers = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_help();
            return 1;
        }
    }

    // 메모리 정보 가져오기
    if (get_memory_info(&mem_info) != 0) {
        return 1;
    }

    // 단위 변환 (bytes 옵션인 경우 1024를 곱해서 바이트로 변환)
    if (strcmp(argv[1] ? argv[1] : "", "-b") == 0 || strcmp(argv[1] ? argv[1] : "", "--bytes") == 0) {
        mem_info.total *= 1024;
        mem_info.free *= 1024;
        mem_info.available *= 1024;
        mem_info.buffers *= 1024;
        mem_info.cached *= 1024;
        mem_info.swap_total *= 1024;
        mem_info.swap_free *= 1024;
    } else if (unit_divider > 1) {
        mem_info.total /= unit_divider;
        mem_info.free /= unit_divider;
        mem_info.available /= unit_divider;
        mem_info.buffers /= unit_divider;
        mem_info.cached /= unit_divider;
        mem_info.swap_total /= unit_divider;
        mem_info.swap_free /= unit_divider;
    }

    // 메모리 정보 출력
    print_memory_info(&mem_info, human_readable, show_buffers);

    return 0;
}
```

## df: 디스크 공간 사용량 정보 출력
- -h: 사람이 읽기 쉬운 형식

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <mntent.h>

// 파일시스템 정보를 저장할 구조체
typedef struct {
    char filesystem[256];
    char mountpoint[256];
    char fstype[64];
    unsigned long long total;
    unsigned long long used;
    unsigned long long available;
    int use_percent;
} FSInfo;

// 바이트를 사람이 읽기 쉬운 형태로 변환
void format_size(unsigned long long bytes, char* buffer, int human_readable) {
    if (human_readable) {
        if (bytes >= 1099511627776ULL) { // 1TB
            snprintf(buffer, 32, "%.1fT", (double)bytes / 1099511627776.0);
        } else if (bytes >= 1073741824ULL) { // 1GB
            snprintf(buffer, 32, "%.1fG", (double)bytes / 1073741824.0);
        } else if (bytes >= 1048576ULL) { // 1MB
            snprintf(buffer, 32, "%.1fM", (double)bytes / 1048576.0);
        } else if (bytes >= 1024ULL) { // 1KB
            snprintf(buffer, 32, "%.1fK", (double)bytes / 1024.0);
        } else {
            snprintf(buffer, 32, "%lluB", bytes);
        }
    } else {
        // 1K 블록 단위로 출력 (기본값)
        snprintf(buffer, 32, "%llu", bytes / 1024);
    }
}

// 특정 파일시스템 타입을 제외할지 확인
int should_skip_fstype(const char* fstype) {
    const char* skip_types[] = {
        "proc", "sysfs", "devtmpfs", "devpts", "tmpfs", "securityfs",
        "cgroup", "pstore", "efivarfs", "bpf", "cgroup2", "hugetlbfs",
        "debugfs", "tracefs", "fusectl", "configfs", "ramfs", "autofs",
        "rpc_pipefs", "nfsd", NULL
    };
    
    for (int i = 0; skip_types[i]; i++) {
        if (strcmp(fstype, skip_types[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// 파일시스템 정보 가져오기
int get_filesystem_info(const char* mountpoint, FSInfo* fs_info) {
    struct statvfs vfs;
    
    if (statvfs(mountpoint, &vfs) != 0) {
        return -1;
    }
    
    // 블록 크기 계산
    unsigned long long block_size = vfs.f_frsize ? vfs.f_frsize : vfs.f_bsize;
    
    // 총 공간, 사용 공간, 사용 가능 공간 계산
    fs_info->total = vfs.f_blocks * block_size;
    fs_info->available = vfs.f_bavail * block_size;
    fs_info->used = fs_info->total - (vfs.f_bfree * block_size);
    
    // 사용 퍼센트 계산
    if (fs_info->total > 0) {
        fs_info->use_percent = (int)((fs_info->used * 100) / fs_info->total);
    } else {
        fs_info->use_percent = 0;
    }
    
    return 0;
}

// 마운트된 파일시스템 목록 출력
void print_filesystems(int human_readable, int show_all, const char* specific_path) {
    FILE* mounts;
    struct mntent* entry;
    FSInfo fs_info;
    char total_str[32], used_str[32], avail_str[32];
    
    // 헤더 출력
    if (human_readable) {
        printf("%-20s %6s %6s %6s %4s %s\n", 
               "Filesystem", "Size", "Used", "Avail", "Use%", "Mounted on");
    } else {
        printf("%-20s %10s %10s %10s %4s %s\n", 
               "Filesystem", "1K-blocks", "Used", "Available", "Use%", "Mounted on");
    }
    
    // 특정 경로가 지정된 경우
    if (specific_path) {
        struct stat path_stat;
        if (stat(specific_path, &path_stat) == 0) {
            // 경로가 존재하는 경우, 해당 경로의 파일시스템 정보만 출력
            if (get_filesystem_info(specific_path, &fs_info) == 0) {
                format_size(fs_info.total, total_str, human_readable);
                format_size(fs_info.used, used_str, human_readable);
                format_size(fs_info.available, avail_str, human_readable);
                
                printf("%-20s %10s %10s %10s %3d%% %s\n",
                       "filesystem", total_str, used_str, avail_str,
                       fs_info.use_percent, specific_path);
            }
        } else {
            fprintf(stderr, "df: %s: No such file or directory\n", specific_path);
        }
        return;
    }
    
    // /proc/mounts 파일 열기
    mounts = setmntent("/proc/mounts", "r");
    if (!mounts) {
        perror("Cannot open /proc/mounts");
        return;
    }
    
    // 각 마운트 포인트에 대해 정보 수집
    while ((entry = getmntent(mounts)) != NULL) {
        // 가상 파일시스템 제외 (show_all 옵션이 없는 경우)
        if (!show_all && should_skip_fstype(entry->mnt_type)) {
            continue;
        }
        
        // 파일시스템 정보 가져오기
        if (get_filesystem_info(entry->mnt_dir, &fs_info) != 0) {
            continue;
        }
        
        // 0 크기 파일시스템 제외
        if (!show_all && fs_info.total == 0) {
            continue;
        }
        
        // 구조체에 추가 정보 저장
        strncpy(fs_info.filesystem, entry->mnt_fsname, sizeof(fs_info.filesystem) - 1);
        strncpy(fs_info.mountpoint, entry->mnt_dir, sizeof(fs_info.mountpoint) - 1);
        strncpy(fs_info.fstype, entry->mnt_type, sizeof(fs_info.fstype) - 1);
        
        // 크기 포맷팅
        format_size(fs_info.total, total_str, human_readable);
        format_size(fs_info.used, used_str, human_readable);
        format_size(fs_info.available, avail_str, human_readable);
        
        // 출력
        printf("%-20s %10s %10s %10s %3d%% %s\n",
               fs_info.filesystem, total_str, used_str, avail_str,
               fs_info.use_percent, fs_info.mountpoint);
    }
    
    endmntent(mounts);
}

// 도움말 출력
void print_help() {
    printf("Usage: df [OPTION]... [FILE]...\n");
    printf("Show information about the file system on which each FILE resides,\n");
    printf("or all file systems by default.\n\n");
    printf("Options:\n");
    printf("  -a, --all             include dummy file systems\n");
    printf("  -h, --human-readable  print sizes in human readable format (e.g., 1K 234M 2G)\n");
    printf("  -k                    like --block-size=1K\n");
    printf("  -T, --print-type      print file system type\n");
    printf("      --help            display this help and exit\n");
    printf("      --version         output version information and exit\n");
}

// 버전 정보 출력
void print_version() {
    printf("df (custom implementation) 1.0\n");
    printf("Written for terminal implementation project.\n");
}

int main(int argc, char* argv[]) {
    int human_readable = 0;
    int show_all = 0;
    int show_type = 0;
    char* specific_path = NULL;
    
    // 명령행 인수 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--human-readable") == 0) {
            human_readable = 1;
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) {
            show_all = 1;
        } else if (strcmp(argv[i], "-k") == 0) {
            human_readable = 0; // 명시적으로 KB 단위
        } else if (strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--print-type") == 0) {
            show_type = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        } else if (strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (argv[i][0] != '-') {
            // 경로 인수
            specific_path = argv[i];
        } else {
            fprintf(stderr, "df: invalid option -- '%s'\n", argv[i]);
            fprintf(stderr, "Try 'df --help' for more information.\n");
            return 1;
        }
    }
    
    // 파일시스템 정보 출력
    print_filesystems(human_readable, show_all, specific_path);
    
    return 0;
}
```

## du: 파일/디렉토리의 디스크 사용량 출력
- -h: 사람이 읽기 쉬운 형식
- -s: 총 사용량만 출력

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>

// 옵션을 저장할 구조체
typedef struct {
    int human_readable;
    int summarize;
    int show_all;
    int max_depth;
    int current_depth;
    int show_apparent_size;
} DuOptions;

// 바이트를 사람이 읽기 쉬운 형태로 변환
void format_size(unsigned long long bytes, char* buffer, int human_readable) {
    if (human_readable) {
        if (bytes >= 1099511627776ULL) { // 1TB
            snprintf(buffer, 32, "%.1fT", (double)bytes / 1099511627776.0);
        } else if (bytes >= 1073741824ULL) { // 1GB
            snprintf(buffer, 32, "%.1fG", (double)bytes / 1073741824.0);
        } else if (bytes >= 1048576ULL) { // 1MB
            snprintf(buffer, 32, "%.1fM", (double)bytes / 1048576.0);
        } else if (bytes >= 1024ULL) { // 1KB
            snprintf(buffer, 32, "%.1fK", (double)bytes / 1024.0);
        } else {
            snprintf(buffer, 32, "%llu", bytes);
        }
    } else {
        // 1K 블록 단위로 출력 (기본값)
        snprintf(buffer, 32, "%llu", (bytes + 1023) / 1024);
    }
}

// 경로 결합 함수
int join_path(char* dest, size_t dest_size, const char* dir, const char* name) {
    int len = snprintf(dest, dest_size, "%s/%s", dir, name);
    if (len >= dest_size) {
        return -1; // 버퍼 오버플로우
    }
    return 0;
}

// 파일의 실제 디스크 사용량 계산
unsigned long long get_file_size(const char* path, const struct stat* st, int apparent_size) {
    if (apparent_size) {
        return st->st_size;
    } else {
        // 실제 디스크 블록 사용량 (512바이트 블록 단위)
        return st->st_blocks * 512;
    }
}

// 디렉토리 크기를 재귀적으로 계산
unsigned long long calculate_directory_size(const char* path, DuOptions* opts) {
    DIR* dir;
    struct dirent* entry;
    struct stat st;
    unsigned long long total_size = 0;
    char full_path[PATH_MAX];
    
    // 현재 디렉토리의 stat 정보 가져오기
    if (lstat(path, &st) != 0) {
        if (errno != ENOENT) {
            fprintf(stderr, "du: cannot access '%s': %s\n", path, strerror(errno));
        }
        return 0;
    }
    
    // 심볼릭 링크인 경우 링크 자체의 크기만 반환
    if (S_ISLNK(st.st_mode)) {
        return get_file_size(path, &st, opts->show_apparent_size);
    }
    
    // 일반 파일인 경우
    if (!S_ISDIR(st.st_mode)) {
        return get_file_size(path, &st, opts->show_apparent_size);
    }
    
    // 디렉토리 열기
    dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "du: cannot read directory '%s': %s\n", path, strerror(errno));
        return 0;
    }
    
    // 디렉토리 자체의 크기 추가
    total_size += get_file_size(path, &st, opts->show_apparent_size);
    
    // 디렉토리 내 항목들 처리
    while ((entry = readdir(dir)) != NULL) {
        // "."와 ".." 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 전체 경로 생성
        if (join_path(full_path, sizeof(full_path), path, entry->d_name) != 0) {
            continue;
        }
        
        // 파일 stat 정보 가져오기
        if (lstat(full_path, &st) != 0) {
            if (errno != ENOENT) {
                fprintf(stderr, "du: cannot access '%s': %s\n", full_path, strerror(errno));
            }
            continue;
        }
        
        unsigned long long item_size = 0;
        
        if (S_ISDIR(st.st_mode)) {
            // 하위 디렉토리 재귀 처리
            opts->current_depth++;
            item_size = calculate_directory_size(full_path, opts);
            opts->current_depth--;
            
            // 각 하위 디렉토리 크기 출력 (summarize 모드가 아닌 경우)
            if (!opts->summarize && (opts->max_depth == -1 || opts->current_depth < opts->max_depth)) {
                char size_str[32];
                format_size(item_size, size_str, opts->human_readable);
                printf("%s\t%s\n", size_str, full_path);
            }
        } else {
            // 일반 파일 크기 계산
            item_size = get_file_size(full_path, &st, opts->show_apparent_size);
            
            // 모든 파일 표시 옵션인 경우
            if (opts->show_all && !opts->summarize) {
                char size_str[32];
                format_size(item_size, size_str, opts->human_readable);
                printf("%s\t%s\n", size_str, full_path);
            }
        }
        
        total_size += item_size;
    }
    
    closedir(dir);
    return total_size;
}

// 도움말 출력
void print_help() {
    printf("Usage: du [OPTION]... [FILE]...\n");
    printf("Summarize disk usage of each FILE, recursively for directories.\n\n");
    printf("Options:\n");
    printf("  -a, --all             write counts for all files, not just directories\n");
    printf("  -h, --human-readable  print sizes in human readable format (e.g., 1K 234M 2G)\n");
    printf("  -s, --summarize       display only a total for each argument\n");
    printf("  -d, --max-depth=N     print the total for a directory (or file, with -a)\n");
    printf("                        only if it is N or fewer levels below the command\n");
    printf("  -c, --total           produce a grand total\n");
    printf("  -b, --bytes           equivalent to '--apparent-size --block-size=1'\n");
    printf("      --apparent-size   print apparent sizes, rather than disk usage\n");
    printf("      --help            display this help and exit\n");
    printf("      --version         output version information and exit\n");
}

// 버전 정보 출력
void print_version() {
    printf("du (custom implementation) 1.0\n");
    printf("Written for terminal implementation project.\n");
}

int main(int argc, char* argv[]) {
    DuOptions opts = {0, 0, 0, -1, 0, 0}; // 기본값 초기화
    int show_total = 0;
    char** paths = NULL;
    int path_count = 0;
    unsigned long long grand_total = 0;
    
    // 메모리 할당
    paths = malloc(argc * sizeof(char*));
    if (!paths) {
        fprintf(stderr, "du: memory allocation failed\n");
        return 1;
    }
    
    // 명령행 인수 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--human-readable") == 0) {
            opts.human_readable = 1;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--summarize") == 0) {
            opts.summarize = 1;
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) {
            opts.show_all = 1;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--total") == 0) {
            show_total = 1;
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bytes") == 0) {
            opts.show_apparent_size = 1;
            opts.human_readable = 0;
        } else if (strcmp(argv[i], "--apparent-size") == 0) {
            opts.show_apparent_size = 1;
        } else if (strncmp(argv[i], "-d", 2) == 0) {
            if (strlen(argv[i]) > 2) {
                opts.max_depth = atoi(argv[i] + 2);
            } else if (i + 1 < argc) {
                opts.max_depth = atoi(argv[++i]);
            }
        } else if (strncmp(argv[i], "--max-depth=", 12) == 0) {
            opts.max_depth = atoi(argv[i] + 12);
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            free(paths);
            return 0;
        } else if (strcmp(argv[i], "--version") == 0) {
            print_version();
            free(paths);
            return 0;
        } else if (argv[i][0] != '-') {
            // 경로 인수
            paths[path_count++] = argv[i];
        } else {
            fprintf(stderr, "du: invalid option -- '%s'\n", argv[i]);
            fprintf(stderr, "Try 'du --help' for more information.\n");
            free(paths);
            return 1;
        }
    }
    
    // 경로가 지정되지 않은 경우 현재 디렉토리 사용
    if (path_count == 0) {
        paths[0] = ".";
        path_count = 1;
    }
    
    // 각 경로에 대해 크기 계산
    for (int i = 0; i < path_count; i++) {
        opts.current_depth = 0;
        unsigned long long total_size = calculate_directory_size(paths[i], &opts);
        
        // 결과 출력
        char size_str[32];
        format_size(total_size, size_str, opts.human_readable);
        printf("%s\t%s\n", size_str, paths[i]);
        
        grand_total += total_size;
    }
    
    // 총합 출력 (여러 경로가 있거나 -c 옵션인 경우)
    if (show_total && (path_count > 1 || show_total)) {
        char total_str[32];
        format_size(grand_total, total_str, opts.human_readable);
        printf("%s\ttotal\n", total_str);
    }
    
    free(paths);
    return 0;
}
```

## uname: 시스템 정보 출력
- -a: 모든 시스템 정보

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

// 시스템 정보 옵션 플래그
typedef struct {
    int show_sysname;    // -s: 시스템 이름 (기본값)
    int show_nodename;   // -n: 네트워크 노드 호스트명
    int show_release;    // -r: 운영체제 릴리스
    int show_version;    // -v: 운영체제 버전
    int show_machine;    // -m: 머신 하드웨어 이름
    int show_processor;  // -p: 프로세서 타입
    int show_hwplatform; // -i: 하드웨어 플랫폼
    int show_opsystem;   // -o: 운영체제
    int show_all;        // -a: 모든 정보
} UnameOptions;

// 프로세서 정보를 /proc/cpuinfo에서 가져오기
int get_processor_info(char* processor, size_t size) {
    FILE* file = fopen("/proc/cpuinfo", "r");
    if (!file) {
        strncpy(processor, "unknown", size - 1);
        processor[size - 1] = '\0';
        return -1;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "model name", 10) == 0) {
            char* colon = strchr(line, ':');
            if (colon) {
                colon++; // ':' 다음으로 이동
                while (*colon == ' ' || *colon == '\t') colon++; // 공백 제거
                
                // 개행 문자 제거
                char* newline = strchr(colon, '\n');
                if (newline) *newline = '\0';
                
                strncpy(processor, colon, size - 1);
                processor[size - 1] = '\0';
                fclose(file);
                return 0;
            }
        }
    }
    
    // model name이 없는 경우 vendor_id 시도
    rewind(file);
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "vendor_id", 9) == 0) {
            char* colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                
                char* newline = strchr(colon, '\n');
                if (newline) *newline = '\0';
                
                strncpy(processor, colon, size - 1);
                processor[size - 1] = '\0';
                fclose(file);
                return 0;
            }
        }
    }
    
    fclose(file);
    strncpy(processor, "unknown", size - 1);
    processor[size - 1] = '\0';
    return -1;
}

// 하드웨어 플랫폼 정보 가져오기
void get_hardware_platform(char* platform, size_t size, const char* machine) {
    // 일반적인 아키텍처 매핑
    if (strstr(machine, "x86_64") || strstr(machine, "amd64")) {
        strncpy(platform, "x86_64", size - 1);
    } else if (strstr(machine, "i386") || strstr(machine, "i686")) {
        strncpy(platform, "i386", size - 1);
    } else if (strstr(machine, "arm") || strstr(machine, "aarch64")) {
        strncpy(platform, "arm", size - 1);
    } else if (strstr(machine, "mips")) {
        strncpy(platform, "mips", size - 1);
    } else {
        strncpy(platform, machine, size - 1);
    }
    platform[size - 1] = '\0';
}

// 시스템 정보 출력
void print_system_info(const UnameOptions* opts) {
    struct utsname sys_info;
    char processor[256] = "unknown";
    char hardware_platform[256];
    int first_output = 1;
    
    // uname 시스템 콜로 기본 정보 가져오기
    if (uname(&sys_info) != 0) {
        perror("uname");
        return;
    }
    
    // 추가 정보 수집
    get_processor_info(processor, sizeof(processor));
    get_hardware_platform(hardware_platform, sizeof(hardware_platform), sys_info.machine);
    
    // 시스템 이름 (기본값 또는 -s 옵션)
    if (opts->show_sysname || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", sys_info.sysname);
        first_output = 0;
    }
    
    // 네트워크 노드 호스트명 (-n 옵션)
    if (opts->show_nodename || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", sys_info.nodename);
        first_output = 0;
    }
    
    // 운영체제 릴리스 (-r 옵션)
    if (opts->show_release || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", sys_info.release);
        first_output = 0;
    }
    
    // 운영체제 버전 (-v 옵션)
    if (opts->show_version || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", sys_info.version);
        first_output = 0;
    }
    
    // 머신 하드웨어 이름 (-m 옵션)
    if (opts->show_machine || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", sys_info.machine);
        first_output = 0;
    }
    
    // 프로세서 타입 (-p 옵션)
    if (opts->show_processor || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", processor);
        first_output = 0;
    }
    
    // 하드웨어 플랫폼 (-i 옵션)
    if (opts->show_hwplatform || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", hardware_platform);
        first_output = 0;
    }
    
    // 운영체제 (-o 옵션)
    if (opts->show_opsystem || opts->show_all) {
        if (!first_output) printf(" ");
        printf("GNU/Linux");
        first_output = 0;
    }
    
    printf("\n");
}

// 도움말 출력
void print_help() {
    printf("Usage: uname [OPTION]...\n");
    printf("Print certain system information. With no OPTION, same as -s.\n\n");
    printf("Options:\n");
    printf("  -a, --all                print all information, in the following order,\n");
    printf("                           except omit -p and -i if unknown:\n");
    printf("  -s, --kernel-name        print the kernel name\n");
    printf("  -n, --nodename           print the network node hostname\n");
    printf("  -r, --kernel-release     print the kernel release\n");
    printf("  -v, --kernel-version     print the kernel version\n");
    printf("  -m, --machine            print the machine hardware name\n");
    printf("  -p, --processor          print the processor type (non-portable)\n");
    printf("  -i, --hardware-platform  print the hardware platform (non-portable)\n");
    printf("  -o, --operating-system   print the operating system\n");
    printf("      --help               display this help and exit\n");
    printf("      --version            output version information and exit\n");
}

// 버전 정보 출력
void print_version() {
    printf("uname (custom implementation) 1.0\n");
    printf("Written for terminal implementation project.\n");
}

int main(int argc, char* argv[]) {
    UnameOptions opts = {0}; // 모든 플래그를 0으로 초기화
    int any_option_set = 0;
    
    // 명령행 인수 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) {
            opts.show_all = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--kernel-name") == 0) {
            opts.show_sysname = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--nodename") == 0) {
            opts.show_nodename = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--kernel-release") == 0) {
            opts.show_release = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--kernel-version") == 0) {
            opts.show_version = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--machine") == 0) {
            opts.show_machine = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--processor") == 0) {
            opts.show_processor = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--hardware-platform") == 0) {
            opts.show_hwplatform = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--operating-system") == 0) {
            opts.show_opsystem = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        } else if (strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else {
            fprintf(stderr, "uname: invalid option -- '%s'\n", argv[i]);
            fprintf(stderr, "Try 'uname --help' for more information.\n");
            return 1;
        }
    }
    
    // 옵션이 지정되지 않은 경우 기본값은 -s (시스템 이름)
    if (!any_option_set) {
        opts.show_sysname = 1;
    }
    
    // 시스템 정보 출력
    print_system_info(&opts);
    
    return 0;
}
```

## date: 현재 날짜 및 시간 출력
- -s STRING: 날짜/시간 설정 (관리자 권한)
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pwd.h>
#endif

// 관리자 권한 확인 함수
int is_admin() {
#ifdef _WIN32
    // Windows에서 관리자 권한 확인
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
#else
    // Unix/Linux에서 root 권한 확인
    return (getuid() == 0);
#endif
}

// 날짜 문자열 파싱 함수
int parse_date_string(const char* date_str, struct tm* tm_info) {
    memset(tm_info, 0, sizeof(struct tm));
    
    // 다양한 날짜 형식 지원
    // 형식 1: "YYYY-MM-DD HH:MM:SS"
    if (sscanf(date_str, "%d-%d-%d %d:%d:%d", 
               &tm_info->tm_year, &tm_info->tm_mon, &tm_info->tm_mday,
               &tm_info->tm_hour, &tm_info->tm_min, &tm_info->tm_sec) == 6) {
        tm_info->tm_year -= 1900;  // tm_year은 1900년부터의 년수
        tm_info->tm_mon -= 1;      // tm_mon은 0부터 시작 (0=1월)
        return 1;
    }
    
    // 형식 2: "MM/DD/YYYY HH:MM:SS"
    if (sscanf(date_str, "%d/%d/%d %d:%d:%d",
               &tm_info->tm_mon, &tm_info->tm_mday, &tm_info->tm_year,
               &tm_info->tm_hour, &tm_info->tm_min, &tm_info->tm_sec) == 6) {
        tm_info->tm_year -= 1900;
        tm_info->tm_mon -= 1;
        return 1;
    }
    
    // 형식 3: "YYYY-MM-DD" (시간은 00:00:00으로 설정)
    if (sscanf(date_str, "%d-%d-%d", 
               &tm_info->tm_year, &tm_info->tm_mon, &tm_info->tm_mday) == 3) {
        tm_info->tm_year -= 1900;
        tm_info->tm_mon -= 1;
        return 1;
    }
    
    // 형식 4: "HH:MM:SS" (오늘 날짜에 시간만 변경)
    if (sscanf(date_str, "%d:%d:%d", 
               &tm_info->tm_hour, &tm_info->tm_min, &tm_info->tm_sec) == 3) {
        time_t now = time(NULL);
        struct tm* current = localtime(&now);
        tm_info->tm_year = current->tm_year;
        tm_info->tm_mon = current->tm_mon;
        tm_info->tm_mday = current->tm_mday;
        return 1;
    }
    
    return 0; // 파싱 실패
}

// 시스템 시간 설정 함수
int set_system_time(struct tm* tm_info) {
#ifdef _WIN32
    SYSTEMTIME st = {0};
    st.wYear = tm_info->tm_year + 1900;
    st.wMonth = tm_info->tm_mon + 1;
    st.wDay = tm_info->tm_mday;
    st.wHour = tm_info->tm_hour;
    st.wMinute = tm_info->tm_min;
    st.wSecond = tm_info->tm_sec;
    
    return SetSystemTime(&st) ? 0 : -1;
#else
    struct timeval tv;
    time_t new_time = mktime(tm_info);
    
    if (new_time == -1) {
        return -1;
    }
    
    tv.tv_sec = new_time;
    tv.tv_usec = 0;
    
    return settimeofday(&tv, NULL);
#endif
}

void print_usage() {
    printf("사용법: date [옵션]\n");
    printf("옵션:\n");
    printf("  (옵션 없음)    현재 날짜 및 시간 출력\n");
    printf("  -s STRING      날짜/시간 설정 (관리자 권한 필요)\n");
    printf("\n");
    printf("날짜 형식 예시:\n");
    printf("  \"2024-12-25 15:30:00\"  (YYYY-MM-DD HH:MM:SS)\n");
    printf("  \"12/25/2024 15:30:00\"  (MM/DD/YYYY HH:MM:SS)\n");
    printf("  \"2024-12-25\"           (YYYY-MM-DD, 시간은 00:00:00)\n");
    printf("  \"15:30:00\"             (HH:MM:SS, 오늘 날짜)\n");
}

int main(int argc, char* argv[]) {
    // 인수가 없으면 현재 시간 출력
    if (argc == 1) {
        time_t current_time = time(NULL);
        char* time_str = ctime(&current_time);
        
        // ctime()의 결과에서 개행문자 제거
        if (time_str) {
            size_t len = strlen(time_str);
            if (len > 0 && time_str[len-1] == '\n') {
                time_str[len-1] = '\0';
            }
            printf("%s\n", time_str);
        }
        return 0;
    }
    
    // -s 옵션 처리
    if (argc >= 3 && strcmp(argv[1], "-s") == 0) {
        // 관리자 권한 확인
        if (!is_admin()) {
            fprintf(stderr, "오류: 시간 설정을 위해서는 관리자 권한이 필요합니다.\n");
#ifdef _WIN32
            fprintf(stderr, "관리자 권한으로 프로그램을 실행해주세요.\n");
#else
            fprintf(stderr, "sudo를 사용하여 실행해주세요.\n");
#endif
            return 1;
        }
        
        const char* date_string = argv[2];
        struct tm tm_info;
        
        // 날짜 문자열 파싱
        if (!parse_date_string(date_string, &tm_info)) {
            fprintf(stderr, "오류: 잘못된 날짜 형식입니다.\n");
            print_usage();
            return 1;
        }
        
        // 파싱된 날짜 정보 검증
        if (tm_info.tm_year < 70 || tm_info.tm_year > 200 ||  // 1970~2100년 범위
            tm_info.tm_mon < 0 || tm_info.tm_mon > 11 ||       // 0~11월
            tm_info.tm_mday < 1 || tm_info.tm_mday > 31 ||     // 1~31일
            tm_info.tm_hour < 0 || tm_info.tm_hour > 23 ||     // 0~23시
            tm_info.tm_min < 0 || tm_info.tm_min > 59 ||       // 0~59분
            tm_info.tm_sec < 0 || tm_info.tm_sec > 59) {       // 0~59초
            fprintf(stderr, "오류: 날짜/시간 값이 유효하지 않습니다.\n");
            return 1;
        }
        
        // 시스템 시간 설정
        if (set_system_time(&tm_info) != 0) {
            perror("시간 설정 실패");
            return 1;
        }
        
        // 설정된 시간 출력
        char buffer[100];
        strftime(buffer, sizeof(buffer), "%Y년 %m월 %d일 %H:%M:%S", &tm_info);
        printf("시간이 설정되었습니다: %s\n", buffer);
        
        return 0;
    }
    
    // 잘못된 옵션
    fprintf(stderr, "오류: 알 수 없는 옵션입니다.\n");
    print_usage();
    return 1;
}
```

## time: 명령어 실행 시간 측정 
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <sys/resource.h>
#endif

// 시간 차이를 계산하는 함수 (마이크로초 단위)
long long time_diff_microseconds(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000000LL + (end.tv_usec - start.tv_usec);
}

// 시간을 사람이 읽기 쉬운 형태로 포맷팅
void format_time(double seconds, char* buffer, size_t buffer_size) {
    if (seconds < 0.001) {
        snprintf(buffer, buffer_size, "%.3fms", seconds * 1000);
    } else if (seconds < 1.0) {
        snprintf(buffer, buffer_size, "%.3fs", seconds);
    } else if (seconds < 60.0) {
        snprintf(buffer, buffer_size, "%.3fs", seconds);
    } else {
        int minutes = (int)(seconds / 60);
        double remaining_seconds = seconds - (minutes * 60);
        snprintf(buffer, buffer_size, "%dm%.3fs", minutes, remaining_seconds);
    }
}

#ifdef _WIN32
// Windows용 명령어 실행 함수
int execute_command_windows(char* argv[], struct timeval* real_start, struct timeval* real_end,
                           double* user_time, double* sys_time) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    FILETIME creation_time, exit_time, kernel_time, user_time_ft;
    SYSTEMTIME st;
    
    // 명령어 문자열 생성
    char command_line[8192] = "";
    for (int i = 1; argv[i] != NULL; i++) {
        if (i > 1) strcat(command_line, " ");
        strcat(command_line, argv[i]);
    }
    
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));
    
    // 실행 시간 측정 시작
    gettimeofday(real_start, NULL);
    
    // 프로세스 생성
    if (!CreateProcess(NULL, command_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "time: 명령어 실행 실패: %s\n", argv[1]);
        return -1;
    }
    
    // 프로세스 완료 대기
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    // 실행 시간 측정 종료
    gettimeofday(real_end, NULL);
    
    // CPU 시간 정보 가져오기
    if (GetProcessTimes(pi.hProcess, &creation_time, &exit_time, &kernel_time, &user_time_ft)) {
        ULARGE_INTEGER user_time_ul, kernel_time_ul;
        user_time_ul.LowPart = user_time_ft.dwLowDateTime;
        user_time_ul.HighPart = user_time_ft.dwHighDateTime;
        kernel_time_ul.LowPart = kernel_time.dwLowDateTime;
        kernel_time_ul.HighPart = kernel_time.dwHighDateTime;
        
        *user_time = user_time_ul.QuadPart / 10000000.0;  // 100ns 단위를 초로 변환
        *sys_time = kernel_time_ul.QuadPart / 10000000.0;
    }
    
    // 종료 코드 가져오기
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    
    // 핸들 정리
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return (int)exit_code;
}
#endif

// Unix/Linux용 명령어 실행 함수
int execute_command_unix(char* argv[], struct timeval* real_start, struct timeval* real_end,
                        double* user_time, double* sys_time) {
    pid_t pid;
    int status;
    struct rusage usage;
    
    // 실행 시간 측정 시작
    gettimeofday(real_start, NULL);
    
    pid = fork();
    if (pid == 0) {
        // 자식 프로세스: 명령어 실행
        execvp(argv[1], &argv[1]);
        // execvp 실패 시
        fprintf(stderr, "time: %s: %s\n", argv[1], strerror(errno));
        exit(127);
    } else if (pid > 0) {
        // 부모 프로세스: 자식 프로세스 대기
        if (wait4(pid, &status, 0, &usage) == -1) {
            perror("time: wait4 실패");
            return -1;
        }
        
        // 실행 시간 측정 종료
        gettimeofday(real_end, NULL);
        
        // CPU 시간 정보 가져오기
        *user_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0;
        *sys_time = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0;
        
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "time: 명령어가 신호 %d로 종료됨\n", WTERMSIG(status));
            return 128 + WTERMSIG(status);
        }
    } else {
        // fork 실패
        perror("time: fork 실패");
        return -1;
    }
    
    return 0;
}

void print_usage() {
    printf("사용법: time 명령어 [인수...]\n");
    printf("\n");
    printf("명령어의 실행 시간을 측정합니다.\n");
    printf("\n");
    printf("출력 정보:\n");
    printf("  real    실제 경과 시간 (wall clock time)\n");
    printf("  user    사용자 모드에서 소비된 CPU 시간\n");
    printf("  sys     시스템 모드에서 소비된 CPU 시간\n");
    printf("\n");
    printf("예시:\n");
    printf("  time ls -l\n");
    printf("  time sleep 2\n");
    printf("  time gcc -o test test.c\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "time: 인수가 부족합니다\n");
        print_usage();
        return 1;
    }
    
    // 도움말 옵션 처리
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage();
        return 0;
    }
    
    struct timeval real_start, real_end;
    double user_time = 0.0, sys_time = 0.0;
    int exit_code;
    
    // 플랫폼별 명령어 실행
#ifdef _WIN32
    exit_code = execute_command_windows(argv, &real_start, &real_end, &user_time, &sys_time);
#else
    exit_code = execute_command_unix(argv, &real_start, &real_end, &user_time, &sys_time);
#endif
    
    if (exit_code == -1) {
        return 1;
    }
    
    // 실제 경과 시간 계산
    long long real_microseconds = time_diff_microseconds(real_start, real_end);
    double real_time = real_microseconds / 1000000.0;
    
    // 시간 정보를 포맷팅하여 출력
    char real_str[64], user_str[64], sys_str[64];
    format_time(real_time, real_str, sizeof(real_str));
    format_time(user_time, user_str, sizeof(user_str));
    format_time(sys_time, sys_str, sizeof(sys_str));
    
    // stderr로 시간 정보 출력 (실제 time 명령어와 동일)
    fprintf(stderr, "\nreal\t%s\n", real_str);
    fprintf(stderr, "user\t%s\n", user_str);
    fprintf(stderr, "sys\t%s\n", sys_str);
    
    // 추가 통계 정보 출력 (Unix 계열에서만)
#ifndef _WIN32
    struct rusage usage;
    if (getrusage(RUSAGE_CHILDREN, &usage) == 0) {
        if (usage.ru_maxrss > 0) {
            fprintf(stderr, "최대 메모리 사용량: %ldKB\n", usage.ru_maxrss);
        }
        if (usage.ru_minflt > 0 || usage.ru_majflt > 0) {
            fprintf(stderr, "페이지 폴트: %ld (minor), %ld (major)\n", 
                   usage.ru_minflt, usage.ru_majflt);
        }
        if (usage.ru_nvcsw > 0 || usage.ru_nivcsw > 0) {
            fprintf(stderr, "컨텍스트 스위치: %ld (voluntary), %ld (involuntary)\n",
                   usage.ru_nvcsw, usage.ru_nivcsw);
        }
    }
#endif
    
    return exit_code;
}
```

## III. 사용자 및 그룹 관리 (User and Group Management)


## whoami: 현재 로그인된 사용자 이름 출력

```
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
```

## id: 사용자 및 그룹 ID 정보 출력

```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <string.h>

// 기본 id 명령어 구현 (uid, gid, groups 모두 출력)
int cmd_id() {
    uid_t uid = getuid();        // 실제 사용자 ID
    uid_t euid = geteuid();      // 유효 사용자 ID
    gid_t gid = getgid();        // 실제 그룹 ID
    gid_t egid = getegid();      // 유효 그룹 ID
    
    struct passwd *pw = getpwuid(uid);
    struct group *gr = getgrgid(gid);
    
    // uid 출력
    printf("uid=%d", uid);
    if (pw != NULL) {
        printf("(%s)", pw->pw_name);
    }
    
    // gid 출력
    printf(" gid=%d", gid);
    if (gr != NULL) {
        printf("(%s)", gr->gr_name);
    }
    
    // euid가 uid와 다르면 출력
    if (euid != uid) {
        struct passwd *epw = getpwuid(euid);
        printf(" euid=%d", euid);
        if (epw != NULL) {
            printf("(%s)", epw->pw_name);
        }
    }
    
    // egid가 gid와 다르면 출력
    if (egid != gid) {
        struct group *egr = getgrgid(egid);
        printf(" egid=%d", egid);
        if (egr != NULL) {
            printf("(%s)", egr->gr_name);
        }
    }
    
    // 보조 그룹들 출력
    int ngroups = getgroups(0, NULL);  // 그룹 개수 확인
    if (ngroups > 0) {
        gid_t *groups = malloc(ngroups * sizeof(gid_t));
        if (groups != NULL) {
            if (getgroups(ngroups, groups) != -1) {
                printf(" groups=");
                for (int i = 0; i < ngroups; i++) {
                    if (i > 0) printf(",");
                    printf("%d", groups[i]);
                    
                    struct group *g = getgrgid(groups[i]);
                    if (g != NULL) {
                        printf("(%s)", g->gr_name);
                    }
                }
            }
            free(groups);
        }
    }
    
    printf("\n");
    return 0;
}

// -u 옵션: 사용자 ID만 출력
int cmd_id_user() {
    printf("%d\n", getuid());
    return 0;
}

// -g 옵션: 그룹 ID만 출력
int cmd_id_group() {
    printf("%d\n", getgid());
    return 0;
}

// -un 옵션: 사용자 이름 출력
int cmd_id_user_name() {
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    
    if (pw == NULL) {
        printf("%d\n", uid);  // 이름을 찾을 수 없으면 숫자로 출력
    } else {
        printf("%s\n", pw->pw_name);
    }
    return 0;
}

// -gn 옵션: 그룹 이름 출력
int cmd_id_group_name() {
    gid_t gid = getgid();
    struct group *gr = getgrgid(gid);
    
    if (gr == NULL) {
        printf("%d\n", gid);  // 이름을 찾을 수 없으면 숫자로 출력
    } else {
        printf("%s\n", gr->gr_name);
    }
    return 0;
}

// -G 옵션: 모든 그룹 ID 출력
int cmd_id_all_groups() {
    int ngroups = getgroups(0, NULL);
    if (ngroups > 0) {
        gid_t *groups = malloc(ngroups * sizeof(gid_t));
        if (groups != NULL) {
            if (getgroups(ngroups, groups) != -1) {
                for (int i = 0; i < ngroups; i++) {
                    if (i > 0) printf(" ");
                    printf("%d", groups[i]);
                }
                printf("\n");
            }
            free(groups);
        }
    }
    return 0;
}

// 터미널에서 사용할 메인 함수
int execute_id(char **args) {
    if (args[1] == NULL) {
        // 옵션이 없으면 기본 출력
        return cmd_id();
    }
    
    // 옵션 파싱
    if (strcmp(args[1], "-u") == 0) {
        return cmd_id_user();
    } else if (strcmp(args[1], "-g") == 0) {
        return cmd_id_group();
    } else if (strcmp(args[1], "-un") == 0) {
        return cmd_id_user_name();
    } else if (strcmp(args[1], "-gn") == 0) {
        return cmd_id_group_name();
    } else if (strcmp(args[1], "-G") == 0) {
        return cmd_id_all_groups();
    } else {
        fprintf(stderr, "id: invalid option '%s'\n", args[1]);
        fprintf(stderr, "Usage: id [-u|-g|-G|-un|-gn]\n");
        return 1;
    }
}

// 테스트용 main 함수
int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Basic id output:\n");
        cmd_id();
        
        printf("\nUser ID: ");
        cmd_id_user();
        
        printf("Group ID: ");
        cmd_id_group();
        
        printf("User name: ");
        cmd_id_user_name();
        
        printf("Group name: ");
        cmd_id_group_name();
        
        printf("All groups: ");
        cmd_id_all_groups();
    } else {
        return execute_id(argv);
    }
    
    return 0;
}
```

## passwd: 사용자 비밀번호 변경 (자신 또는 다른 사용자)

```
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
```

## adduser: 새 사용자 추가 (관리자 권한)

```
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
```

## deluser: 사용자 삭제 (관리자 권한)

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

// 사용법 출력 함수
void print_usage(const char* program_name) {
    printf("사용법: %s [옵션] 사용자명\n", program_name);
    printf("옵션:\n");
    printf("  -r, --remove-home    홈 디렉터리도 함께 삭제\n");
    printf("  -f, --force          강제 삭제 (확인 없이)\n");
    printf("  -h, --help           도움말 출력\n");
    printf("\n예시:\n");
    printf("  %s testuser          # testuser 삭제\n", program_name);
    printf("  %s -r testuser       # testuser와 홈 디렉터리 삭제\n", program_name);
}

// 관리자 권한 확인 함수
int check_admin_privileges() {
    if (getuid() != 0) {
        fprintf(stderr, "오류: 이 명령어는 관리자 권한이 필요합니다.\n");
        fprintf(stderr, "sudo를 사용하여 실행하세요.\n");
        return 0;
    }
    return 1;
}

// 사용자 존재 확인 함수
int check_user_exists(const char* username) {
    struct passwd* pwd = getpwnam(username);
    return (pwd != NULL);
}

// 디렉터리 재귀적 삭제 함수
int remove_directory_recursive(const char* path) {
    DIR* dir;
    struct dirent* entry;
    struct stat statbuf;
    char full_path[1024];
    
    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        // . 과 .. 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (lstat(full_path, &statbuf) == -1) {
            perror("lstat");
            continue;
        }
        
        if (S_ISDIR(statbuf.st_mode)) {
            // 디렉터리인 경우 재귀적으로 삭제
            remove_directory_recursive(full_path);
        } else {
            // 파일인 경우 삭제
            if (unlink(full_path) == -1) {
                fprintf(stderr, "파일 삭제 실패: %s (%s)\n", full_path, strerror(errno));
            } else {
                printf("파일 삭제됨: %s\n", full_path);
            }
        }
    }
    
    closedir(dir);
    
    // 빈 디렉터리 삭제
    if (rmdir(path) == -1) {
        fprintf(stderr, "디렉터리 삭제 실패: %s (%s)\n", path, strerror(errno));
        return -1;
    } else {
        printf("디렉터리 삭제됨: %s\n", path);
    }
    
    return 0;
}

// 사용자 확인 함수
int confirm_deletion(const char* username, int remove_home) {
    char response[10];
    
    printf("사용자 '%s'를 삭제하시겠습니까?", username);
    if (remove_home) {
        printf(" (홈 디렉터리 포함)");
    }
    printf(" [y/N]: ");
    
    if (fgets(response, sizeof(response), stdin) == NULL) {
        return 0;
    }
    
    return (response[0] == 'y' || response[0] == 'Y');
}

// 시스템 사용자 삭제 함수 (userdel 명령어 사용)
int delete_system_user(const char* username, int remove_home) {
    char command[256];
    int result;
    
    if (remove_home) {
        snprintf(command, sizeof(command), "userdel -r %s", username);
    } else {
        snprintf(command, sizeof(command), "userdel %s", username);
    }
    
    printf("시스템 명령어 실행: %s\n", command);
    result = system(command);
    
    if (result == 0) {
        printf("사용자 '%s'가 성공적으로 삭제되었습니다.\n", username);
        return 1;
    } else {
        fprintf(stderr, "사용자 삭제 중 오류가 발생했습니다.\n");
        return 0;
    }
}

// 수동 사용자 삭제 함수 (passwd, shadow 파일 수정)
int delete_user_manual(const char* username, int remove_home) {
    struct passwd* pwd;
    char temp_file[] = "/tmp/passwd_temp_XXXXXX";
    char temp_shadow[] = "/tmp/shadow_temp_XXXXXX";
    FILE* passwd_file, *shadow_file;
    FILE* temp_passwd, *temp_shadow;
    char line[1024];
    char* user_field;
    int found = 0;
    
    // 사용자 정보 가져오기
    pwd = getpwnam(username);
    if (pwd == NULL) {
        fprintf(stderr, "사용자 '%s'를 찾을 수 없습니다.\n", username);
        return 0;
    }
    
    // 임시 파일 생성
    int temp_fd = mkstemp(temp_file);
    if (temp_fd == -1) {
        perror("임시 파일 생성 실패");
        return 0;
    }
    close(temp_fd);
    
    int temp_shadow_fd = mkstemp(temp_shadow);
    if (temp_shadow_fd == -1) {
        perror("임시 shadow 파일 생성 실패");
        unlink(temp_file);
        return 0;
    }
    close(temp_shadow_fd);
    
    // /etc/passwd 파일 수정
    passwd_file = fopen("/etc/passwd", "r");
    temp_passwd = fopen(temp_file, "w");
    
    if (passwd_file == NULL || temp_passwd == NULL) {
        perror("passwd 파일 열기 실패");
        if (passwd_file) fclose(passwd_file);
        if (temp_passwd) fclose(temp_passwd);
        unlink(temp_file);
        unlink(temp_shadow);
        return 0;
    }
    
    while (fgets(line, sizeof(line), passwd_file)) {
        user_field = strtok(line, ":");
        if (user_field != NULL && strcmp(user_field, username) == 0) {
            found = 1;
            printf("passwd에서 사용자 제거: %s\n", username);
        } else {
            fputs(line, temp_passwd);
            // strtok로 인해 변경된 줄을 복원하기 위해 원본 다시 읽기
            fseek(passwd_file, ftell(passwd_file) - strlen(line), SEEK_SET);
            fgets(line, sizeof(line), passwd_file);
            fputs(line, temp_passwd);
        }
    }
    
    fclose(passwd_file);
    fclose(temp_passwd);
    
    if (!found) {
        fprintf(stderr, "passwd 파일에서 사용자를 찾을 수 없습니다.\n");
        unlink(temp_file);
        unlink(temp_shadow);
        return 0;
    }
    
    // /etc/shadow 파일 수정
    shadow_file = fopen("/etc/shadow", "r");
    temp_shadow = fopen(temp_shadow, "w");
    
    if (shadow_file != NULL && temp_shadow != NULL) {
        while (fgets(line, sizeof(line), shadow_file)) {
            user_field = strtok(line, ":");
            if (user_field == NULL || strcmp(user_field, username) != 0) {
                // strtok로 인해 변경된 줄을 복원
                fseek(shadow_file, ftell(shadow_file) - strlen(line), SEEK_SET);
                fgets(line, sizeof(line), shadow_file);
                fputs(line, temp_shadow);
            } else {
                printf("shadow에서 사용자 제거: %s\n", username);
            }
        }
        fclose(shadow_file);
        fclose(temp_shadow);
        
        // shadow 파일 교체
        if (rename(temp_shadow, "/etc/shadow") != 0) {
            perror("shadow 파일 교체 실패");
        }
    } else {
        if (shadow_file) fclose(shadow_file);
        if (temp_shadow) fclose(temp_shadow);
        unlink(temp_shadow);
    }
    
    // passwd 파일 교체
    if (rename(temp_file, "/etc/passwd") != 0) {
        perror("passwd 파일 교체 실패");
        unlink(temp_file);
        return 0;
    }
    
    // 홈 디렉터리 삭제
    if (remove_home && pwd->pw_dir != NULL) {
        printf("홈 디렉터리 삭제 중: %s\n", pwd->pw_dir);
        if (remove_directory_recursive(pwd->pw_dir) == 0) {
            printf("홈 디렉터리가 삭제되었습니다.\n");
        }
    }
    
    return 1;
}

// deluser 명령어 구현
int deluser_command(int argc, char* argv[]) {
    int remove_home = 0;
    int force = 0;
    char* username = NULL;
    
    // 인자 파싱
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--remove-home") == 0) {
            remove_home = 1;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            force = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            username = argv[i];
        } else {
            fprintf(stderr, "알 수 없는 옵션: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // 사용자명 확인
    if (username == NULL) {
        fprintf(stderr, "오류: 사용자명을 입력해주세요.\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // 관리자 권한 확인
    if (!check_admin_privileges()) {
        return 1;
    }
    
    // 사용자 존재 확인
    if (!check_user_exists(username)) {
        fprintf(stderr, "오류: 사용자 '%s'가 존재하지 않습니다.\n", username);
        return 1;
    }
    
    // root 사용자 삭제 방지
    if (strcmp(username, "root") == 0) {
        fprintf(stderr, "오류: root 사용자는 삭제할 수 없습니다.\n");
        return 1;
    }
    
    // 확인 (force 옵션이 없는 경우)
    if (!force) {
        if (!confirm_deletion(username, remove_home)) {
            printf("사용자 삭제가 취소되었습니다.\n");
            return 0;
        }
    }
    
    // 사용자 삭제 시도 (시스템 명령어 우선)
    if (delete_system_user(username, remove_home)) {
        return 0;
    }
    
    // 시스템 명령어 실패 시 수동 삭제 시도
    printf("시스템 명령어 실패, 수동 삭제를 시도합니다...\n");
    if (delete_user_manual(username, remove_home)) {
        printf("사용자 '%s'가 성공적으로 삭제되었습니다.\n", username);
        return 0;
    }
    
    fprintf(stderr, "사용자 삭제에 실패했습니다.\n");
    return 1;
}

int main(int argc, char* argv[]) {
    return deluser_command(argc, argv);
}
```

## addgroup: 새 그룹 추가 (관리자 권한) 

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>

// 사용법 출력 함수
void print_usage(const char* program_name) {
    printf("사용법: %s [옵션] 그룹명\n", program_name);
    printf("옵션:\n");
    printf("  -g, --gid GID        그룹 ID 지정\n");
    printf("  -r, --system         시스템 그룹으로 생성\n");
    printf("  -f, --force          그룹이 이미 존재해도 성공으로 처리\n");
    printf("  -h, --help           도움말 출력\n");
    printf("\n예시:\n");
    printf("  %s mygroup           # mygroup 그룹 생성\n", program_name);
    printf("  %s -g 1500 mygroup   # GID 1500으로 mygroup 생성\n", program_name);
    printf("  %s -r sysgroup       # 시스템 그룹 sysgroup 생성\n", program_name);
}

// 관리자 권한 확인 함수
int check_admin_privileges() {
    if (getuid() != 0) {
        fprintf(stderr, "오류: 이 명령어는 관리자 권한이 필요합니다.\n");
        fprintf(stderr, "sudo를 사용하여 실행하세요.\n");
        return 0;
    }
    return 1;
}

// 그룹 이름 유효성 검사 함수
int validate_groupname(const char* groupname) {
    int len = strlen(groupname);
    
    // 길이 체크 (최대 32자)
    if (len == 0 || len > 32) {
        fprintf(stderr, "오류: 그룹명은 1-32자 사이여야 합니다.\n");
        return 0;
    }
    
    // 첫 문자는 알파벳 또는 _
    if (!isalpha(groupname[0]) && groupname[0] != '_') {
        fprintf(stderr, "오류: 그룹명은 알파벳 또는 '_'로 시작해야 합니다.\n");
        return 0;
    }
    
    // 나머지 문자는 알파벳, 숫자, _, - 만 허용
    for (int i = 1; i < len; i++) {
        if (!isalnum(groupname[i]) && groupname[i] != '_' && groupname[i] != '-') {
            fprintf(stderr, "오류: 그룹명에는 알파벳, 숫자, '_', '-'만 사용할 수 있습니다.\n");
            return 0;
        }
    }
    
    // 예약된 이름 확인
    if (strcmp(groupname, "root") == 0 || 
        strcmp(groupname, "daemon") == 0 ||
        strcmp(groupname, "sys") == 0) {
        fprintf(stderr, "오류: '%s'는 예약된 그룹명입니다.\n", groupname);
        return 0;
    }
    
    return 1;
}

// 그룹 존재 확인 함수
int check_group_exists(const char* groupname) {
    struct group* grp = getgrnam(groupname);
    return (grp != NULL);
}

// GID 존재 확인 함수
int check_gid_exists(gid_t gid) {
    struct group* grp = getgrgid(gid);
    return (grp != NULL);
}

// 다음 사용 가능한 GID 찾기 함수
gid_t find_next_available_gid(int is_system) {
    gid_t start_gid, end_gid;
    
    if (is_system) {
        start_gid = 100;  // 시스템 그룹 시작 GID
        end_gid = 999;    // 시스템 그룹 끝 GID
    } else {
        start_gid = 1000; // 일반 그룹 시작 GID
        end_gid = 60000;  // 일반 그룹 끝 GID
    }
    
    for (gid_t gid = start_gid; gid <= end_gid; gid++) {
        if (!check_gid_exists(gid)) {
            return gid;
        }
    }
    
    return (gid_t)-1; // 사용 가능한 GID 없음
}

// /etc/group 파일에 그룹 추가 함수
int add_group_to_file(const char* groupname, gid_t gid) {
    FILE* group_file;
    char temp_file[] = "/tmp/group_temp_XXXXXX";
    FILE* temp_group;
    char line[1024];
    int temp_fd;
    int found_insert_pos = 0;
    
    // 임시 파일 생성
    temp_fd = mkstemp(temp_file);
    if (temp_fd == -1) {
        perror("임시 파일 생성 실패");
        return 0;
    }
    close(temp_fd);
    
    // /etc/group 파일 읽기
    group_file = fopen("/etc/group", "r");
    temp_group = fopen(temp_file, "w");
    
    if (group_file == NULL || temp_group == NULL) {
        perror("group 파일 열기 실패");
        if (group_file) fclose(group_file);
        if (temp_group) fclose(temp_group);
        unlink(temp_file);
        return 0;
    }
    
    // 기존 내용 복사하면서 적절한 위치에 새 그룹 삽입
    while (fgets(line, sizeof(line), group_file)) {
        char* line_copy = strdup(line);
        char* current_group = strtok(line_copy, ":");
        char* gid_str = strtok(NULL, ":");
        
        if (current_group && gid_str) {
            gid_t current_gid = atoi(gid_str);
            
            // GID 순서로 정렬하여 삽입
            if (!found_insert_pos && current_gid > gid) {
                fprintf(temp_group, "%s:x:%d:\n", groupname, gid);
                found_insert_pos = 1;
            }
        }
        
        fputs(line, temp_group);
        free(line_copy);
    }
    
    // 파일 끝에 도달했는데 아직 삽입하지 않은 경우
    if (!found_insert_pos) {
        fprintf(temp_group, "%s:x:%d:\n", groupname, gid);
    }
    
    fclose(group_file);
    fclose(temp_group);
    
    // 파일 권한 설정 (644)
    if (chmod(temp_file, 0644) != 0) {
        perror("파일 권한 설정 실패");
        unlink(temp_file);
        return 0;
    }
    
    // 원본 파일 교체
    if (rename(temp_file, "/etc/group") != 0) {
        perror("group 파일 교체 실패");
        unlink(temp_file);
        return 0;
    }
    
    return 1;
}

// /etc/gshadow 파일에 그룹 추가 함수
int add_group_to_gshadow(const char* groupname) {
    FILE* gshadow_file;
    char temp_file[] = "/tmp/gshadow_temp_XXXXXX";
    FILE* temp_gshadow;
    char line[1024];
    int temp_fd;
    
    // gshadow 파일이 없으면 건너뛰기
    if (access("/etc/gshadow", F_OK) != 0) {
        return 1; // gshadow 없어도 성공으로 처리
    }
    
    // 임시 파일 생성
    temp_fd = mkstemp(temp_file);
    if (temp_fd == -1) {
        perror("gshadow 임시 파일 생성 실패");
        return 0;
    }
    close(temp_fd);
    
    // /etc/gshadow 파일 처리
    gshadow_file = fopen("/etc/gshadow", "r");
    temp_gshadow = fopen(temp_file, "w");
    
    if (gshadow_file == NULL || temp_gshadow == NULL) {
        perror("gshadow 파일 열기 실패");
        if (gshadow_file) fclose(gshadow_file);
        if (temp_gshadow) fclose(temp_gshadow);
        unlink(temp_file);
        return 0;
    }
    
    // 기존 내용 복사
    while (fgets(line, sizeof(line), gshadow_file)) {
        fputs(line, temp_gshadow);
    }
    
    // 새 그룹 추가
    fprintf(temp_gshadow, "%s:!::\n", groupname);
    
    fclose(gshadow_file);
    fclose(temp_gshadow);
    
    // 파일 권한 설정 (640)
    if (chmod(temp_file, 0640) != 0) {
        perror("gshadow 파일 권한 설정 실패");
        unlink(temp_file);
        return 0;
    }
    
    // 원본 파일 교체
    if (rename(temp_file, "/etc/gshadow") != 0) {
        perror("gshadow 파일 교체 실패");
        unlink(temp_file);
        return 0;
    }
    
    return 1;
}

// 시스템 명령어를 사용한 그룹 추가 함수
int add_group_system_command(const char* groupname, gid_t gid, int is_system) {
    char command[256];
    int result;
    
    if (gid != (gid_t)-1) {
        if (is_system) {
            snprintf(command, sizeof(command), "groupadd -r -g %d %s", gid, groupname);
        } else {
            snprintf(command, sizeof(command), "groupadd -g %d %s", gid, groupname);
        }
    } else {
        if (is_system) {
            snprintf(command, sizeof(command), "groupadd -r %s", groupname);
        } else {
            snprintf(command, sizeof(command), "groupadd %s", groupname);
        }
    }
    
    printf("시스템 명령어 실행: %s\n", command);
    result = system(command);
    
    if (result == 0) {
        printf("그룹 '%s'가 성공적으로 생성되었습니다.\n", groupname);
        return 1;
    } else {
        fprintf(stderr, "그룹 생성 중 오류가 발생했습니다.\n");
        return 0;
    }
}

// 수동 그룹 추가 함수
int add_group_manual(const char* groupname, gid_t gid, int is_system) {
    gid_t final_gid;
    
    if (gid != (gid_t)-1) {
        // 지정된 GID 사용
        if (check_gid_exists(gid)) {
            fprintf(stderr, "오류: GID %d는 이미 사용 중입니다.\n", gid);
            return 0;
        }
        final_gid = gid;
    } else {
        // 자동으로 GID 할당
        final_gid = find_next_available_gid(is_system);
        if (final_gid == (gid_t)-1) {
            fprintf(stderr, "오류: 사용 가능한 GID를 찾을 수 없습니다.\n");
            return 0;
        }
    }
    
    printf("그룹 '%s'를 GID %d로 생성합니다.\n", groupname, final_gid);
    
    // /etc/group에 추가
    if (!add_group_to_file(groupname, final_gid)) {
        return 0;
    }
    
    // /etc/gshadow에 추가 (선택적)
    if (!add_group_to_gshadow(groupname)) {
        fprintf(stderr, "경고: gshadow 파일 업데이트에 실패했습니다.\n");
    }
    
    printf("그룹 '%s' (GID: %d)가 성공적으로 생성되었습니다.\n", groupname, final_gid);
    return 1;
}

// addgroup 명령어 구현
int addgroup_command(int argc, char* argv[]) {
    gid_t gid = (gid_t)-1;
    int is_system = 0;
    int force = 0;
    char* groupname = NULL;
    
    // 인자 파싱
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--gid") == 0) {
            if (i + 1 < argc) {
                gid = atoi(argv[++i]);
                if (gid <= 0) {
                    fprintf(stderr, "오류: 유효하지 않은 GID입니다.\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "오류: -g 옵션에 GID가 필요합니다.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--system") == 0) {
            is_system = 1;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            force = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            groupname = argv[i];
        } else {
            fprintf(stderr, "알 수 없는 옵션: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // 그룹명 확인
    if (groupname == NULL) {
        fprintf(stderr, "오류: 그룹명을 입력해주세요.\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // 관리자 권한 확인
    if (!check_admin_privileges()) {
        return 1;
    }
    
    // 그룹명 유효성 확인
    if (!validate_groupname(groupname)) {
        return 1;
    }
    
    // 그룹 존재 확인
    if (check_group_exists(groupname)) {
        if (force) {
            printf("그룹 '%s'가 이미 존재합니다. (force 옵션으로 무시)\n", groupname);
            return 0;
        } else {
            fprintf(stderr, "오류: 그룹 '%s'가 이미 존재합니다.\n", groupname);
            return 1;
        }
    }
    
    // 시스템 그룹인데 일반 GID 범위 지정한 경우 확인
    if (is_system && gid != (gid_t)-1 && gid >= 1000) {
        fprintf(stderr, "경고: 시스템 그룹에 일반 사용자 GID 범위를 지정했습니다.\n");
    }
    
    // 그룹 생성 시도 (시스템 명령어 우선)
    if (add_group_system_command(groupname, gid, is_system)) {
        return 0;
    }
    
    // 시스템 명령어 실패 시 수동 생성 시도
    printf("시스템 명령어 실패, 수동 생성을 시도합니다...\n");
    if (add_group_manual(groupname, gid, is_system)) {
        return 0;
    }
    
    fprintf(stderr, "그룹 생성에 실패했습니다.\n");
    return 1;
}

int main(int argc, char* argv[]) {
    return addgroup_command(argc, argv);
}
```

## delgroup: 그룹 삭제 (관리자 권한)

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>

// 사용법 출력 함수
void print_usage(const char* program_name) {
    printf("사용법: %s [옵션] 그룹명\n", program_name);
    printf("또는: %s [옵션] 사용자명 그룹명\n", program_name);
    printf("옵션:\n");
    printf("  -f, --force          강제 삭제 (확인 없이)\n");
    printf("  --only-if-empty      그룹이 비어있을 때만 삭제\n");
    printf("  -h, --help           도움말 출력\n");
    printf("\n사용 모드:\n");
    printf("  1. 그룹 삭제: %s 그룹명\n", program_name);
    printf("  2. 사용자를 그룹에서 제거: %s 사용자명 그룹명\n", program_name);
    printf("\n예시:\n");
    printf("  %s mygroup           # mygroup 그룹 삭제\n", program_name);
    printf("  %s user1 mygroup     # user1을 mygroup에서 제거\n", program_name);
    printf("  %s -f mygroup        # 강제로 mygroup 삭제\n", program_name);
}

// 관리자 권한 확인 함수
int check_admin_privileges() {
    if (getuid() != 0) {
        fprintf(stderr, "오류: 이 명령어는 관리자 권한이 필요합니다.\n");
        fprintf(stderr, "sudo를 사용하여 실행하세요.\n");
        return 0;
    }
    return 1;
}

// 그룹 존재 확인 함수
struct group* check_group_exists(const char* groupname) {
    return getgrnam(groupname);
}

// 사용자 존재 확인 함수
struct passwd* check_user_exists(const char* username) {
    return getpwnam(username);
}

// 그룹이 다른 사용자의 주 그룹인지 확인
int is_primary_group_for_users(gid_t gid) {
    struct passwd* pwd;
    setpwent();
    
    while ((pwd = getpwent()) != NULL) {
        if (pwd->pw_gid == gid) {
            endpwent();
            return 1; // 주 그룹으로 사용되고 있음
        }
    }
    
    endpwent();
    return 0; // 주 그룹으로 사용되지 않음
}

// 그룹 멤버 목록 출력
void print_group_members(struct group* grp) {
    if (grp->gr_mem && grp->gr_mem[0]) {
        printf("그룹 멤버: ");
        for (int i = 0; grp->gr_mem[i]; i++) {
            printf("%s", grp->gr_mem[i]);
            if (grp->gr_mem[i + 1]) {
                printf(", ");
            }
        }
        printf("\n");
    } else {
        printf("그룹 멤버: 없음\n");
    }
}

// 사용자가 그룹의 멤버인지 확인
int is_user_in_group(const char* username, struct group* grp) {
    if (!grp->gr_mem) return 0;
    
    for (int i = 0; grp->gr_mem[i]; i++) {
        if (strcmp(grp->gr_mem[i], username) == 0) {
            return 1;
        }
    }
    return 0;
}

// 그룹 삭제 확인 함수
int confirm_group_deletion(const char* groupname, struct group* grp, int only_if_empty) {
    char response[10];
    
    printf("그룹 정보:\n");
    printf("  그룹명: %s\n", grp->gr_name);
    printf("  GID: %d\n", grp->gr_gid);
    print_group_members(grp);
    
    // 주 그룹 확인
    if (is_primary_group_for_users(grp->gr_gid)) {
        printf("  경고: 이 그룹은 일부 사용자의 주 그룹입니다!\n");
    }
    
    // only-if-empty 옵션 확인
    if (only_if_empty) {
        if (grp->gr_mem && grp->gr_mem[0]) {
            fprintf(stderr, "오류: 그룹이 비어있지 않습니다. (--only-if-empty 옵션)\n");
            return 0;
        }
        if (is_primary_group_for_users(grp->gr_gid)) {
            fprintf(stderr, "오류: 그룹이 사용자의 주 그룹으로 사용되고 있습니다.\n");
            return 0;
        }
    }
    
    printf("\n그룹 '%s'를 삭제하시겠습니까? [y/N]: ", groupname);
    
    if (fgets(response, sizeof(response), stdin) == NULL) {
        return 0;
    }
    
    return (response[0] == 'y' || response[0] == 'Y');
}

// /etc/group 파일에서 그룹 제거
int remove_group_from_file(const char* groupname) {
    FILE* group_file;
    char temp_file[] = "/tmp/group_temp_XXXXXX";
    FILE* temp_group;
    char line[1024];
    int temp_fd;
    int found = 0;
    
    // 임시 파일 생성
    temp_fd = mkstemp(temp_file);
    if (temp_fd == -1) {
        perror("임시 파일 생성 실패");
        return 0;
    }
    close(temp_fd);
    
    // /etc/group 파일 처리
    group_file = fopen("/etc/group", "r");
    temp_group = fopen(temp_file, "w");
    
    if (group_file == NULL || temp_group == NULL) {
        perror("group 파일 열기 실패");
        if (group_file) fclose(group_file);
        if (temp_group) fclose(temp_group);
        unlink(temp_file);
        return 0;
    }
    
    while (fgets(line, sizeof(line), group_file)) {
        char* line_copy = strdup(line);
        char* current_group = strtok(line_copy, ":");
        
        if (current_group && strcmp(current_group, groupname) == 0) {
            found = 1;
            printf("group 파일에서 그룹 제거: %s\n", groupname);
        } else {
            fputs(line, temp_group);
        }
        
        free(line_copy);
    }
    
    fclose(group_file);
    fclose(temp_group);
    
    if (!found) {
        fprintf(stderr, "경고: group 파일에서 그룹을 찾을 수 없습니다.\n");
        unlink(temp_file);
        return 0;
    }
    
    // 파일 권한 설정
    if (chmod(temp_file, 0644) != 0) {
        perror("파일 권한 설정 실패");
        unlink(temp_file);
        return 0;
    }
    
    // 원본 파일 교체
    if (rename(temp_file, "/etc/group") != 0) {
        perror("group 파일 교체 실패");
        unlink(temp_file);
        return 0;
    }
    
    return 1;
}

// /etc/gshadow 파일에서 그룹 제거
int remove_group_from_gshadow(const char* groupname) {
    FILE* gshadow_file;
    char temp_file[] = "/tmp/gshadow_temp_XXXXXX";
    FILE* temp_gshadow;
    char line[1024];
    int temp_fd;
    int found = 0;
    
    // gshadow 파일이 없으면 건너뛰기
    if (access("/etc/gshadow", F_OK) != 0) {
        return 1; // gshadow 없어도 성공으로 처리
    }
    
    // 임시 파일 생성
    temp_fd = mkstemp(temp_file);
    if (temp_fd == -1) {
        perror("gshadow 임시 파일 생성 실패");
        return 0;
    }
    close(temp_fd);
    
    // /etc/gshadow 파일 처리
    gshadow_file = fopen("/etc/gshadow", "r");
    temp_gshadow = fopen(temp_file, "w");
    
    if (gshadow_file == NULL || temp_gshadow == NULL) {
        perror("gshadow 파일 열기 실패");
        if (gshadow_file) fclose(gshadow_file);
        if (temp_gshadow) fclose(temp_gshadow);
        unlink(temp_file);
        return 0;
    }
    
    while (fgets(line, sizeof(line), gshadow_file)) {
        char* line_copy = strdup(line);
        char* current_group = strtok(line_copy, ":");
        
        if (current_group && strcmp(current_group, groupname) == 0) {
            found = 1;
            printf("gshadow 파일에서 그룹 제거: %s\n", groupname);
        } else {
            fputs(line, temp_gshadow);
        }
        
        free(line_copy);
    }
    
    fclose(gshadow_file);
    fclose(temp_gshadow);
    
    if (!found) {
        printf("gshadow 파일에 그룹이 없습니다.\n");
        unlink(temp_file);
        return 1; // 없어도 성공으로 처리
    }
    
    // 파일 권한 설정
    if (chmod(temp_file, 0640) != 0) {
        perror("gshadow 파일 권한 설정 실패");
        unlink(temp_file);
        return 0;
    }
    
    // 원본 파일 교체
    if (rename(temp_file, "/etc/gshadow") != 0) {
        perror("gshadow 파일 교체 실패");
        unlink(temp_file);
        return 0;
    }
    
    return 1;
}

// 모든 그룹에서 사용자 제거
int remove_user_from_all_groups(const char* username) {
    FILE* group_file;
    char temp_file[] = "/tmp/group_temp_XXXXXX";
    FILE* temp_group;
    char line[1024];
    int temp_fd;
    int modified = 0;
    
    // 임시 파일 생성
    temp_fd = mkstemp(temp_file);
    if (temp_fd == -1) {
        perror("임시 파일 생성 실패");
        return 0;
    }
    close(temp_fd);
    
    group_file = fopen("/etc/group", "r");
    temp_group = fopen(temp_file, "w");
    
    if (group_file == NULL || temp_group == NULL) {
        perror("group 파일 열기 실패");
        if (group_file) fclose(group_file);
        if (temp_group) fclose(temp_group);
        unlink(temp_file);
        return 0;
    }
    
    while (fgets(line, sizeof(line), group_file)) {
        char* line_copy = strdup(line);
        char* group_name = strtok(line_copy, ":");
        char* password = strtok(NULL, ":");
        char* gid_str = strtok(NULL, ":");
        char* members = strtok(NULL, ":");
        
        if (group_name && password && gid_str) {
            // 멤버 목록에서 사용자 제거
            if (members && strlen(members) > 0) {
                char new_members[1024] = "";
                char* member = strtok(members, ",");
                int first = 1;
                
                while (member) {
                    // 공백 제거
                    while (*member == ' ') member++;
                    char* end = member + strlen(member) - 1;
                    while (end > member && (*end == ' ' || *end == '\n' || *end == '\r')) {
                        *end = '\0';
                        end--;
                    }
                    
                    if (strcmp(member, username) != 0) {
                        if (!first) {
                            strcat(new_members, ",");
                        }
                        strcat(new_members, member);
                        first = 0;
                    } else {
                        modified = 1;
                        printf("그룹 %s에서 사용자 %s 제거\n", group_name, username);
                    }
                    member = strtok(NULL, ",");
                }
                
                fprintf(temp_group, "%s:%s:%s:%s\n", group_name, password, gid_str, new_members);
            } else {
                fprintf(temp_group, "%s:%s:%s:\n", group_name, password, gid_str);
            }
        }
        
        free(line_copy);
    }
    
    fclose(group_file);
    fclose(temp_group);
    
    if (!modified) {
        unlink(temp_file);
        return 1; // 변경사항 없어도 성공
    }
    
    // 파일 권한 설정
    if (chmod(temp_file, 0644) != 0) {
        perror("파일 권한 설정 실패");
        unlink(temp_file);
        return 0;
    }
    
    // 원본 파일 교체
    if (rename(temp_file, "/etc/group") != 0) {
        perror("group 파일 교체 실패");
        unlink(temp_file);
        return 0;
    }
    
    return 1;
}

// 특정 그룹에서 사용자 제거
int remove_user_from_group(const char* username, const char* groupname) {
    struct group* grp = check_group_exists(groupname);
    if (!grp) {
        fprintf(stderr, "오류: 그룹 '%s'가 존재하지 않습니다.\n", groupname);
        return 0;
    }
    
    struct passwd* pwd = check_user_exists(username);
    if (!pwd) {
        fprintf(stderr, "오류: 사용자 '%s'가 존재하지 않습니다.\n", username);
        return 0;
    }
    
    // 사용자가 그룹의 멤버인지 확인
    if (!is_user_in_group(username, grp)) {
        printf("사용자 '%s'는 그룹 '%s'의 멤버가 아닙니다.\n", username, groupname);
        return 1;
    }
    
    // 주 그룹인지 확인
    if (pwd->pw_gid == grp->gr_gid) {
        fprintf(stderr, "오류: '%s'는 사용자 '%s'의 주 그룹입니다. 주 그룹에서는 제거할 수 없습니다.\n", 
                groupname, username);
        return 0;
    }
    
    // 시스템 명령어 시도
    char command[256];
    snprintf(command, sizeof(command), "gpasswd -d %s %s", username, groupname);
    printf("시스템 명령어 실행: %s\n", command);
    
    int result = system(command);
    if (result == 0) {
        printf("사용자 '%s'가 그룹 '%s'에서 제거되었습니다.\n", username, groupname);
        return 1;
    }
    
    // 시스템 명령어 실패 시 수동 처리
    printf("시스템 명령어 실패, 수동 처리를 시도합니다...\n");
    return remove_user_from_all_groups(username);
}

// 시스템 명령어를 사용한 그룹 삭제
int delete_group_system_command(const char* groupname) {
    char command[256];
    snprintf(command, sizeof(command), "groupdel %s", groupname);
    
    printf("시스템 명령어 실행: %s\n", command);
    int result = system(command);
    
    if (result == 0) {
        printf("그룹 '%s'가 성공적으로 삭제되었습니다.\n", groupname);
        return 1;
    } else {
        fprintf(stderr, "그룹 삭제 중 오류가 발생했습니다.\n");
        return 0;
    }
}

// 수동 그룹 삭제
int delete_group_manual(const char* groupname) {
    // /etc/group에서 제거
    if (!remove_group_from_file(groupname)) {
        return 0;
    }
    
    // /etc/gshadow에서 제거
    if (!remove_group_from_gshadow(groupname)) {
        fprintf(stderr, "경고: gshadow 파일 업데이트에 실패했습니다.\n");
    }
    
    printf("그룹 '%s'가 성공적으로 삭제되었습니다.\n", groupname);
    return 1;
}

// delgroup 명령어 구현
int delgroup_command(int argc, char* argv[]) {
    int force = 0;
    int only_if_empty = 0;
    char* username = NULL;
    char* groupname = NULL;
    
    // 인자 파싱
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            force = 1;
        } else if (strcmp(argv[i], "--only-if-empty") == 0) {
            only_if_empty = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            if (username == NULL) {
                username = argv[i];
            } else if (groupname == NULL) {
                groupname = argv[i];
            } else {
                fprintf(stderr, "오류: 너무 많은 인자입니다.\n");
                print_usage(argv[0]);
                return 1;
            }
        } else {
            fprintf(stderr, "알 수 없는 옵션: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // 인자 검증
    if (username == NULL) {
        fprintf(stderr, "오류: 그룹명 또는 사용자명을 입력해주세요.\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // 관리자 권한 확인
    if (!check_admin_privileges()) {
        return 1;
    }
    
    // 모드 결정: 사용자를 그룹에서 제거 vs 그룹 삭제
    if (groupname != NULL) {
        // 사용자를 그룹에서 제거
        return remove_user_from_group(username, groupname) ? 0 : 1;
    } else {
        // 그룹 삭제
        groupname = username;
        
        // 그룹 존재 확인
        struct group* grp = check_group_exists(groupname);
        if (!grp) {
            fprintf(stderr, "오류: 그룹 '%s'가 존재하지 않습니다.\n", groupname);
            return 1;
        }
        
        // 시스템 그룹 보호
        if (grp->gr_gid < 100) {
            fprintf(stderr, "오류: 시스템 그룹 '%s' (GID: %d)는 삭제할 수 없습니다.\n", 
                    groupname, grp->gr_gid);
            return 1;
        }
        
        // root 그룹 보호
        if (strcmp(groupname, "root") == 0) {
            fprintf(stderr, "오류: root 그룹은 삭제할 수 없습니다.\n");
            return 1;
        }
        
        // 확인 (force 옵션이 없는 경우)
        if (!force) {
            if (!confirm_group_deletion(groupname, grp, only_if_empty)) {
                printf("그룹 삭제가 취소되었습니다.\n");
                return 0;
            }
        } else if (only_if_empty) {
            // force와 only-if-empty가 함께 사용된 경우에도 빈 그룹 확인
            if ((grp->gr_mem && grp->gr_mem[0]) || is_primary_group_for_users(grp->gr_gid)) {
                fprintf(stderr, "오류: 그룹이 비어있지 않습니다. (--only-if-empty 옵션)\n");
                return 1;
            }
        }
        
        // 그룹 삭제 시도 (시스템 명령어 우선)
        if (delete_group_system_command(groupname)) {
            return 0;
        }
        
        // 시스템 명령어 실패 시 수동 삭제 시도
        printf("시스템 명령어 실패, 수동 삭제를 시도합니다...\n");
        if (delete_group_manual(groupname)) {
            return 0;
        }
        
        fprintf(stderr, "그룹 삭제에 실패했습니다.\n");
        return 1;
    }
}

int main(int argc, char* argv[]) {
    return delgroup_command(argc, argv);
}
```

# IV. 네트워크 기능 (Network Functionality)

## ping: 네트워크 호스트에 대한 응답 시간 확인
- -c COUNT: 지정된 횟수만큼만 전송

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>

#define PACKET_SIZE 64
#define MAX_WAIT_TIME 5
#define PING_SLEEP_RATE 1000000

struct ping_packet {
    struct icmphdr hdr;
    char msg[PACKET_SIZE - sizeof(struct icmphdr)];
};

int sockfd;
int ping_count = 0;
int packets_sent = 0;
int packets_received = 0;
char *ping_target_ip;
char *ping_target_hostname;
struct sockaddr_in ping_addr;

// 체크섬 계산 함수
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    // 2바이트씩 더하기
    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }

    // 홀수 바이트가 남았을 경우
    if (len == 1) {
        sum += *(unsigned char*)buf << 8;
    }

    // 캐리 비트를 더하기
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    result = ~sum;
    return result;
}

// DNS 호스트명을 IP로 변환
char* dns_lookup(char *addr_host, struct sockaddr_in *addr_con) {
    struct hostent *host_entity;
    char *ip = malloc(16);
    
    if ((host_entity = gethostbyname(addr_host)) == NULL) {
        return NULL;
    }
    
    strcpy(ip, inet_ntoa(*((struct in_addr *)host_entity->h_addr)));
    (*addr_con).sin_family = host_entity->h_addrtype;
    (*addr_con).sin_port = 0;
    (*addr_con).sin_addr.s_addr = *((unsigned long *)host_entity->h_addr);
    
    return ip;
}

// 역방향 DNS 조회
char* reverse_dns_lookup(char *ip_addr) {
    struct sockaddr_in addr;
    socklen_t len;
    char buf[NI_MAXHOST], *ret_buf;
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    len = sizeof(struct sockaddr_in);
    
    if (getnameinfo((struct sockaddr *) &addr, len, buf, sizeof(buf), NULL, 0, NI_NAMEREQD)) {
        return NULL;
    }
    
    ret_buf = malloc(strlen(buf) + 1);
    strcpy(ret_buf, buf);
    return ret_buf;
}

// ping 패킷 전송
void send_ping(int ping_sockfd, struct sockaddr_in *ping_addr, char *ping_dom, char *ping_ip, char *rev_host) {
    int ttl_val = 64, msg_count = 0, flag = 1, msg_received_count = 0;
    
    struct ping_packet pckt;
    struct sockaddr_in r_addr;
    struct timespec time_start, time_end, tfs, tfe;
    long double rtt_msec = 0, total_msec = 0;
    struct timeval tv_out;
    tv_out.tv_sec = MAX_WAIT_TIME;
    tv_out.tv_usec = 0;

    clock_gettime(CLOCK_MONOTONIC, &tfs);

    // TTL 설정
    if (setsockopt(ping_sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0) {
        printf("\nTTL 설정 실패\n");
        return;
    }

    // 수신 타임아웃 설정
    setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof tv_out);

    printf("PING %s (%s) %d(%d) bytes of data.\n", ping_dom, ping_ip, PACKET_SIZE, PACKET_SIZE + 28);

    // ping 전송
    while (1) {
        // ping_count가 설정되어 있고 그 수만큼 보냈으면 종료
        if (ping_count > 0 && msg_count >= ping_count) {
            break;
        }

        flag = 1;

        // 패킷 초기화
        bzero(&pckt, sizeof(pckt));

        pckt.hdr.type = ICMP_ECHO;
        pckt.hdr.un.echo.id = getpid();
        
        for (int i = 0; i < sizeof(pckt.msg) - 1; i++) {
            pckt.msg[i] = i + '0';
        }
        
        pckt.msg[sizeof(pckt.msg) - 1] = 0;
        pckt.hdr.un.echo.sequence = msg_count++;
        pckt.hdr.checksum = 0;
        pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));

        usleep(PING_SLEEP_RATE);

        // 전송 시간 기록
        clock_gettime(CLOCK_MONOTONIC, &time_start);
        
        if (sendto(ping_sockfd, &pckt, sizeof(pckt), 0, (struct sockaddr*) ping_addr, sizeof(*ping_addr)) <= 0) {
            printf("\n패킷 전송 실패\n");
            flag = 0;
        }

        packets_sent++;

        // 응답 수신
        socklen_t addr_len = sizeof(r_addr);

        if (recvfrom(ping_sockfd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, &addr_len) <= 0 && msg_count > 1) {
            printf("\n패킷 수신 실패\n");
        } else {
            clock_gettime(CLOCK_MONOTONIC, &time_end);
            
            double timeElapsed = ((double)(time_end.tv_nsec - time_start.tv_nsec)) / 1000000.0;
            rtt_msec = (time_end.tv_sec - time_start.tv_sec) * 1000.0 + timeElapsed;
            
            if (flag) {
                if (!(pckt.hdr.type == 69 && pckt.hdr.code == 0)) {
                    printf("오류..패킷 수신됨 타입 %d 코드 %d\n", pckt.hdr.type, pckt.hdr.code);
                } else {
                    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.2Lf ms\n", 
                           PACKET_SIZE, ping_ip, msg_count, ttl_val, rtt_msec);
                    msg_received_count++;
                    packets_received++;
                }
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &tfe);
    double timeElapsed = ((double)(tfe.tv_nsec - tfs.tv_nsec)) / 1000000.0;
    total_msec = (tfe.tv_sec - tfs.tv_sec) * 1000.0 + timeElapsed;
    
    printf("\n--- %s ping statistics ---\n", ping_ip);
    printf("%d packets transmitted, %d received, %d%% packet loss, time %.0fms\n", 
           packets_sent, packets_received, 
           ((packets_sent - packets_received) / packets_sent) * 100, total_msec);
}

// 시그널 핸들러 (Ctrl+C)
void intHandler(int dummy) {
    printf("\n--- %s ping statistics ---\n", ping_target_ip);
    printf("%d packets transmitted, %d received, %d%% packet loss\n", 
           packets_sent, packets_received, 
           ((packets_sent - packets_received) / packets_sent) * 100);
    close(sockfd);
    exit(0);
}

int main(int argc, char *argv[]) {
    int sockfd;
    char *ip_str, *reverse_hostname;
    struct sockaddr_in addr_con;
    int addrlen = sizeof(addr_con);
    char net_buf[NI_MAXHOST];
    
    // 인자 파싱
    if (argc < 2) {
        printf("사용법: %s [-c count] hostname\n", argv[0]);
        return -1;
    }
    
    char *hostname = NULL;
    
    // -c 옵션 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            ping_count = atoi(argv[i + 1]);
            i++; // 다음 인자는 건너뛰기
        } else {
            hostname = argv[i];
        }
    }
    
    if (hostname == NULL) {
        printf("호스트명을 입력해주세요.\n");
        return -1;
    }

    // IP 주소로 변환
    ip_str = dns_lookup(hostname, &addr_con);
    if (ip_str == NULL) {
        printf("\nDNS 조회 실패.. 호스트명이나 IP를 확인해주세요\n");
        return -1;
    }

    ping_target_hostname = hostname;
    ping_target_ip = ip_str;
    ping_addr = addr_con;
    
    reverse_hostname = reverse_dns_lookup(ip_str);
    
    printf("PING %s (%s) %d(%d) bytes of data.\n", 
           reverse_hostname != NULL ? reverse_hostname : hostname, 
           ip_str, PACKET_SIZE, PACKET_SIZE + 28);

    // 원시 소켓 생성 (root 권한 필요)
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        printf("소켓 생성 실패. root 권한으로 실행해주세요.\n");
        return -1;
    }

    // 시그널 핸들러 등록
    signal(SIGINT, intHandler);

    // ping 전송
    send_ping(sockfd, &addr_con, reverse_hostname, ip_str, reverse_hostname);

    close(sockfd);
    return 0;
}
```

## ifconfig / ip addr (간략화): 네트워크 인터페이스 정보 출력

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>

// MAC 주소를 문자열로 변환
void format_mac_address(unsigned char *mac, char *buffer) {
    sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// 바이트 단위를 읽기 쉬운 형태로 변환
void format_bytes(unsigned long bytes, char *buffer) {
    if (bytes >= 1024 * 1024 * 1024) {
        sprintf(buffer, "%.1f GB", (double)bytes / (1024 * 1024 * 1024));
    } else if (bytes >= 1024 * 1024) {
        sprintf(buffer, "%.1f MB", (double)bytes / (1024 * 1024));
    } else if (bytes >= 1024) {
        sprintf(buffer, "%.1f KB", (double)bytes / 1024);
    } else {
        sprintf(buffer, "%lu B", bytes);
    }
}

// 인터페이스 플래그를 문자열로 변환
void format_flags(unsigned int flags, char *buffer) {
    buffer[0] = '\0';
    
    if (flags & IFF_UP) strcat(buffer, "UP ");
    if (flags & IFF_BROADCAST) strcat(buffer, "BROADCAST ");
    if (flags & IFF_DEBUG) strcat(buffer, "DEBUG ");
    if (flags & IFF_LOOPBACK) strcat(buffer, "LOOPBACK ");
    if (flags & IFF_POINTOPOINT) strcat(buffer, "POINTOPOINT ");
    if (flags & IFF_RUNNING) strcat(buffer, "RUNNING ");
    if (flags & IFF_NOARP) strcat(buffer, "NOARP ");
    if (flags & IFF_PROMISC) strcat(buffer, "PROMISC ");
    if (flags & IFF_NOTRAILERS) strcat(buffer, "NOTRAILERS ");
    if (flags & IFF_ALLMULTI) strcat(buffer, "ALLMULTI ");
    if (flags & IFF_MASTER) strcat(buffer, "MASTER ");
    if (flags & IFF_SLAVE) strcat(buffer, "SLAVE ");
    if (flags & IFF_MULTICAST) strcat(buffer, "MULTICAST ");
    
    // 마지막 공백 제거
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == ' ') {
        buffer[len - 1] = '\0';
    }
}

// 네트워크 통계 정보 읽기
int get_interface_stats(const char *interface, unsigned long *rx_bytes, 
                       unsigned long *tx_bytes, unsigned long *rx_packets, 
                       unsigned long *tx_packets) {
    FILE *fp;
    char path[256];
    char buffer[256];
    
    // RX bytes
    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/rx_bytes", interface);
    fp = fopen(path, "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            *rx_bytes = strtoul(buffer, NULL, 10);
        }
        fclose(fp);
    }
    
    // TX bytes
    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/tx_bytes", interface);
    fp = fopen(path, "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            *tx_bytes = strtoul(buffer, NULL, 10);
        }
        fclose(fp);
    }
    
    // RX packets
    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/rx_packets", interface);
    fp = fopen(path, "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            *rx_packets = strtoul(buffer, NULL, 10);
        }
        fclose(fp);
    }
    
    // TX packets
    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/tx_packets", interface);
    fp = fopen(path, "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            *tx_packets = strtoul(buffer, NULL, 10);
        }
        fclose(fp);
    }
    
    return 0;
}

// MTU 크기 가져오기
int get_mtu(const char *interface) {
    int sockfd;
    struct ifreq ifr;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return -1;
    
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    
    if (ioctl(sockfd, SIOCGIFMTU, &ifr) < 0) {
        close(sockfd);
        return -1;
    }
    
    close(sockfd);
    return ifr.ifr_mtu;
}

// 특정 인터페이스 정보 출력
void show_interface(const char *interface_name) {
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    char mac_str[18];
    char flags_str[256];
    char rx_bytes_str[32], tx_bytes_str[32];
    unsigned long rx_bytes = 0, tx_bytes = 0, rx_packets = 0, tx_packets = 0;
    int mtu;
    int found = 0;
    
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        // 특정 인터페이스만 출력하거나 모든 인터페이스 출력
        if (interface_name && strcmp(ifa->ifa_name, interface_name) != 0) {
            continue;
        }
        
        // 중복 출력 방지
        if (found && interface_name && strcmp(ifa->ifa_name, interface_name) == 0) {
            if (ifa->ifa_addr->sa_family != AF_INET && ifa->ifa_addr->sa_family != AF_INET6) {
                continue;
            }
        }
        
        if (!found || !interface_name || strcmp(ifa->ifa_name, interface_name) == 0) {
            // 인터페이스 헤더 출력 (처음 한 번만)
            static char last_interface[IFNAMSIZ] = "";
            if (strcmp(last_interface, ifa->ifa_name) != 0) {
                strcpy(last_interface, ifa->ifa_name);
                
                printf("%s: ", ifa->ifa_name);
                
                // 플래그 정보
                format_flags(ifa->ifa_flags, flags_str);
                printf("flags=%d<%s> ", ifa->ifa_flags, flags_str);
                
                // MTU 정보
                mtu = get_mtu(ifa->ifa_name);
                if (mtu > 0) {
                    printf("mtu %d\n", mtu);
                } else {
                    printf("\n");
                }
                
                // 통계 정보 가져오기
                get_interface_stats(ifa->ifa_name, &rx_bytes, &tx_bytes, &rx_packets, &tx_packets);
            }
        }
        
        // IP 주소 정보
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
            struct sockaddr_in *netmask_in = (struct sockaddr_in *)ifa->ifa_netmask;
            struct sockaddr_in *broadcast_in = (struct sockaddr_in *)ifa->ifa_broadaddr;
            
            printf("        inet %s", inet_ntoa(addr_in->sin_addr));
            
            if (netmask_in) {
                printf("  netmask %s", inet_ntoa(netmask_in->sin_addr));
            }
            
            if (broadcast_in && (ifa->ifa_flags & IFF_BROADCAST)) {
                printf("  broadcast %s", inet_ntoa(broadcast_in->sin_addr));
            }
            printf("\n");
        }
        
        // IPv6 주소 정보
        else if (ifa->ifa_addr->sa_family == AF_INET6) {
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)ifa->ifa_addr;
            char addr_str[INET6_ADDRSTRLEN];
            
            if (inet_ntop(AF_INET6, &addr_in6->sin6_addr, addr_str, INET6_ADDRSTRLEN)) {
                printf("        inet6 %s", addr_str);
                
                // Scope ID 출력
                if (addr_in6->sin6_scope_id > 0) {
                    printf("%%%d", addr_in6->sin6_scope_id);
                }
                
                // 프리픽스 길이 (간단히 /64로 가정)
                printf("/64");
                
                // 주소 타입
                if (IN6_IS_ADDR_LINKLOCAL(&addr_in6->sin6_addr)) {
                    printf("  scopeid 0x%x<link>", addr_in6->sin6_scope_id);
                } else if (IN6_IS_ADDR_LOOPBACK(&addr_in6->sin6_addr)) {
                    printf("  scopeid 0x%x<host>", addr_in6->sin6_scope_id);
                }
                printf("\n");
            }
        }
        
        // MAC 주소 정보 (패킷 소켓)
        else if (ifa->ifa_addr->sa_family == AF_PACKET) {
            struct sockaddr_ll *s = (struct sockaddr_ll *)ifa->ifa_addr;
            if (s->sll_halen == 6) {  // Ethernet MAC
                format_mac_address(s->sll_addr, mac_str);
                printf("        ether %s  txqueuelen 1000  (Ethernet)\n", mac_str);
                
                // 통계 정보 출력
                format_bytes(rx_bytes, rx_bytes_str);
                format_bytes(tx_bytes, tx_bytes_str);
                
                printf("        RX packets %lu  bytes %lu (%s)\n", rx_packets, rx_bytes, rx_bytes_str);
                printf("        RX errors 0  dropped 0  overruns 0  frame 0\n");
                printf("        TX packets %lu  bytes %lu (%s)\n", tx_packets, tx_bytes, tx_bytes_str);
                printf("        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0\n");
                printf("\n");
            }
        }
        
        found = 1;
        if (interface_name) break;
    }
    
    if (interface_name && !found) {
        printf("%s: error fetching interface information: Device not found\n", interface_name);
    }
    
    freeifaddrs(ifaddr);
}

// 사용법 출력
void print_usage(const char *program_name) {
    printf("Usage: %s [interface]\n", program_name);
    printf("Show configuration of network interface(s)\n");
    printf("\nOptions:\n");
    printf("  interface    Show only specified interface\n");
    printf("  (no args)    Show all interfaces\n");
}

int main(int argc, char *argv[]) {
    // 도움말 옵션
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return 0;
    }
    
    // 특정 인터페이스 지정
    if (argc == 2) {
        show_interface(argv[1]);
    } 
    // 모든 인터페이스 출력
    else if (argc == 1) {
        show_interface(NULL);
    } 
    // 잘못된 인자
    else {
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}
```

## wget (간략화): 웹에서 파일 다운로드 (HTTP/HTTPS 지원) 

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <ctype.h>
#include <time.h>

#define BUFFER_SIZE 8192
#define MAX_URL_SIZE 2048
#define MAX_HEADER_SIZE 4096
#define HTTP_PORT 80
#define HTTPS_PORT 443

typedef struct {
    char protocol[10];
    char hostname[256];
    int port;
    char path[1024];
} url_info_t;

typedef struct {
    long content_length;
    int status_code;
    char filename[256];
    char content_type[128];
    int chunked;
} http_response_t;

// URL 파싱
int parse_url(const char *url, url_info_t *info) {
    const char *ptr = url;
    
    // 프로토콜 추출
    if (strncmp(ptr, "http://", 7) == 0) {
        strcpy(info->protocol, "http");
        info->port = HTTP_PORT;
        ptr += 7;
    } else if (strncmp(ptr, "https://", 8) == 0) {
        strcpy(info->protocol, "https");
        info->port = HTTPS_PORT;
        ptr += 8;
    } else {
        // 프로토콜이 없으면 http로 가정
        strcpy(info->protocol, "http");
        info->port = HTTP_PORT;
    }
    
    // 호스트명과 포트 추출
    const char *slash = strchr(ptr, '/');
    const char *colon = strchr(ptr, ':');
    
    if (colon && (slash == NULL || colon < slash)) {
        // 포트가 지정된 경우
        int hostname_len = colon - ptr;
        strncpy(info->hostname, ptr, hostname_len);
        info->hostname[hostname_len] = '\0';
        
        info->port = atoi(colon + 1);
        ptr = slash ? slash : ptr + strlen(ptr);
    } else {
        // 포트가 지정되지 않은 경우
        int hostname_len = slash ? (slash - ptr) : strlen(ptr);
        strncpy(info->hostname, ptr, hostname_len);
        info->hostname[hostname_len] = '\0';
        ptr = slash ? slash : ptr + strlen(ptr);
    }
    
    // 경로 추출
    if (*ptr == '\0') {
        strcpy(info->path, "/");
    } else {
        strcpy(info->path, ptr);
    }
    
    return 0;
}

// 파일명 추출
void extract_filename(const char *url, const char *path, char *filename) {
    const char *last_slash = strrchr(path, '/');
    if (last_slash && strlen(last_slash + 1) > 0) {
        strcpy(filename, last_slash + 1);
    } else {
        // URL에서 호스트명 추출하여 기본 파일명으로 사용
        const char *host_start = strstr(url, "://");
        if (host_start) {
            host_start += 3;
            const char *host_end = strchr(host_start, '/');
            if (host_end) {
                int len = host_end - host_start;
                strncpy(filename, host_start, len);
                filename[len] = '\0';
            } else {
                strcpy(filename, host_start);
            }
        } else {
            strcpy(filename, "index.html");
        }
    }
}

// 소켓 연결
int connect_to_server(const char *hostname, int port) {
    struct sockaddr_in server_addr;
    struct hostent *server;
    int sockfd;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "Host not found: %s\n", hostname);
        close(sockfd);
        return -1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// HTTP 헤더 파싱
void parse_http_response(const char *response, http_response_t *info) {
    char *line, *response_copy;
    char *saveptr;
    
    response_copy = strdup(response);
    
    // 첫 번째 줄에서 상태 코드 추출
    line = strtok_r(response_copy, "\r\n", &saveptr);
    if (line && strncmp(line, "HTTP/", 5) == 0) {
        sscanf(line, "HTTP/%*s %d", &info->status_code);
    }
    
    // 나머지 헤더 파싱
    while ((line = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
        if (strncasecmp(line, "Content-Length:", 15) == 0) {
            info->content_length = atol(line + 15);
        } else if (strncasecmp(line, "Content-Type:", 13) == 0) {
            sscanf(line + 13, "%127s", info->content_type);
        } else if (strncasecmp(line, "Transfer-Encoding:", 18) == 0) {
            if (strstr(line + 18, "chunked")) {
                info->chunked = 1;
            }
        }
    }
    
    free(response_copy);
}

// 청크 데이터 디코딩
int decode_chunked_data(const char *data, int data_len, char *output, int output_size) {
    int i = 0, output_len = 0;
    
    while (i < data_len && output_len < output_size - 1) {
        // 청크 크기 읽기
        char chunk_size_str[16] = {0};
        int j = 0;
        
        while (i < data_len && data[i] != '\r' && j < 15) {
            chunk_size_str[j++] = data[i++];
        }
        
        if (i >= data_len - 1) break;
        
        // \r\n 건너뛰기
        i += 2;
        
        int chunk_size = strtol(chunk_size_str, NULL, 16);
        if (chunk_size == 0) break; // 마지막 청크
        
        // 청크 데이터 복사
        int copy_len = (chunk_size < output_size - output_len - 1) ? 
                       chunk_size : output_size - output_len - 1;
        memcpy(output + output_len, data + i, copy_len);
        output_len += copy_len;
        i += chunk_size + 2; // 청크 데이터 + \r\n
    }
    
    return output_len;
}

// 진행률 표시
void show_progress(long downloaded, long total, time_t start_time) {
    time_t current_time = time(NULL);
    double elapsed = difftime(current_time, start_time);
    
    if (total > 0) {
        int percent = (int)((downloaded * 100) / total);
        double speed = downloaded / (elapsed > 0 ? elapsed : 1);
        
        printf("\r%ld/%ld bytes (%d%%) downloaded at %.1f KB/s", 
               downloaded, total, percent, speed / 1024);
    } else {
        double speed = downloaded / (elapsed > 0 ? elapsed : 1);
        printf("\r%ld bytes downloaded at %.1f KB/s", downloaded, speed / 1024);
    }
    fflush(stdout);
}

// HTTP 다운로드
int download_http(const url_info_t *url_info, const char *output_filename) {
    int sockfd;
    char request[1024];
    char buffer[BUFFER_SIZE];
    char header_buffer[MAX_HEADER_SIZE];
    FILE *output_file;
    http_response_t response_info = {0};
    long downloaded = 0;
    int header_received = 0;
    int header_len = 0;
    time_t start_time;
    
    printf("Connecting to %s:%d...\n", url_info->hostname, url_info->port);
    
    sockfd = connect_to_server(url_info->hostname, url_info->port);
    if (sockfd < 0) {
        return -1;
    }
    
    printf("Connected.\n");
    
    // HTTP 요청 생성
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: wget/1.0\r\n"
             "Connection: close\r\n"
             "\r\n",
             url_info->path, url_info->hostname);
    
    // 요청 전송
    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("send");
        close(sockfd);
        return -1;
    }
    
    printf("HTTP request sent, awaiting response...\n");
    
    // 출력 파일 열기
    output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("fopen");
        close(sockfd);
        return -1;
    }
    
    start_time = time(NULL);
    
    // 응답 수신
    while (1) {
        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) break;
        
        buffer[bytes_received] = '\0';
        
        if (!header_received) {
            // 헤더와 바디를 분리
            int copy_len = (header_len + bytes_received < MAX_HEADER_SIZE - 1) ?
                          bytes_received : MAX_HEADER_SIZE - 1 - header_len;
            memcpy(header_buffer + header_len, buffer, copy_len);
            header_len += copy_len;
            header_buffer[header_len] = '\0';
            
            char *header_end = strstr(header_buffer, "\r\n\r\n");
            if (header_end) {
                header_received = 1;
                parse_http_response(header_buffer, &response_info);
                
                printf("Response: %d\n", response_info.status_code);
                if (response_info.content_length > 0) {
                    printf("Length: %ld bytes\n", response_info.content_length);
                }
                printf("Saving to: %s\n\n", output_filename);
                
                if (response_info.status_code != 200) {
                    printf("HTTP Error: %d\n", response_info.status_code);
                    fclose(output_file);
                    close(sockfd);
                    unlink(output_filename);
                    return -1;
                }
                
                // 헤더 이후의 데이터 처리
                int body_start = (header_end - header_buffer) + 4;
                int remaining_data = header_len - body_start;
                if (remaining_data > 0) {
                    fwrite(header_buffer + body_start, 1, remaining_data, output_file);
                    downloaded += remaining_data;
                }
            }
        } else {
            // 바디 데이터 저장
            fwrite(buffer, 1, bytes_received, output_file);
            downloaded += bytes_received;
        }
        
        if (header_received) {
            show_progress(downloaded, response_info.content_length, start_time);
        }
    }
    
    printf("\n\nDownload completed: %s (%ld bytes)\n", output_filename, downloaded);
    
    fclose(output_file);
    close(sockfd);
    return 0;
}

// HTTPS 다운로드
int download_https(const url_info_t *url_info, const char *output_filename) {
    int sockfd;
    SSL_CTX *ctx;
    SSL *ssl;
    char request[1024];
    char buffer[BUFFER_SIZE];
    char header_buffer[MAX_HEADER_SIZE];
    FILE *output_file;
    http_response_t response_info = {0};
    long downloaded = 0;
    int header_received = 0;
    int header_len = 0;
    time_t start_time;
    
    // SSL 초기화
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    printf("Connecting to %s:%d...\n", url_info->hostname, url_info->port);
    
    sockfd = connect_to_server(url_info->hostname, url_info->port);
    if (sockfd < 0) {
        SSL_CTX_free(ctx);
        return -1;
    }
    
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    
    if (SSL_connect(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    
    printf("SSL connection established.\n");
    
    // HTTPS 요청 생성
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: wget/1.0\r\n"
             "Connection: close\r\n"
             "\r\n",
             url_info->path, url_info->hostname);
    
    // 요청 전송
    if (SSL_write(ssl, request, strlen(request)) < 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    
    printf("HTTPS request sent, awaiting response...\n");
    
    // 출력 파일 열기
    output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("fopen");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    
    start_time = time(NULL);
    
    // 응답 수신
    while (1) {
        int bytes_received = SSL_read(ssl, buffer, BUFFER_SIZE - 1);
        if (bytes_received <= 0) break;
        
        buffer[bytes_received] = '\0';
        
        if (!header_received) {
            // 헤더와 바디를 분리
            int copy_len = (header_len + bytes_received < MAX_HEADER_SIZE - 1) ?
                          bytes_received : MAX_HEADER_SIZE - 1 - header_len;
            memcpy(header_buffer + header_len, buffer, copy_len);
            header_len += copy_len;
            header_buffer[header_len] = '\0';
            
            char *header_end = strstr(header_buffer, "\r\n\r\n");
            if (header_end) {
                header_received = 1;
                parse_http_response(header_buffer, &response_info);
                
                printf("Response: %d\n", response_info.status_code);
                if (response_info.content_length > 0) {
                    printf("Length: %ld bytes\n", response_info.content_length);
                }
                printf("Saving to: %s\n\n", output_filename);
                
                if (response_info.status_code != 200) {
                    printf("HTTP Error: %d\n", response_info.status_code);
                    fclose(output_file);
                    SSL_free(ssl);
                    SSL_CTX_free(ctx);
                    close(sockfd);
                    unlink(output_filename);
                    return -1;
                }
                
                // 헤더 이후의 데이터 처리
                int body_start = (header_end - header_buffer) + 4;
                int remaining_data = header_len - body_start;
                if (remaining_data > 0) {
                    fwrite(header_buffer + body_start, 1, remaining_data, output_file);
                    downloaded += remaining_data;
                }
            }
        } else {
            // 바디 데이터 저장
            fwrite(buffer, 1, bytes_received, output_file);
            downloaded += bytes_received;
        }
        
        if (header_received) {
            show_progress(downloaded, response_info.content_length, start_time);
        }
    }
    
    printf("\n\nDownload completed: %s (%ld bytes)\n", output_filename, downloaded);
    
    fclose(output_file);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sockfd);
    return 0;
}

// 사용법 출력
void print_usage(const char *program_name) {
    printf("Usage: %s [OPTION]... [URL]...\n", program_name);
    printf("Download files from web servers via HTTP/HTTPS.\n\n");
    printf("Options:\n");
    printf("  -O file    save document to file\n");
    printf("  -h, --help display this help and exit\n");
    printf("\nExamples:\n");
    printf("  %s http://example.com/file.txt\n", program_name);
    printf("  %s -O myfile.txt https://example.com/file.txt\n", program_name);
}

int main(int argc, char *argv[]) {
    char *url = NULL;
    char *output_filename = NULL;
    char default_filename[256];
    url_info_t url_info;
    int i;
    
    // 인자 파싱
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-O") == 0 && i + 1 < argc) {
            output_filename = argv[++i];
        } else if (argv[i][0] != '-') {
            url = argv[i];
        }
    }
    
    if (!url) {
        printf("Error: No URL specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // URL 파싱
    if (parse_url(url, &url_info) != 0) {
        printf("Error: Invalid URL format\n");
        return 1;
    }
    
    // 출력 파일명 결정
    if (!output_filename) {
        extract_filename(url, url_info.path, default_filename);
        output_filename = default_filename;
    }
    
    printf("URL: %s\n", url);
    printf("Protocol: %s\n", url_info.protocol);
    printf("Host: %s\n", url_info.hostname);
    printf("Port: %d\n", url_info.port);
    printf("Path: %s\n", url_info.path);
    printf("Output: %s\n\n", output_filename);
    
    // 프로토콜에 따라 다운로드
    if (strcmp(url_info.protocol, "https") == 0) {
        return download_https(&url_info, output_filename);
    } else {
        return download_http(&url_info, output_filename);
    }
}
```

## curl (간략화): URL로부터 데이터 전송 (HTTP/HTTPS 지원)

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <ctype.h>
#include <time.h>

#define BUFFER_SIZE 8192
#define MAX_URL_SIZE 2048
#define MAX_HEADER_SIZE 4096
#define MAX_HEADERS 50
#define HTTP_PORT 80
#define HTTPS_PORT 443

typedef struct {
    char protocol[10];
    char hostname[256];
    int port;
    char path[1024];
} url_info_t;

typedef struct {
    long content_length;
    int status_code;
    char status_text[128];
    char content_type[128];
    int chunked;
    char location[512];  // 리다이렉션용
} http_response_t;

typedef struct {
    char method[16];
    char *data;
    char *headers[MAX_HEADERS];
    int header_count;
    char *output_file;
    int include_headers;
    int verbose;
    int silent;
    int follow_redirects;
    int max_redirects;
    char *user_agent;
} curl_options_t;

// URL 파싱
int parse_url(const char *url, url_info_t *info) {
    const char *ptr = url;
    
    // 프로토콜 추출
    if (strncmp(ptr, "http://", 7) == 0) {
        strcpy(info->protocol, "http");
        info->port = HTTP_PORT;
        ptr += 7;
    } else if (strncmp(ptr, "https://", 8) == 0) {
        strcpy(info->protocol, "https");
        info->port = HTTPS_PORT;
        ptr += 8;
    } else {
        strcpy(info->protocol, "http");
        info->port = HTTP_PORT;
    }
    
    // 호스트명과 포트 추출
    const char *slash = strchr(ptr, '/');
    const char *colon = strchr(ptr, ':');
    
    if (colon && (slash == NULL || colon < slash)) {
        int hostname_len = colon - ptr;
        strncpy(info->hostname, ptr, hostname_len);
        info->hostname[hostname_len] = '\0';
        
        info->port = atoi(colon + 1);
        ptr = slash ? slash : ptr + strlen(ptr);
    } else {
        int hostname_len = slash ? (slash - ptr) : strlen(ptr);
        strncpy(info->hostname, ptr, hostname_len);
        info->hostname[hostname_len] = '\0';
        ptr = slash ? slash : ptr + strlen(ptr);
    }
    
    // 경로 추출
    if (*ptr == '\0') {
        strcpy(info->path, "/");
    } else {
        strcpy(info->path, ptr);
    }
    
    return 0;
}

// 소켓 연결
int connect_to_server(const char *hostname, int port) {
    struct sockaddr_in server_addr;
    struct hostent *server;
    int sockfd;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "Host not found: %s\n", hostname);
        close(sockfd);
        return -1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// HTTP 응답 파싱
void parse_http_response(const char *response, http_response_t *info) {
    char *line, *response_copy;
    char *saveptr;
    
    response_copy = strdup(response);
    
    // 첫 번째 줄에서 상태 코드 추출
    line = strtok_r(response_copy, "\r\n", &saveptr);
    if (line && strncmp(line, "HTTP/", 5) == 0) {
        sscanf(line, "HTTP/%*s %d %127[^\r\n]", &info->status_code, info->status_text);
    }
    
    // 나머지 헤더 파싱
    while ((line = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
        if (strncasecmp(line, "Content-Length:", 15) == 0) {
            info->content_length = atol(line + 15);
        } else if (strncasecmp(line, "Content-Type:", 13) == 0) {
            sscanf(line + 13, " %127[^\r\n]", info->content_type);
        } else if (strncasecmp(line, "Transfer-Encoding:", 18) == 0) {
            if (strstr(line + 18, "chunked")) {
                info->chunked = 1;
            }
        } else if (strncasecmp(line, "Location:", 9) == 0) {
            sscanf(line + 9, " %511[^\r\n]", info->location);
        }
    }
    
    free(response_copy);
}

// HTTP 요청 생성
void build_http_request(const url_info_t *url_info, const curl_options_t *opts, char *request, size_t size) {
    int len = 0;
    
    // 요청 라인
    len += snprintf(request + len, size - len, "%s %s HTTP/1.1\r\n", opts->method, url_info->path);
    
    // 기본 헤더
    len += snprintf(request + len, size - len, "Host: %s\r\n", url_info->hostname);
    
    // User-Agent
    if (opts->user_agent) {
        len += snprintf(request + len, size - len, "User-Agent: %s\r\n", opts->user_agent);
    } else {
        len += snprintf(request + len, size - len, "User-Agent: curl/1.0\r\n");
    }
    
    // 사용자 정의 헤더
    for (int i = 0; i < opts->header_count; i++) {
        len += snprintf(request + len, size - len, "%s\r\n", opts->headers[i]);
    }
    
    // POST 데이터가 있는 경우
    if (opts->data) {
        len += snprintf(request + len, size - len, "Content-Type: application/x-www-form-urlencoded\r\n");
        len += snprintf(request + len, size - len, "Content-Length: %zu\r\n", strlen(opts->data));
    }
    
    len += snprintf(request + len, size - len, "Connection: close\r\n");
    len += snprintf(request + len, size - len, "\r\n");
    
    // POST 데이터 추가
    if (opts->data) {
        len += snprintf(request + len, size - len, "%s", opts->data);
    }
}

// HTTP 요청 처리
int process_http_request(const url_info_t *url_info, const curl_options_t *opts, FILE *output) {
    int sockfd;
    char request[4096];
    char buffer[BUFFER_SIZE];
    char header_buffer[MAX_HEADER_SIZE];
    http_response_t response_info = {0};
    int header_received = 0;
    int header_len = 0;
    
    if (opts->verbose) {
        printf("* Connecting to %s:%d...\n", url_info->hostname, url_info->port);
    }
    
    sockfd = connect_to_server(url_info->hostname, url_info->port);
    if (sockfd < 0) {
        return -1;
    }
    
    if (opts->verbose) {
        printf("* Connected to %s (%s) port %d\n", url_info->hostname, url_info->hostname, url_info->port);
    }
    
    // HTTP 요청 생성
    build_http_request(url_info, opts, request, sizeof(request));
    
    if (opts->verbose) {
        printf("> %s", request);
    }
    
    // 요청 전송
    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("send");
        close(sockfd);
        return -1;
    }
    
    // 응답 수신
    while (1) {
        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) break;
        
        buffer[bytes_received] = '\0';
        
        if (!header_received) {
            // 헤더와 바디를 분리
            int copy_len = (header_len + bytes_received < MAX_HEADER_SIZE - 1) ?
                          bytes_received : MAX_HEADER_SIZE - 1 - header_len;
            memcpy(header_buffer + header_len, buffer, copy_len);
            header_len += copy_len;
            header_buffer[header_len] = '\0';
            
            char *header_end = strstr(header_buffer, "\r\n\r\n");
            if (header_end) {
                header_received = 1;
                parse_http_response(header_buffer, &response_info);
                
                if (opts->verbose) {
                    printf("< HTTP/1.1 %d %s\n", response_info.status_code, response_info.status_text);
                    
                    // 헤더 출력
                    char *line, *header_copy = strdup(header_buffer);
                    char *saveptr;
                    strtok_r(header_copy, "\r\n", &saveptr); // 첫 번째 줄 건너뛰기
                    while ((line = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
                        if (strlen(line) > 0) {
                            printf("< %s\n", line);
                        }
                    }
                    free(header_copy);
                    printf("< \n");
                }
                
                // 헤더 포함 옵션
                if (opts->include_headers) {
                    int header_only_len = (header_end - header_buffer) + 4;
                    fwrite(header_buffer, 1, header_only_len, output);
                }
                
                // 헤더 이후의 데이터 처리
                int body_start = (header_end - header_buffer) + 4;
                int remaining_data = header_len - body_start;
                if (remaining_data > 0) {
                    fwrite(header_buffer + body_start, 1, remaining_data, output);
                }
            }
        } else {
            // 바디 데이터 출력
            fwrite(buffer, 1, bytes_received, output);
        }
    }
    
    close(sockfd);
    
    // 리다이렉션 처리
    if (opts->follow_redirects && (response_info.status_code == 301 || response_info.status_code == 302 || 
        response_info.status_code == 303 || response_info.status_code == 307 || response_info.status_code == 308)) {
        if (strlen(response_info.location) > 0) {
            if (opts->verbose) {
                printf("* Following redirect to %s\n", response_info.location);
            }
            url_info_t new_url;
            parse_url(response_info.location, &new_url);
            return process_http_request(&new_url, opts, output);
        }
    }
    
    return response_info.status_code;
}

// HTTPS 요청 처리
int process_https_request(const url_info_t *url_info, const curl_options_t *opts, FILE *output) {
    int sockfd;
    SSL_CTX *ctx;
    SSL *ssl;
    char request[4096];
    char buffer[BUFFER_SIZE];
    char header_buffer[MAX_HEADER_SIZE];
    http_response_t response_info = {0};
    int header_received = 0;
    int header_len = 0;
    
    // SSL 초기화
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    if (opts->verbose) {
        printf("* Connecting to %s:%d...\n", url_info->hostname, url_info->port);
    }
    
    sockfd = connect_to_server(url_info->hostname, url_info->port);
    if (sockfd < 0) {
        SSL_CTX_free(ctx);
        return -1;
    }
    
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    
    if (SSL_connect(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    
    if (opts->verbose) {
        printf("* SSL connection established\n");
    }
    
    // HTTPS 요청 생성
    build_http_request(url_info, opts, request, sizeof(request));
    
    if (opts->verbose) {
        printf("> %s", request);
    }
    
    // 요청 전송
    if (SSL_write(ssl, request, strlen(request)) < 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    
    // 응답 수신
    while (1) {
        int bytes_received = SSL_read(ssl, buffer, BUFFER_SIZE - 1);
        if (bytes_received <= 0) break;
        
        buffer[bytes_received] = '\0';
        
        if (!header_received) {
            // 헤더와 바디를 분리
            int copy_len = (header_len + bytes_received < MAX_HEADER_SIZE - 1) ?
                          bytes_received : MAX_HEADER_SIZE - 1 - header_len;
            memcpy(header_buffer + header_len, buffer, copy_len);
            header_len += copy_len;
            header_buffer[header_len] = '\0';
            
            char *header_end = strstr(header_buffer, "\r\n\r\n");
            if (header_end) {
                header_received = 1;
                parse_http_response(header_buffer, &response_info);
                
                if (opts->verbose) {
                    printf("< HTTP/1.1 %d %s\n", response_info.status_code, response_info.status_text);
                    
                    // 헤더 출력
                    char *line, *header_copy = strdup(header_buffer);
                    char *saveptr;
                    strtok_r(header_copy, "\r\n", &saveptr); // 첫 번째 줄 건너뛰기
                    while ((line = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
                        if (strlen(line) > 0) {
                            printf("< %s\n", line);
                        }
                    }
                    free(header_copy);
                    printf("< \n");
                }
                
                // 헤더 포함 옵션
                if (opts->include_headers) {
                    int header_only_len = (header_end - header_buffer) + 4;
                    fwrite(header_buffer, 1, header_only_len, output);
                }
                
                // 헤더 이후의 데이터 처리
                int body_start = (header_end - header_buffer) + 4;
                int remaining_data = header_len - body_start;
                if (remaining_data > 0) {
                    fwrite(header_buffer + body_start, 1, remaining_data, output);
                }
            }
        } else {
            // 바디 데이터 출력
            fwrite(buffer, 1, bytes_received, output);
        }
    }
    
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sockfd);
    
    // 리다이렉션 처리
    if (opts->follow_redirects && (response_info.status_code == 301 || response_info.status_code == 302 || 
        response_info.status_code == 303 || response_info.status_code == 307 || response_info.status_code == 308)) {
        if (strlen(response_info.location) > 0) {
            if (opts->verbose) {
                printf("* Following redirect to %s\n", response_info.location);
            }
            url_info_t new_url;
            parse_url(response_info.location, &new_url);
            if (strcmp(new_url.protocol, "https") == 0) {
                return process_https_request(&new_url, opts, output);
            } else {
                return process_http_request(&new_url, opts, output);
            }
        }
    }
    
    return response_info.status_code;
}

// 사용법 출력
void print_usage(const char *program_name) {
    printf("Usage: %s [options...] <url>\n", program_name);
    printf("Transfer data from or to a server\n\n");
    printf("Options:\n");
    printf("  -X, --request <method>     HTTP method (GET, POST, PUT, DELETE, etc.)\n");
    printf("  -d, --data <data>          HTTP POST data\n");
    printf("  -H, --header <header>      Custom header to pass to server\n");
    printf("  -o, --output <file>        Write output to file instead of stdout\n");
    printf("  -i, --include              Include HTTP headers in output\n");
    printf("  -v, --verbose              Make the operation more talkative\n");
    printf("  -s, --silent               Silent mode\n");
    printf("  -L, --location             Follow redirects\n");
    printf("  -A, --user-agent <agent>   User-Agent to send to server\n");
    printf("  -h, --help                 Display this help and exit\n");
    printf("\nExamples:\n");
    printf("  %s http://example.com\n", program_name);
    printf("  %s -X POST -d \"name=value\" http://example.com/api\n", program_name);
    printf("  %s -H \"Authorization: Bearer token\" https://api.example.com\n", program_name);
    printf("  %s -o output.html -L http://example.com\n", program_name);
}

int main(int argc, char *argv[]) {
    char *url = NULL;
    curl_options_t opts = {0};
    url_info_t url_info;
    FILE *output = stdout;
    int result;
    
    // 기본값 설정
    strcpy(opts.method, "GET");
    opts.follow_redirects = 0;
    opts.max_redirects = 10;
    
    // 인자 파싱
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if ((strcmp(argv[i], "-X") == 0 || strcmp(argv[i], "--request") == 0) && i + 1 < argc) {
            strncpy(opts.method, argv[++i], sizeof(opts.method) - 1);
        } else if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0) && i + 1 < argc) {
            opts.data = argv[++i];
            if (strcmp(opts.method, "GET") == 0) {
                strcpy(opts.method, "POST");
            }
        } else if ((strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--header") == 0) && i + 1 < argc) {
            if (opts.header_count < MAX_HEADERS) {
                opts.headers[opts.header_count++] = argv[++i];
            }
        } else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i + 1 < argc) {
            opts.output_file = argv[++i];
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--include") == 0) {
            opts.include_headers = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            opts.verbose = 1;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--silent") == 0) {
            opts.silent = 1;
        } else if (strcmp(argv[i], "-L") == 0 || strcmp(argv[i], "--location") == 0) {
            opts.follow_redirects = 1;
        } else if ((strcmp(argv[i], "-A") == 0 || strcmp(argv[i], "--user-agent") == 0) && i + 1 < argc) {
            opts.user_agent = argv[++i];
        } else if (argv[i][0] != '-') {
            url = argv[i];
        }
    }
    
    if (!url) {
        fprintf(stderr, "Error: No URL specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // URL 파싱
    if (parse_url(url, &url_info) != 0) {
        fprintf(stderr, "Error: Invalid URL format\n");
        return 1;
    }
    
    // 출력 파일 열기
    if (opts.output_file) {
        output = fopen(opts.output_file, "wb");
        if (!output) {
            perror("fopen");
            return 1;
        }
    }
    
    // 요청 처리
    if (strcmp(url_info.protocol, "https") == 0) {
        result = process_https_request(&url_info, &opts, output);
    } else {
        result = process_http_request(&url_info, &opts, output);
    }
    
    if (output != stdout) {
        fclose(output);
    }
    
    if (!opts.silent && opts.verbose) {
        printf("* Request completed with status code: %d\n", result);
    }
    
    return (result >= 200 && result < 300) ? 0 : 1;
}
```

# V. 입출력 재지향 및 파이프 (Redirection and Pipes)

## >: 표준 출력 파일로 재지향 (덮어쓰기)

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64

// 명령어를 파싱하여 인자 배열로 변환
int parse_command(char *cmd, char **args) {
    int argc = 0;
    char *token = strtok(cmd, " \t\n");
    
    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc] = token;
        argc++;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;
    return argc;
}

// 리다이렉션 처리 함수
int handle_redirection(char **args, int argc) {
    int i;
    char *output_file = NULL;
    
    // '>' 기호를 찾아서 출력 파일명 확인
    for (i = 0; i < argc; i++) {
        if (strcmp(args[i], ">") == 0) {
            if (i + 1 < argc) {
                output_file = args[i + 1];
                args[i] = NULL;  // '>' 기호 제거
                break;
            } else {
                printf("오류: '>' 뒤에 파일명이 없습니다.\n");
                return -1;
            }
        }
    }
    
    // 리다이렉션이 있는 경우
    if (output_file != NULL) {
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("파일 열기 오류");
            return -1;
        }
        
        // 표준 출력을 파일로 리다이렉션
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("리다이렉션 오류");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    return 0;
}

// 명령어 실행 함수
void execute_command(char **args, int argc) {
    pid_t pid = fork();
    
    if (pid == 0) {  // 자식 프로세스
        // 리다이렉션 처리
        if (handle_redirection(args, argc) == -1) {
            exit(1);
        }
        
        // 명령어 실행
        if (execvp(args[0], args) == -1) {
            perror("명령어 실행 오류");
            exit(1);
        }
    } else if (pid > 0) {  // 부모 프로세스
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork 오류");
    }
}

// 내장 명령어 처리
int handle_builtin(char **args, int argc) {
    if (argc == 0) return 0;
    
    // exit 명령어
    if (strcmp(args[0], "exit") == 0) {
        printf("터미널을 종료합니다.\n");
        exit(0);
    }
    
    // cd 명령어
    if (strcmp(args[0], "cd") == 0) {
        if (argc < 2) {
            printf("사용법: cd <디렉토리>\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("디렉토리 변경 오류");
            }
        }
        return 1;
    }
    
    return 0;  // 내장 명령어가 아님
}

int main() {
    char command[MAX_CMD_LEN];
    char *args[MAX_ARGS];
    int argc;
    
    printf("간단한 터미널 (리다이렉션 지원)\n");
    printf("종료하려면 'exit'를 입력하세요.\n\n");
    
    while (1) {
        printf("myshell> ");
        fflush(stdout);
        
        // 명령어 입력받기
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        // 빈 명령어 처리
        if (strlen(command) <= 1) {
            continue;
        }
        
        // 명령어 파싱
        argc = parse_command(command, args);
        if (argc == 0) continue;
        
        // 내장 명령어 처리
        if (handle_builtin(args, argc)) {
            continue;
        }
        
        // 외부 명령어 실행
        execute_command(args, argc);
    }
    
    return 0;
}
```

## >>: 표준 출력 파일로 재지향 (추가)

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64

// 명령어를 파싱하여 인자 배열로 변환
int parse_command(char *cmd, char **args) {
    int argc = 0;
    char *token = strtok(cmd, " \t\n");
    
    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc] = token;
        argc++;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;
    return argc;
}

// 리다이렉션 처리 함수
int handle_redirection(char **args, int argc) {
    int i;
    char *output_file = NULL;
    int append_mode = 0;  // 추가 모드 플래그
    
    // '>>' 또는 '>' 기호를 찾아서 출력 파일명 확인
    for (i = 0; i < argc; i++) {
        if (strcmp(args[i], ">>") == 0) {
            if (i + 1 < argc) {
                output_file = args[i + 1];
                append_mode = 1;  // 추가 모드 설정
                args[i] = NULL;   // '>>' 기호 제거
                break;
            } else {
                printf("오류: '>>' 뒤에 파일명이 없습니다.\n");
                return -1;
            }
        } else if (strcmp(args[i], ">") == 0) {
            if (i + 1 < argc) {
                output_file = args[i + 1];
                append_mode = 0;  // 덮어쓰기 모드 설정
                args[i] = NULL;   // '>' 기호 제거
                break;
            } else {
                printf("오류: '>' 뒤에 파일명이 없습니다.\n");
                return -1;
            }
        }
    }
    
    // 리다이렉션이 있는 경우
    if (output_file != NULL) {
        int fd;
        if (append_mode) {
            // 추가 모드: 파일 끝에 추가
            fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        } else {
            // 덮어쓰기 모드: 파일 내용 덮어쓰기
            fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }
        
        if (fd == -1) {
            perror("파일 열기 오류");
            return -1;
        }
        
        // 표준 출력을 파일로 리다이렉션
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("리다이렉션 오류");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    return 0;
}

// 명령어 실행 함수
void execute_command(char **args, int argc) {
    pid_t pid = fork();
    
    if (pid == 0) {  // 자식 프로세스
        // 리다이렉션 처리
        if (handle_redirection(args, argc) == -1) {
            exit(1);
        }
        
        // 명령어 실행
        if (execvp(args[0], args) == -1) {
            perror("명령어 실행 오류");
            exit(1);
        }
    } else if (pid > 0) {  // 부모 프로세스
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork 오류");
    }
}

// 내장 명령어 처리
int handle_builtin(char **args, int argc) {
    if (argc == 0) return 0;
    
    // exit 명령어
    if (strcmp(args[0], "exit") == 0) {
        printf("터미널을 종료합니다.\n");
        exit(0);
    }
    
    // cd 명령어
    if (strcmp(args[0], "cd") == 0) {
        if (argc < 2) {
            printf("사용법: cd <디렉토리>\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("디렉토리 변경 오류");
            }
        }
        return 1;
    }
    
    return 0;  // 내장 명령어가 아님
}

int main() {
    char command[MAX_CMD_LEN];
    char *args[MAX_ARGS];
    int argc;
    
    printf("간단한 터미널 (리다이렉션 지원: > 및 >>)\n");
    printf("종료하려면 'exit'를 입력하세요.\n\n");
    
    while (1) {
        printf("myshell> ");
        fflush(stdout);
        
        // 명령어 입력받기
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        // 빈 명령어 처리
        if (strlen(command) <= 1) {
            continue;
        }
        
        // 명령어 파싱
        argc = parse_command(command, args);
        if (argc == 0) continue;
        
        // 내장 명령어 처리
        if (handle_builtin(args, argc)) {
            continue;
        }
        
        // 외부 명령어 실행
        execute_command(args, argc);
    }
    
    return 0;
}
```

## <: 표준 입력 파일로 재지향 

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64

// 명령어를 파싱하여 인자 배열로 변환
int parse_command(char *cmd, char **args) {
    int argc = 0;
    char *token = strtok(cmd, " \t\n");
    
    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc] = token;
        argc++;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;
    return argc;
}

// 리다이렉션 처리 함수
int handle_redirection(char **args, int argc) {
    int i;
    char *output_file = NULL;
    char *input_file = NULL;
    int append_mode = 0;  // 추가 모드 플래그
    
    // 리다이렉션 기호들을 찾아서 파일명 확인
    for (i = 0; i < argc; i++) {
        if (strcmp(args[i], ">>") == 0) {
            if (i + 1 < argc) {
                output_file = args[i + 1];
                append_mode = 1;  // 추가 모드 설정
                args[i] = NULL;   // '>>' 기호 제거
                args[i + 1] = NULL; // 파일명도 제거
                break;
            } else {
                printf("오류: '>>' 뒤에 파일명이 없습니다.\n");
                return -1;
            }
        } else if (strcmp(args[i], ">") == 0) {
            if (i + 1 < argc) {
                output_file = args[i + 1];
                append_mode = 0;  // 덮어쓰기 모드 설정
                args[i] = NULL;   // '>' 기호 제거
                args[i + 1] = NULL; // 파일명도 제거
                break;
            } else {
                printf("오류: '>' 뒤에 파일명이 없습니다.\n");
                return -1;
            }
        }
    }
    
    // 입력 리다이렉션 찾기
    for (i = 0; i < argc; i++) {
        if (args[i] != NULL && strcmp(args[i], "<") == 0) {
            if (i + 1 < argc) {
                input_file = args[i + 1];
                args[i] = NULL;     // '<' 기호 제거
                args[i + 1] = NULL; // 파일명도 제거
                break;
            } else {
                printf("오류: '<' 뒤에 파일명이 없습니다.\n");
                return -1;
            }
        }
    }
    
    // 입력 리다이렉션 처리
    if (input_file != NULL) {
        int fd = open(input_file, O_RDONLY);
        if (fd == -1) {
            perror("입력 파일 열기 오류");
            return -1;
        }
        
        // 표준 입력을 파일로 리다이렉션
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("입력 리다이렉션 오류");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    // 출력 리다이렉션 처리
    if (output_file != NULL) {
        int fd;
        if (append_mode) {
            // 추가 모드: 파일 끝에 추가
            fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        } else {
            // 덮어쓰기 모드: 파일 내용 덮어쓰기
            fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }
        
        if (fd == -1) {
            perror("출력 파일 열기 오류");
            return -1;
        }
        
        // 표준 출력을 파일로 리다이렉션
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("출력 리다이렉션 오류");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    return 0;
}

// 명령어 실행 함수
void execute_command(char **args, int argc) {
    pid_t pid = fork();
    
    if (pid == 0) {  // 자식 프로세스
        // 리다이렉션 처리
        if (handle_redirection(args, argc) == -1) {
            exit(1);
        }
        
        // 명령어 실행
        if (execvp(args[0], args) == -1) {
            perror("명령어 실행 오류");
            exit(1);
        }
    } else if (pid > 0) {  // 부모 프로세스
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork 오류");
    }
}

// 내장 명령어 처리
int handle_builtin(char **args, int argc) {
    if (argc == 0) return 0;
    
    // exit 명령어
    if (strcmp(args[0], "exit") == 0) {
        printf("터미널을 종료합니다.\n");
        exit(0);
    }
    
    // cd 명령어
    if (strcmp(args[0], "cd") == 0) {
        if (argc < 2) {
            printf("사용법: cd <디렉토리>\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("디렉토리 변경 오류");
            }
        }
        return 1;
    }
    
    return 0;  // 내장 명령어가 아님
}

int main() {
    char command[MAX_CMD_LEN];
    char *args[MAX_ARGS];
    int argc;
    
    printf("간단한 터미널 (리다이렉션 지원: >, >>, <)\n");
    printf("종료하려면 'exit'를 입력하세요.\n\n");
    
    while (1) {
        printf("myshell> ");
        fflush(stdout);
        
        // 명령어 입력받기
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        // 빈 명령어 처리
        if (strlen(command) <= 1) {
            continue;
        }
        
        // 명령어 파싱
        argc = parse_command(command, args);
        if (argc == 0) continue;
        
        // 내장 명령어 처리
        if (handle_builtin(args, argc)) {
            continue;
        }
        
        // 외부 명령어 실행
        execute_command(args, argc);
    }
    
    return 0;
}
```

## |: 파이프 (한 명령어의 출력을 다른 명령어의 입력으로)

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64
#define MAX_PIPES 10

// 파이프로 연결된 명령어들을 저장하는 구조체
typedef struct {
    char *args[MAX_ARGS];
    int argc;
} Command;

// 명령어를 파싱하여 인자 배열로 변환
int parse_command(char *cmd, char **args) {
    int argc = 0;
    char *token = strtok(cmd, " \t\n");
    
    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc] = token;
        argc++;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;
    return argc;
}

// 파이프가 있는지 확인하고 명령어들을 분리
int parse_pipes(char *cmd, Command *commands) {
    int cmd_count = 0;
    char *cmd_start = cmd;
    char *pipe_pos;
    
    // 파이프 기호로 명령어 분리
    while ((pipe_pos = strstr(cmd_start, "|")) != NULL && cmd_count < MAX_PIPES) {
        *pipe_pos = '\0';  // 파이프 위치를 NULL로 변경
        
        // 현재 명령어 파싱
        commands[cmd_count].argc = parse_command(cmd_start, commands[cmd_count].args);
        if (commands[cmd_count].argc > 0) {
            cmd_count++;
        }
        
        cmd_start = pipe_pos + 1;  // 다음 명령어 시작점
    }
    
    // 마지막 명령어 파싱
    if (cmd_count < MAX_PIPES) {
        commands[cmd_count].argc = parse_command(cmd_start, commands[cmd_count].args);
        if (commands[cmd_count].argc > 0) {
            cmd_count++;
        }
    }
    
    return cmd_count;
}

// 파이프 실행 함수
void execute_pipes(Command *commands, int cmd_count) {
    int pipes[MAX_PIPES - 1][2];  // 파이프 배열
    pid_t pids[MAX_PIPES];        // 프로세스 ID 배열
    int i;
    
    // 파이프 생성
    for (i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("파이프 생성 오류");
            return;
        }
    }
    
    // 각 명령어를 위한 프로세스 생성
    for (i = 0; i < cmd_count; i++) {
        pids[i] = fork();
        
        if (pids[i] == 0) {  // 자식 프로세스
            // 첫 번째 명령어가 아닌 경우, 이전 파이프에서 입력 받기
            if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                    perror("입력 파이프 연결 오류");
                    exit(1);
                }
            }
            
            // 마지막 명령어가 아닌 경우, 다음 파이프로 출력 보내기
            if (i < cmd_count - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("출력 파이프 연결 오류");
                    exit(1);
                }
            }
            
            // 모든 파이프 닫기
            int j;
            for (j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // 명령어 실행
            if (execvp(commands[i].args[0], commands[i].args) == -1) {
                perror("명령어 실행 오류");
                exit(1);
            }
        } else if (pids[i] == -1) {
            perror("fork 오류");
            return;
        }
    }
    
    // 부모 프로세스에서 모든 파이프 닫기
    for (i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // 모든 자식 프로세스 대기
    for (i = 0; i < cmd_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
}

// 리다이렉션 처리 함수
int handle_redirection(char **args, int argc) {
    int i;
    char *output_file = NULL;
    char *input_file = NULL;
    int append_mode = 0;  // 추가 모드 플래그
    
    // 리다이렉션 기호들을 찾아서 파일명 확인
    for (i = 0; i < argc; i++) {
        if (strcmp(args[i], ">>") == 0) {
            if (i + 1 < argc) {
                output_file = args[i + 1];
                append_mode = 1;  // 추가 모드 설정
                args[i] = NULL;   // '>>' 기호 제거
                args[i + 1] = NULL; // 파일명도 제거
                break;
            } else {
                printf("오류: '>>' 뒤에 파일명이 없습니다.\n");
                return -1;
            }
        } else if (strcmp(args[i], ">") == 0) {
            if (i + 1 < argc) {
                output_file = args[i + 1];
                append_mode = 0;  // 덮어쓰기 모드 설정
                args[i] = NULL;   // '>' 기호 제거
                args[i + 1] = NULL; // 파일명도 제거
                break;
            } else {
                printf("오류: '>' 뒤에 파일명이 없습니다.\n");
                return -1;
            }
        }
    }
    
    // 입력 리다이렉션 찾기
    for (i = 0; i < argc; i++) {
        if (args[i] != NULL && strcmp(args[i], "<") == 0) {
            if (i + 1 < argc) {
                input_file = args[i + 1];
                args[i] = NULL;     // '<' 기호 제거
                args[i + 1] = NULL; // 파일명도 제거
                break;
            } else {
                printf("오류: '<' 뒤에 파일명이 없습니다.\n");
                return -1;
            }
        }
    }
    
    // 입력 리다이렉션 처리
    if (input_file != NULL) {
        int fd = open(input_file, O_RDONLY);
        if (fd == -1) {
            perror("입력 파일 열기 오류");
            return -1;
        }
        
        // 표준 입력을 파일로 리다이렉션
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("입력 리다이렉션 오류");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    // 출력 리다이렉션 처리
    if (output_file != NULL) {
        int fd;
        if (append_mode) {
            // 추가 모드: 파일 끝에 추가
            fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        } else {
            // 덮어쓰기 모드: 파일 내용 덮어쓰기
            fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }
        
        if (fd == -1) {
            perror("출력 파일 열기 오류");
            return -1;
        }
        
        // 표준 출력을 파일로 리다이렉션
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("출력 리다이렉션 오류");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    return 0;
}

// 명령어 실행 함수
void execute_command(char **args, int argc) {
    pid_t pid = fork();
    
    if (pid == 0) {  // 자식 프로세스
        // 리다이렉션 처리
        if (handle_redirection(args, argc) == -1) {
            exit(1);
        }
        
        // 명령어 실행
        if (execvp(args[0], args) == -1) {
            perror("명령어 실행 오류");
            exit(1);
        }
    } else if (pid > 0) {  // 부모 프로세스
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork 오류");
    }
}

// 내장 명령어 처리
int handle_builtin(char **args, int argc) {
    if (argc == 0) return 0;
    
    // exit 명령어
    if (strcmp(args[0], "exit") == 0) {
        printf("터미널을 종료합니다.\n");
        exit(0);
    }
    
    // cd 명령어
    if (strcmp(args[0], "cd") == 0) {
        if (argc < 2) {
            printf("사용법: cd <디렉토리>\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("디렉토리 변경 오류");
            }
        }
        return 1;
    }
    
    return 0;  // 내장 명령어가 아님
}

int main() {
    char command[MAX_CMD_LEN];
    char *args[MAX_ARGS];
    Command commands[MAX_PIPES];
    int argc, cmd_count;
    
    printf("간단한 터미널 (리다이렉션 지원: >, >>, <, |)\n");
    printf("종료하려면 'exit'를 입력하세요.\n\n");
    
    while (1) {
        printf("myshell> ");
        fflush(stdout);
        
        // 명령어 입력받기
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        // 빈 명령어 처리
        if (strlen(command) <= 1) {
            continue;
        }
        
        // 파이프가 있는지 확인
        if (strstr(command, "|") != NULL) {
            // 파이프 명령어 처리
            cmd_count = parse_pipes(command, commands);
            if (cmd_count > 1) {
                execute_pipes(commands, cmd_count);
            } else if (cmd_count == 1) {
                // 파이프가 있었지만 실제로는 하나의 명령어만 있는 경우
                execute_command(commands[0].args, commands[0].argc);
            }
        } else {
            // 일반 명령어 처리
            argc = parse_command(command, args);
            if (argc == 0) continue;
            
            // 내장 명령어 처리
            if (handle_builtin(args, argc)) {
                continue;
            }
            
            // 외부 명령어 실행
            execute_command(args, argc);
        }
    }
    
    return 0;
}
```

