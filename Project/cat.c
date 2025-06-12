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