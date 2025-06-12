#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <sys/resource.h>
#endif

// 시간 차이를 계산하는 함수 (마이크로초 단위)
long long time_diff_microseconds(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000000LL + (end.tv_usec - start.tv_usec);
}

// 시간을 사람이 읽기 쉬운 형태로 포맷팅
void format_time(double seconds, char* buffer, size_t buffer_size) {
    if (seconds < 0.001) {
        snprintf(buffer, buffer_size, "%.3fms", seconds * 1000);
    } else if (seconds < 1.0) {
        snprintf(buffer, buffer_size, "%.3fs", seconds);
    } else if (seconds < 60.0) {
        snprintf(buffer, buffer_size, "%.3fs", seconds);
    } else {
        int minutes = (int)(seconds / 60);
        double remaining_seconds = seconds - (minutes * 60);
        snprintf(buffer, buffer_size, "%dm%.3fs", minutes, remaining_seconds);
    }
}

#ifdef _WIN32
// Windows용 명령어 실행 함수
int execute_command_windows(char* argv[], struct timeval* real_start, struct timeval* real_end,
                           double* user_time, double* sys_time) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    FILETIME creation_time, exit_time, kernel_time, user_time_ft;
    SYSTEMTIME st;
    
    // 명령어 문자열 생성
    char command_line[8192] = "";
    for (int i = 1; argv[i] != NULL; i++) {
        if (i > 1) strcat(command_line, " ");
        strcat(command_line, argv[i]);
    }
    
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));
    
    // 실행 시간 측정 시작
    gettimeofday(real_start, NULL);
    
    // 프로세스 생성
    if (!CreateProcess(NULL, command_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "time: 명령어 실행 실패: %s\n", argv[1]);
        return -1;
    }
    
    // 프로세스 완료 대기
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    // 실행 시간 측정 종료
    gettimeofday(real_end, NULL);
    
    // CPU 시간 정보 가져오기
    if (GetProcessTimes(pi.hProcess, &creation_time, &exit_time, &kernel_time, &user_time_ft)) {
        ULARGE_INTEGER user_time_ul, kernel_time_ul;
        user_time_ul.LowPart = user_time_ft.dwLowDateTime;
        user_time_ul.HighPart = user_time_ft.dwHighDateTime;
        kernel_time_ul.LowPart = kernel_time.dwLowDateTime;
        kernel_time_ul.HighPart = kernel_time.dwHighDateTime;
        
        *user_time = user_time_ul.QuadPart / 10000000.0;  // 100ns 단위를 초로 변환
        *sys_time = kernel_time_ul.QuadPart / 10000000.0;
    }
    
    // 종료 코드 가져오기
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    
    // 핸들 정리
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return (int)exit_code;
}
#endif

// Unix/Linux용 명령어 실행 함수
int execute_command_unix(char* argv[], struct timeval* real_start, struct timeval* real_end,
                        double* user_time, double* sys_time) {
    pid_t pid;
    int status;
    struct rusage usage;
    
    // 실행 시간 측정 시작
    gettimeofday(real_start, NULL);
    
    pid = fork();
    if (pid == 0) {
        // 자식 프로세스: 명령어 실행
        execvp(argv[1], &argv[1]);
        // execvp 실패 시
        fprintf(stderr, "time: %s: %s\n", argv[1], strerror(errno));
        exit(127);
    } else if (pid > 0) {
        // 부모 프로세스: 자식 프로세스 대기
        if (wait4(pid, &status, 0, &usage) == -1) {
            perror("time: wait4 실패");
            return -1;
        }
        
        // 실행 시간 측정 종료
        gettimeofday(real_end, NULL);
        
        // CPU 시간 정보 가져오기
        *user_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0;
        *sys_time = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0;
        
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "time: 명령어가 신호 %d로 종료됨\n", WTERMSIG(status));
            return 128 + WTERMSIG(status);
        }
    } else {
        // fork 실패
        perror("time: fork 실패");
        return -1;
    }
    
    return 0;
}

void print_usage() {
    printf("사용법: time 명령어 [인수...]\n");
    printf("\n");
    printf("명령어의 실행 시간을 측정합니다.\n");
    printf("\n");
    printf("출력 정보:\n");
    printf("  real    실제 경과 시간 (wall clock time)\n");
    printf("  user    사용자 모드에서 소비된 CPU 시간\n");
    printf("  sys     시스템 모드에서 소비된 CPU 시간\n");
    printf("\n");
    printf("예시:\n");
    printf("  time ls -l\n");
    printf("  time sleep 2\n");
    printf("  time gcc -o test test.c\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "time: 인수가 부족합니다\n");
        print_usage();
        return 1;
    }
    
    // 도움말 옵션 처리
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage();
        return 0;
    }
    
    struct timeval real_start, real_end;
    double user_time = 0.0, sys_time = 0.0;
    int exit_code;
    
    // 플랫폼별 명령어 실행
#ifdef _WIN32
    exit_code = execute_command_windows(argv, &real_start, &real_end, &user_time, &sys_time);
#else
    exit_code = execute_command_unix(argv, &real_start, &real_end, &user_time, &sys_time);
#endif
    
    if (exit_code == -1) {
        return 1;
    }
    
    // 실제 경과 시간 계산
    long long real_microseconds = time_diff_microseconds(real_start, real_end);
    double real_time = real_microseconds / 1000000.0;
    
    // 시간 정보를 포맷팅하여 출력
    char real_str[64], user_str[64], sys_str[64];
    format_time(real_time, real_str, sizeof(real_str));
    format_time(user_time, user_str, sizeof(user_str));
    format_time(sys_time, sys_str, sizeof(sys_str));
    
    // stderr로 시간 정보 출력 (실제 time 명령어와 동일)
    fprintf(stderr, "\nreal\t%s\n", real_str);
    fprintf(stderr, "user\t%s\n", user_str);
    fprintf(stderr, "sys\t%s\n", sys_str);
    
    // 추가 통계 정보 출력 (Unix 계열에서만)
#ifndef _WIN32
    struct rusage usage;
    if (getrusage(RUSAGE_CHILDREN, &usage) == 0) {
        if (usage.ru_maxrss > 0) {
            fprintf(stderr, "최대 메모리 사용량: %ldKB\n", usage.ru_maxrss);
        }
        if (usage.ru_minflt > 0 || usage.ru_majflt > 0) {
            fprintf(stderr, "페이지 폴트: %ld (minor), %ld (major)\n", 
                   usage.ru_minflt, usage.ru_majflt);
        }
        if (usage.ru_nvcsw > 0 || usage.ru_nivcsw > 0) {
            fprintf(stderr, "컨텍스트 스위치: %ld (voluntary), %ld (involuntary)\n",
                   usage.ru_nvcsw, usage.ru_nivcsw);
        }
    }
#endif
    
    return exit_code;
}