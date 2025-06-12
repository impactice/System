#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#define MAX_CMD_LEN 256
#define MAX_PATH_LEN 512

// 프로세스 정보 구조체
typedef struct {
    int pid;
    char user[32];
    char state;
    float cpu_percent;
    float mem_percent;
    char cmd[256];
    long vsz;  // Virtual memory size
    long rss;  // Resident set size
    char tty[16];
    char stat[16];
    char start_time[16];
} ProcessInfo;

// 숫자인지 확인하는 함수 (PID 디렉토리 구분용)
int is_number(const char *str) {
    while (*str) {
        if (*str < '0' || *str > '9') return 0;
        str++;
    // kill 명령어 처리
    else if (strcmp(args[0], "kill") == 0) {
        kill_command(argc, args);
    }
    // pgrep 명령어 (프로세스 이름으로 검색)
    else if (strcmp(args[0], "pgrep") == 0) {
        if (argc < 2) {
            printf("Usage: pgrep <process_name>\n");
        } else {
            find_processes_by_name(args[1]);
        }
    }
    return 1;
}

// /proc/pid/stat 파일에서 프로세스 정보 읽기
int read_proc_stat(int pid, ProcessInfo *proc) {
    char path[MAX_PATH_LEN];
    FILE *file;
    char comm[256];
    char state;
    int ppid, pgrp, session, tty_nr;
    unsigned long utime, stime, vsize;
    long rss;
    
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    file = fopen(path, "r");
    if (!file) return -1;
    
    // stat 파일의 주요 필드들 읽기
    fscanf(file, "%d %s %c %d %d %d %d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %*u %lu %ld",
           &proc->pid, comm, &state, &ppid, &pgrp, &session, &tty_nr,
           &utime, &stime, &vsize, &rss);
    
    proc->state = state;
    proc->vsz = vsize / 1024; // KB로 변환
    proc->rss = rss * 4; // 페이지 크기 4KB로 가정
    
    // 명령어 이름에서 괄호 제거
    if (comm[0] == '(') {
        strncpy(proc->cmd, comm + 1, sizeof(proc->cmd) - 1);
        proc->cmd[strlen(proc->cmd) - 1] = '\0'; // 마지막 ')' 제거
    } else {
        strncpy(proc->cmd, comm, sizeof(proc->cmd) - 1);
    }
    
    fclose(file);
    return 0;
}

// /proc/pid/status 파일에서 사용자 정보 읽기
int read_proc_status(int pid, ProcessInfo *proc) {
    char path[MAX_PATH_LEN];
    FILE *file;
    char line[256];
    uid_t uid = -1;
    
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    file = fopen(path, "r");
    if (!file) return -1;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line, "Uid:\t%d", &uid);
            break;
        }
    }
    fclose(file);
    
    if (uid != -1) {
        struct passwd *pw = getpwuid(uid);
        if (pw) {
            strncpy(proc->user, pw->pw_name, sizeof(proc->user) - 1);
        } else {
            snprintf(proc->user, sizeof(proc->user), "%d", uid);
        }
    } else {
        strcpy(proc->user, "unknown");
    }
    
    return 0;
}

// 메모리 사용률 계산
float calculate_mem_percent(long rss) {
    FILE *file = fopen("/proc/meminfo", "r");
    if (!file) return 0.0;
    
    char line[256];
    long total_mem = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line, "MemTotal: %ld kB", &total_mem);
            break;
        }
    }
    fclose(file);
    
    if (total_mem > 0) {
        return ((float)rss / total_mem) * 100.0;
    }
    return 0.0;
}

