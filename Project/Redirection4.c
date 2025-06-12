#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64
#define MAX_PIPES 10

// 파이프로 연결된 명령어들을 저장하는 구조체
typedef struct {
    char *args[MAX_ARGS];
    int argc;
} Command;

// 명령어를 파싱하여 인자 배열로 변환
int parse_command(char *cmd, char **args) {
    int argc = 0;
    char *token = strtok(cmd, " \t\n");
    
    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc] = token;
        argc++;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;
    return argc;
}

// 파이프가 있는지 확인하고 명령어들을 분리
int parse_pipes(char *cmd, Command *commands) {
    int cmd_count = 0;
    char *cmd_start = cmd;
    char *pipe_pos;
    
    // 파이프 기호로 명령어 분리
    while ((pipe_pos = strstr(cmd_start, "|")) != NULL && cmd_count < MAX_PIPES) {
        *pipe_pos = '\0';  // 파이프 위치를 NULL로 변경
        
        // 현재 명령어 파싱
        commands[cmd_count].argc = parse_command(cmd_start, commands[cmd_count].args);
        if (commands[cmd_count].argc > 0) {
            cmd_count++;
        }
        
        cmd_start = pipe_pos + 1;  // 다음 명령어 시작점
    }
    
    // 마지막 명령어 파싱
    if (cmd_count < MAX_PIPES) {
        commands[cmd_count].argc = parse_command(cmd_start, commands[cmd_count].args);
        if (commands[cmd_count].argc > 0) {
            cmd_count++;
        }
    }
    
    return cmd_count;
}

