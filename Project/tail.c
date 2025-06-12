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