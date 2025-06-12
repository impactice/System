#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <utime.h>

// 파일이 존재하는지 확인하는 함수
int file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

// 새 파일을 생성하는 함수
int create_file(const char *filename) {
    int fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        perror("touch: 파일 생성 실패");
        return -1;
    }
    close(fd);
    return 0;
}

// 파일의 접근/수정 시간을 현재 시간으로 업데이트하는 함수
int update_file_time(const char *filename) {
    if (utime(filename, NULL) == -1) {
        perror("touch: 시간 업데이트 실패");
        return -1;
    }
    return 0;
}

// 파일의 상태 정보를 출력하는 함수 (테스트용)
void show_file_info(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        printf("파일 '%s'의 정보를 가져올 수 없습니다\n", filename);
        return;
    }
    
    printf("파일: %s\n", filename);
    printf("  크기: %ld 바이트\n", st.st_size);
    printf("  접근 시간: %s", ctime(&st.st_atime));
    printf("  수정 시간: %s", ctime(&st.st_mtime));
    printf("  상태 변경 시간: %s", ctime(&st.st_ctime));
}

// 단일 파일 처리 함수
int touch_file(const char *filename) {
    if (file_exists(filename)) {
        // 파일이 존재하면 시간 업데이트
        if (update_file_time(filename) == 0) {
            printf("파일 '%s'의 시간을 업데이트했습니다\n", filename);
            return 0;
        }
    } else {
        // 파일이 존재하지 않으면 새로 생성
        if (create_file(filename) == 0) {
            printf("새 파일 '%s'을 생성했습니다\n", filename);
            return 0;
        }
    }
    return -1;
}

// 시간 문자열을 파싱하는 함수 (간단한 형태만 지원: YYYYMMDDHHMM)
int parse_time_string(const char *time_str, struct tm *tm_time) {
    if (strlen(time_str) != 12) {
        return -1;
    }
    
    // YYYYMMDDHHMM 형식 파싱
    char year[5], month[3], day[3], hour[3], minute[3];
    
    strncpy(year, time_str, 4); year[4] = '\0';
    strncpy(month, time_str + 4, 2); month[2] = '\0';
    strncpy(day, time_str + 6, 2); day[2] = '\0';
    strncpy(hour, time_str + 8, 2); hour[2] = '\0';
    strncpy(minute, time_str + 10, 2); minute[2] = '\0';
    
    tm_time->tm_year = atoi(year) - 1900;
    tm_time->tm_mon = atoi(month) - 1;
    tm_time->tm_mday = atoi(day);
    tm_time->tm_hour = atoi(hour);
    tm_time->tm_min = atoi(minute);
    tm_time->tm_sec = 0;
    tm_time->tm_isdst = -1;
    
    return 0;
}

// 지정된 시간으로 파일 시간을 설정하는 함수
int set_file_time(const char *filename, const char *time_str) {
    struct tm tm_time;
    if (parse_time_string(time_str, &tm_time) == -1) {
        printf("touch: 잘못된 시간 형식입니다 (YYYYMMDDHHMM 형식을 사용하세요)\n");
        return -1;
    }
    
    time_t new_time = mktime(&tm_time);
    if (new_time == -1) {
        printf("touch: 유효하지 않은 시간입니다\n");
        return -1;
    }
    
    struct utimbuf times;
    times.actime = new_time;
    times.modtime = new_time;
    
    if (utime(filename, &times) == -1) {
        perror("touch: 시간 설정 실패");
        return -1;
    }
    
    return 0;
}

