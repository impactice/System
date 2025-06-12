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
#include <sys/sysinfo.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>

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
    unsigned long utime;  // user time
    unsigned long stime;  // system time
    unsigned long total_time; // total time
} ProcessInfo;

// 시스템 정보 구조체
typedef struct {
    long total_mem;
    long free_mem;
    long used_mem;
    long cached_mem;
    float load_avg[3];
    int num_processes;
    int num_running;
    int num_sleeping;
    int num_zombie;
    unsigned long total_cpu_time;
    unsigned long idle_cpu_time;
} SystemInfo;

// 숫자인지 확인하는 함수 (PID 디렉토리 구분용)
int is_number(const char *str) {
    while (*str) {
        if (*str < '0' || *str > '9') return 0;
        str++;
    // kill 명령어 처리
    else if (strcmp(args[0], "kill") == 0) {
        kill_command(argc, args);
    // top 명령어 처리
    else if (strcmp(args[0], "top") == 0) {
        top_command();
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
    proc->utime = utime;
    proc->stime = stime;
    proc->total_time = utime + stime;
    
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

// 시스템 정보 읽기
int get_system_info(SystemInfo *sys_info) {
    FILE *file;
    char line[256];
    
    // 메모리 정보 읽기
    file = fopen("/proc/meminfo", "r");
    if (!file) return -1;
    
    sys_info->total_mem = 0;
    sys_info->free_mem = 0;
    sys_info->cached_mem = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line, "MemTotal: %ld kB", &sys_info->total_mem);
        } else if (strncmp(line, "MemFree:", 8) == 0) {
            sscanf(line, "MemFree: %ld kB", &sys_info->free_mem);
        } else if (strncmp(line, "Cached:", 7) == 0) {
            sscanf(line, "Cached: %ld kB", &sys_info->cached_mem);
        }
    }
    fclose(file);
    
    sys_info->used_mem = sys_info->total_mem - sys_info->free_mem - sys_info->cached_mem;
    
    // Load average 읽기
    file = fopen("/proc/loadavg", "r");
    if (file) {
        fscanf(file, "%f %f %f", &sys_info->load_avg[0], &sys_info->load_avg[1], &sys_info->load_avg[2]);
        fclose(file);
    }
    
    // CPU 정보 읽기
    file = fopen("/proc/stat", "r");
    if (file) {
        fgets(line, sizeof(line), file);
        unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
        sscanf(line, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
        
        sys_info->total_cpu_time = user + nice + system + idle + iowait + irq + softirq + steal;
        sys_info->idle_cpu_time = idle + iowait;
        fclose(file);
    }
    
    return 0;
}

// 프로세스 수 계산
void count_processes(SystemInfo *sys_info) {
    DIR *proc_dir;
    struct dirent *entry;
    ProcessInfo proc;
    
    sys_info->num_processes = 0;
    sys_info->num_running = 0;
    sys_info->num_sleeping = 0;
    sys_info->num_zombie = 0;
    
    proc_dir = opendir("/proc");
    if (!proc_dir) return;
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (!is_number(entry->d_name)) continue;
        
        int pid = atoi(entry->d_name);
        if (read_proc_stat(pid, &proc) == 0) {
            sys_info->num_processes++;
            switch (proc.state) {
                case 'R': sys_info->num_running++; break;
                case 'S': case 'D': sys_info->num_sleeping++; break;
                case 'Z': sys_info->num_zombie++; break;
            }
        }
    }
    
    closedir(proc_dir);
}

// 터미널 설정 변경 (non-blocking input)
void set_terminal_mode(int enable) {
    static struct termios old_termios;
    static int is_set = 0;
    
    if (enable && !is_set) {
        tcgetattr(STDIN_FILENO, &old_termios);
        struct termios new_termios = old_termios;
        new_termios.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
        is_set = 1;
    } else if (!enable && is_set) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
        fcntl(STDIN_FILENO, F_SETFL, 0);
        is_set = 0;
    }
}

// 화면 지우기
void clear_screen() {
    printf("\033[2J\033[H");
    fflush(stdout);
}

// CPU 사용률 계산을 위한 이전 값들 저장
static unsigned long prev_total_time[1000] = {0};
static unsigned long prev_cpu_total = 0;
static unsigned long prev_cpu_idle = 0;

