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