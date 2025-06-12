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