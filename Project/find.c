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