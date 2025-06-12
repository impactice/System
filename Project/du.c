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