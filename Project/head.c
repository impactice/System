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