// kill 명령어 구현
void kill_command(int argc, char **args) {
    if (argc < 2) {
        printf("Usage: kill [signal] <PID>\n");
        printf("       kill <PID>          (sends SIGTERM by default)\n");
        printf("       kill -SIGTERM <PID> (sends SIGTERM - normal termination)\n");
        printf("       kill -SIGKILL <PID> (sends SIGKILL - force kill)\n");
        printf("       kill -9 <PID>       (sends SIGKILL - force kill)\n");
        printf("       kill -15 <PID>      (sends SIGTERM - normal termination)\n");
        return;
    }
    
    int signal_num = SIGTERM; // 기본값은 SIGTERM
    int pid;
    int pid_index = 1; // PID가 있는 인덱스
    
    // 시그널 옵션 파싱
    if (argc >= 3 && args[1][0] == '-') {
        char *signal_str = args[1] + 1; // '-' 제거
        
        if (strcmp(signal_str, "SIGTERM") == 0 || strcmp(signal_str, "15") == 0) {
            signal_num = SIGTERM;
        }
        else if (strcmp(signal_str, "SIGKILL") == 0 || strcmp(signal_str, "9") == 0) {
            signal_num = SIGKILL;
        }
        else if (strcmp(signal_str, "SIGINT") == 0 || strcmp(signal_str, "2") == 0) {
            signal_num = SIGINT;
        }
        else if (strcmp(signal_str, "SIGHUP") == 0 || strcmp(signal_str, "1") == 0) {
            signal_num = SIGHUP;
        }
        else {
            // 숫자로 직접 시그널 지정
            int sig = atoi(signal_str);
            if (sig > 0 && sig < 32) {
                signal_num = sig;
            } else {
                printf("kill: invalid signal specification '%s'\n", signal_str);
                return;
            }
        }
        pid_index = 2;
    }
    
    // PID 파싱
    if (pid_index >= argc) {
        printf("kill: missing PID\n");
        return;
    }
    
    pid = atoi(args[pid_index]);
    if (pid <= 0) {
        printf("kill: invalid PID '%s'\n", args[pid_index]);
        return;
    }
    
    // 프로세스가 존재하는지 확인
    char proc_path[MAX_PATH_LEN];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d", pid);
    
    struct stat st;
    if (stat(proc_path, &st) != 0) {
        printf("kill: (%d) - No such process\n", pid);
        return;
    }
    
    // 시그널 전송
    if (kill(pid, signal_num) == 0) {
        char *signal_name;
        switch (signal_num) {
            case SIGTERM: signal_name = "SIGTERM"; break;
            case SIGKILL: signal_name = "SIGKILL"; break;
            case SIGINT:  signal_name = "SIGINT"; break;
            case SIGHUP:  signal_name = "SIGHUP"; break;
            default:      signal_name = "signal"; break;
        }
        
        if (signal_num == SIGKILL) {
            printf("Process %d force killed with %s\n", pid, signal_name);
        } else {
            printf("Process %d sent %s signal\n", pid, signal_name);
        }
    } else {
        switch (errno) {
            case ESRCH:
                printf("kill: (%d) - No such process\n", pid);
                break;
            case EPERM:
                printf("kill: (%d) - Operation not permitted\n", pid);
                break;
            default:
                printf("kill: (%d) - %s\n", pid, strerror(errno));
                break;
        }
    }
}

// 프로세스 이름으로 PID 찾기 (killall 기능을 위한 헬퍼 함수)
void find_processes_by_name(const char *name) {
    DIR *proc_dir;
    struct dirent *entry;
    ProcessInfo proc;
    int found = 0;
    
    proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("opendir /proc");
        return;
    }
    
    printf("Found processes matching '%s':\n", name);
    printf("  PID USER     COMMAND\n");
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (!is_number(entry->d_name)) continue;
        
        int pid = atoi(entry->d_name);
        if (get_process_info(pid, &proc) == 0) {
            if (strstr(proc.cmd, name) != NULL) {
                printf("%5d %-8s %s\n", proc.pid, proc.user, proc.cmd);
                found = 1;
            }
        }
    }
    
    if (!found) {
        printf("No processes found matching '%s'\n", name);
    }
    
    closedir(proc_dir);
}
int get_process_info(int pid, ProcessInfo *proc) {
    if (read_proc_stat(pid, proc) != 0) return -1;
    if (read_proc_status(pid, proc) != 0) return -1;
    
    proc->cpu_percent = 0.0; // 간단한 구현에서는 0으로 설정
    proc->mem_percent = calculate_mem_percent(proc->rss);
    strcpy(proc->tty, "?"); // 간단한 구현에서는 ? 로 설정
    snprintf(proc->stat, sizeof(proc->stat), "%c", proc->state);
    strcpy(proc->start_time, "00:00"); // 간단한 구현에서는 기본값
    
    return 0;
}