// touch 명령어 구현
int cmd_touch(int argc, char *argv[]) {
    int c_flag = 0;  // -c: 파일이 없어도 생성하지 않음
    int t_flag = 0;  // -t: 특정 시간으로 설정
    char *time_str = NULL;
    int success_count = 0;
    int total_count = 0;
    
    // 인수가 부족한 경우
    if (argc < 2) {
        printf("사용법: touch [-c] [-t YYYYMMDDHHMM] <파일명> [파일명...]\n");
        printf("  -c: 파일이 존재하지 않아도 생성하지 않음\n");
        printf("  -t: 지정된 시간으로 설정 (YYYYMMDDHHMM 형식)\n");
        return -1;
    }
    
    // 옵션 파싱
    int i;
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            c_flag = 1;
        } else if (strcmp(argv[i], "-t") == 0) {
            if (i + 1 >= argc) {
                printf("touch: -t 옵션에는 시간 인수가 필요합니다\n");
                return -1;
            }
            t_flag = 1;
            time_str = argv[++i];
        } else {
            printf("touch: 알 수 없는 옵션 '%s'\n", argv[i]);
            printf("사용법: touch [-c] [-t YYYYMMDDHHMM] <파일명> [파일명...]\n");
            return -1;
        }
    }
    
    // 파일명이 제공되지 않은 경우
    if (i >= argc) {
        printf("touch: 파일명을 입력해주세요\n");
        return -1;
    }
    
    // 각 파일 처리
    for (; i < argc; i++) {
        total_count++;
        
        if (c_flag && !file_exists(argv[i])) {
            // -c 옵션: 파일이 없으면 생성하지 않음
            printf("touch: '%s' 파일이 존재하지 않습니다 (-c 옵션)\n", argv[i]);
            continue;
        }
        
        // 파일이 존재하지 않으면 생성
        if (!file_exists(argv[i])) {
            if (create_file(argv[i]) == -1) {
                printf("touch: '%s' 파일 생성 실패\n", argv[i]);
                continue;
            }
            printf("새 파일 '%s'을 생성했습니다\n", argv[i]);
        }
        
        // 시간 설정
        if (t_flag) {
            // 지정된 시간으로 설정
            if (set_file_time(argv[i], time_str) == 0) {
                printf("파일 '%s'의 시간을 설정했습니다\n", argv[i]);
                success_count++;
            }
        } else {
            // 현재 시간으로 업데이트
            if (update_file_time(argv[i]) == 0) {
                printf("파일 '%s'의 시간을 업데이트했습니다\n", argv[i]);
                success_count++;
            }
        }
    }
    
    // 결과 요약
    if (total_count > 1) {
        printf("\n총 %d개 파일 중 %d개 처리 완료\n", total_count, success_count);
    }
    
    return (success_count == total_count) ? 0 : -1;
}

// 테스트용 메인 함수
int main(int argc, char *argv[]) {
    printf("=== touch 명령어 테스트 ===\n");
    
    // 명령행 인수가 있으면 그대로 실행
    if (argc > 1) {
        return cmd_touch(argc, argv);
    }
    
    printf("\n1. 새 파일 생성 테스트:\n");
    char *test1[] = {"touch", "test_new_file.txt"};
    cmd_touch(2, test1);
    
    printf("\n파일 정보 확인:\n");
    show_file_info("test_new_file.txt");
    
    printf("\n2. 기존 파일 시간 업데이트 테스트:\n");
    printf("2초 대기 후 시간 업데이트...\n");
    sleep(2);
    char *test2[] = {"touch", "test_new_file.txt"};
    cmd_touch(2, test2);
    
    printf("\n업데이트된 파일 정보:\n");
    show_file_info("test_new_file.txt");
    
    printf("\n3. 여러 파일 생성 테스트:\n");
    char *test3[] = {"touch", "file1.txt", "file2.txt", "file3.txt"};
    cmd_touch(4, test3);
    
    printf("\n4. -c 옵션 테스트 (존재하지 않는 파일):\n");
    char *test4[] = {"touch", "-c", "nonexistent.txt"};
    cmd_touch(3, test4);
    
    printf("\n5. -t 옵션 테스트 (특정 시간 설정):\n");
    char *test5[] = {"touch", "-t", "202312251430", "test_new_file.txt"};
    cmd_touch(4, test5);
    
    printf("\n시간 설정 후 파일 정보:\n");
    show_file_info("test_new_file.txt");
    
    printf("\n6. 잘못된 사용법 테스트:\n");
    char *test6[] = {"touch"};
    cmd_touch(1, test6);
    
    // 정리
    printf("\n테스트 파일 정리 중...\n");
    system("rm -f test_new_file.txt file1.txt file2.txt file3.txt");
    
    return 0;
}