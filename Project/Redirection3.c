#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64

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
    int argc;
    
    printf("간단한 터미널 (리다이렉션 지원: >, >>, <)\n");
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
        
        // 명령어 파싱
        argc = parse_command(command, args);
        if (argc == 0) continue;
        
        // 내장 명령어 처리
        if (handle_builtin(args, argc)) {
            continue;
        }
        
        // 외부 명령어 실행
        execute_command(args, argc);
    }
    
    return 0;
}