// ps 명령어 구현
void ps_command(int show_all) {
    DIR *proc_dir;
    struct dirent *entry;
    ProcessInfo proc;
    
    proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("opendir /proc");
        return;
    }
    
    if (show_all) {
        printf("USER       PID %%CPU %%MEM    VSZ   RSS TTY      STAT START   TIME COMMAND\n");
    } else {
        printf("  PID TTY          TIME CMD\n");
    }
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (!is_number(entry->d_name)) continue;
        
        int pid = atoi(entry->d_name);
        if (get_process_info(pid, &proc) == 0) {
            if (show_all) {
                printf("%-8s %5d %4.1f %4.1f %6ld %5ld %-8s %-4s %5s %7s %s\n",
                       proc.user, proc.pid, proc.cpu_percent, proc.mem_percent,
                       proc.vsz, proc.rss, proc.tty, proc.stat, 
                       proc.start_time, "00:00:00", proc.cmd);
            } else {
                printf("%5d %-12s %8s %s\n", proc.pid, proc.tty, "00:00:00", proc.cmd);
            }
        }
    }
    
    closedir(proc_dir);
}

// 명령어 파싱 및 실행
void execute_command(char *cmd) {
    char *token;
    char *args[10];
    int argc = 0;
    
    // 명령어를 공백으로 분리
    token = strtok(cmd, " \t\n");
    while (token != NULL && argc < 9) {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;
    
    if (argc == 0) return;
    
    // ps 명령어 처리
    if (strcmp(args[0], "ps") == 0) {
        int show_all = 0;
        
        // 옵션 확인
        for (int i = 1; i < argc; i++) {
            if (strcmp(args[i], "-aux") == 0 || strcmp(args[i], "aux") == 0) {
                show_all = 1;
                break;
            }
        }
        
        ps_command(show_all);
    }
    // exit 명령어
    else if (strcmp(args[0], "exit") == 0) {
        printf("Goodbye!\n");
        exit(0);
    }
    // help 명령어
    else if (strcmp(args[0], "help") == 0) {
        printf("Available commands:\n");
        printf("  ps              - Show running processes\n");
        printf("  ps -aux         - Show all processes with detailed info\n");
        printf("  kill <PID>      - Send SIGTERM to process (normal termination)\n");
        printf("  kill -SIGTERM <PID> - Send SIGTERM to process\n");
        printf("  kill -SIGKILL <PID> - Send SIGKILL to process (force kill)\n");
        printf("  kill -9 <PID>   - Send SIGKILL to process (force kill)\n");
        printf("  kill -15 <PID>  - Send SIGTERM to process\n");
        printf("  pgrep <name>    - Find processes by name\n");
        printf("  help            - Show this help message\n");
        printf("  exit            - Exit the terminal\n");
    }
    // 알 수 없는 명령어
    else {
        printf("Unknown command: %s\n", args[0]);
        printf("Type 'help' for available commands.\n");
    }
}

int main() {
    char input[MAX_CMD_LEN];
    
    printf("Simple Terminal with PS and KILL Commands\n");
    printf("Type 'help' for available commands, 'exit' to quit.\n");
    printf("Warning: Use kill commands carefully!\n\n");
    
    while (1) {
        printf("simple-shell$ ");
        fflush(stdout);
        
        // 사용자 입력 받기
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // 빈 줄 처리
        if (strlen(input) <= 1) continue;
        
        // 명령어 실행
        execute_command(input);
    }
    
    return 0;
}