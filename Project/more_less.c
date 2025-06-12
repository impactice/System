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