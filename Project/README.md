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





