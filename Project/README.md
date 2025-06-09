

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