// 파이프 실행 함수
void execute_pipes(Command *commands, int cmd_count) {
    int pipes[MAX_PIPES - 1][2];  // 파이프 배열
    pid_t pids[MAX_PIPES];        // 프로세스 ID 배열
    int i;
    
    // 파이프 생성
    for (i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("파이프 생성 오류");
            return;
        }
    }
    
    // 각 명령어를 위한 프로세스 생성
    for (i = 0; i < cmd_count; i++) {
        pids[i] = fork();
        
        if (pids[i] == 0) {  // 자식 프로세스
            // 첫 번째 명령어가 아닌 경우, 이전 파이프에서 입력 받기
            if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                    perror("입력 파이프 연결 오류");
                    exit(1);
                }
            }
            
            // 마지막 명령어가 아닌 경우, 다음 파이프로 출력 보내기
            if (i < cmd_count - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("출력 파이프 연결 오류");
                    exit(1);
                }
            }
            
            // 모든 파이프 닫기
            int j;
            for (j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // 명령어 실행
            if (execvp(commands[i].args[0], commands[i].args) == -1) {
                perror("명령어 실행 오류");
                exit(1);
            }
        } else if (pids[i] == -1) {
            perror("fork 오류");
            return;
        }
    }
    
    // 부모 프로세스에서 모든 파이프 닫기
    for (i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // 모든 자식 프로세스 대기
    for (i = 0; i < cmd_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
}

// 리다이렉션 처리 함수
int handle_redirection(char **args, int argc) {
    int i;
    char *output_file = NULL;
    char *input_file = NULL;
    int append_mode = 0;  // 추가 모드 플래그
    
    // 리다이렉션 기호들을 찾아서 파일명 확인
    for (i = 0; i < argc; i++) {
        if (strcmp(args[i], ">>") == 0) {
            if (i + 1 < argc) {
                output_file = args[i + 1];
                append_mode = 1;  // 추가 모드 설정
                args[i] = NULL;   // '>>' 기호 제거
                args[i + 1] = NULL; // 파일명도 제거
                break;
            } else {
                printf("오류: '>>' 뒤에 파일명이 없습니다.\n");
                return -1;
            }
        } else if (strcmp(args[i], ">") == 0) {
            if (i + 1 < argc) {
                output_file = args[i + 1];
                append_mode = 0;  // 덮어쓰기 모드 설정
                args[i] = NULL;   // '>' 기호 제거
                args[i + 1] = NULL; // 파일명도 제거
                break;
            } else {
                printf("오류: '>' 뒤에 파일명이 없습니다.\n");
                return -1;
            }
        }
    }
    
    // 입력 리다이렉션 찾기
    for (i = 0; i < argc; i++) {
        if (args[i] != NULL && strcmp(args[i], "<") == 0) {
            if (i + 1 < argc) {
                input_file = args[i + 1];
                args[i] = NULL;     // '<' 기호 제거
                args[i + 1] = NULL; // 파일명도 제거
                break;
            } else {
                printf("오류: '<' 뒤에 파일명이 없습니다.\n");
                return -1;
            }
        }
    }
    
    // 입력 리다이렉션 처리
    if (input_file != NULL) {
        int fd = open(input_file, O_RDONLY);
        if (fd == -1) {
            perror("입력 파일 열기 오류");
            return -1;
        }
        
        // 표준 입력을 파일로 리다이렉션
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("입력 리다이렉션 오류");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    // 출력 리다이렉션 처리
    if (output_file != NULL) {
        int fd;
        if (append_mode) {
            // 추가 모드: 파일 끝에 추가
            fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        } else {
            // 덮어쓰기 모드: 파일 내용 덮어쓰기
            fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }
        
        if (fd == -1) {
            perror("출력 파일 열기 오류");
            return -1;
        }
        
        // 표준 출력을 파일로 리다이렉션
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("출력 리다이렉션 오류");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    return 0;
}

// 명령어 실행 함수
void execute_command(char **args, int argc) {
    pid_t pid = fork();
    
    if (pid == 0) {  // 자식 프로세스
        // 리다이렉션 처리
        if (handle_redirection(args, argc) == -1) {
            exit(1);
        }
        
        // 명령어 실행
        if (execvp(args[0], args) == -1) {
            perror("명령어 실행 오류");
            exit(1);
        }
    } else if (pid > 0) {  // 부모 프로세스
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork 오류");
    }
}

// 내장 명령어 처리
int handle_builtin(char **args, int argc) {
    if (argc == 0) return 0;
    
    // exit 명령어
    if (strcmp(args[0], "exit") == 0) {
        printf("터미널을 종료합니다.\n");
        exit(0);
    }
    
    // cd 명령어
    if (strcmp(args[0], "cd") == 0) {
        if (argc < 2) {
            printf("사용법: cd <디렉토리>\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("디렉토리 변경 오류");
            }
        }
        return 1;
    }
    
    return 0;  // 내장 명령어가 아님
}

int main() {
    char command[MAX_CMD_LEN];
    char *args[MAX_ARGS];
    Command commands[MAX_PIPES];
    int argc, cmd_count;
    
    printf("간단한 터미널 (리다이렉션 지원: >, >>, <, |)\n");
    printf("종료하려면 'exit'를 입력하세요.\n\n");
    
    while (1) {
        printf("myshell> ");
        fflush(stdout);
        
        // 명령어 입력받기
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        // 빈 명령어 처리
        if (strlen(command) <= 1) {
            continue;
        }
        
        // 파이프가 있는지 확인
        if (strstr(command, "|") != NULL) {
            // 파이프 명령어 처리
            cmd_count = parse_pipes(command, commands);
            if (cmd_count > 1) {
                execute_pipes(commands, cmd_count);
            } else if (cmd_count == 1) {
                // 파이프가 있었지만 실제로는 하나의 명령어만 있는 경우
                execute_command(commands[0].args, commands[0].argc);
            }
        } else {
            // 일반 명령어 처리
            argc = parse_command(command, args);
            if (argc == 0) continue;
            
            // 내장 명령어 처리
            if (handle_builtin(args, argc)) {
                continue;
            }
            
            // 외부 명령어 실행
            execute_command(args, argc);
        }
    }
    
    return 0;
}