// CPU 사용률 계산
float calculate_cpu_usage(ProcessInfo *proc, unsigned long total_cpu_time, unsigned long idle_cpu_time) {
    static int first_call = 1;
    
    if (first_call) {
        prev_total_time[proc->pid % 1000] = proc->total_time;
        prev_cpu_total = total_cpu_time;
        prev_cpu_idle = idle_cpu_time;
        first_call = 0;
        return 0.0;
    }
    
    unsigned long proc_time_diff = proc->total_time - prev_total_time[proc->pid % 1000];
    unsigned long cpu_time_diff = total_cpu_time - prev_cpu_total;
    
    prev_total_time[proc->pid % 1000] = proc->total_time;
    
    if (cpu_time_diff > 0) {
        return ((float)proc_time_diff / cpu_time_diff) * 100.0;
    }
    return 0.0;
}

// top 명령어 구현
void top_command() {
    SystemInfo sys_info;
    ProcessInfo processes[100];  // 최대 100개 프로세스 표시
    int process_count = 0;
    
    set_terminal_mode(1);  // non-blocking 모드 활성화
    
    printf("Press 'q' to quit top\n");
    sleep(1);
    
    while (1) {
        clear_screen();
        
        // 시스템 정보 수집
        get_system_info(&sys_info);
        count_processes(&sys_info);
        
        // 헤더 정보 출력
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        printf("top - %02d:%02d:%02d up time, load average: %.2f, %.2f, %.2f\n",
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
               sys_info.load_avg[0], sys_info.load_avg[1], sys_info.load_avg[2]);
        
        printf("Tasks: %d total, %d running, %d sleeping, %d zombie\n",
               sys_info.num_processes, sys_info.num_running, 
               sys_info.num_sleeping, sys_info.num_zombie);
        
        float cpu_usage = 100.0 - ((float)sys_info.idle_cpu_time / sys_info.total_cpu_time * 100.0);
        printf("%%Cpu(s): %.1f us, %.1f sy, %.1f id\n", cpu_usage * 0.7, cpu_usage * 0.3, 100.0 - cpu_usage);
        
        printf("KiB Mem: %8ld total, %8ld free, %8ld used, %8ld buff/cache\n",
               sys_info.total_mem, sys_info.free_mem, sys_info.used_mem, sys_info.cached_mem);
        
        printf("\n");
        printf("  PID USER      PR  NI    VIRT    RES    SHR S  %%CPU %%MEM     TIME+ COMMAND\n");
        
        // 프로세스 정보 수집
        DIR *proc_dir = opendir("/proc");
        if (proc_dir) {
            struct dirent *entry;
            process_count = 0;
            
            while ((entry = readdir(proc_dir)) != NULL && process_count < 100) {
                if (!is_number(entry->d_name)) continue;
                
                int pid = atoi(entry->d_name);
                ProcessInfo *proc = &processes[process_count];
                
                if (get_process_info(pid, proc) == 0) {
                    proc->cpu_percent = calculate_cpu_usage(proc, sys_info.total_cpu_time, sys_info.idle_cpu_time);
                    process_count++;
                }
            }
            closedir(proc_dir);
        }
        
        // CPU 사용률 기준으로 정렬 (간단한 버블 정렬)
        for (int i = 0; i < process_count - 1; i++) {
            for (int j = 0; j < process_count - i - 1; j++) {
                if (processes[j].cpu_percent < processes[j + 1].cpu_percent) {
                    ProcessInfo temp = processes[j];
                    processes[j] = processes[j + 1];
                    processes[j + 1] = temp;
                }
            }
        }
        
        // 프로세스 목록 출력 (상위 20개)
        int display_count = (process_count > 20) ? 20 : process_count;
        for (int i = 0; i < display_count; i++) {
            ProcessInfo *proc = &processes[i];
            printf("%5d %-8s 20   0 %7ld %6ld      0 %c %5.1f %4.1f %8s %s\n",
                   proc->pid, proc->user, proc->vsz, proc->rss,
                   proc->state, proc->cpu_percent, proc->mem_percent,
                   "00:00:00", proc->cmd);
        }
        
        printf("\nPress 'q' to quit, any other key to refresh...");
        fflush(stdout);
        
        // 키 입력 확인 (non-blocking)
        char ch;
        if (read(STDIN_FILENO, &ch, 1) > 0) {
            if (ch == 'q' || ch == 'Q') {
                break;
            }
        }
        
        sleep(2);  // 2초마다 갱신
    }
    
    set_terminal_mode(0);  // 터미널 모드 복원
    clear_screen();
    printf("Exited top mode.\n");
}
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
    
    printf("Simple Terminal with PS, KILL, and TOP Commands\n");
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