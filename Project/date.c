#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pwd.h>
#endif

// 관리자 권한 확인 함수
int is_admin() {
#ifdef _WIN32
    // Windows에서 관리자 권한 확인
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
#else
    // Unix/Linux에서 root 권한 확인
    return (getuid() == 0);
#endif
}

// 날짜 문자열 파싱 함수
int parse_date_string(const char* date_str, struct tm* tm_info) {
    memset(tm_info, 0, sizeof(struct tm));
    
    // 다양한 날짜 형식 지원
    // 형식 1: "YYYY-MM-DD HH:MM:SS"
    if (sscanf(date_str, "%d-%d-%d %d:%d:%d", 
               &tm_info->tm_year, &tm_info->tm_mon, &tm_info->tm_mday,
               &tm_info->tm_hour, &tm_info->tm_min, &tm_info->tm_sec) == 6) {
        tm_info->tm_year -= 1900;  // tm_year은 1900년부터의 년수
        tm_info->tm_mon -= 1;      // tm_mon은 0부터 시작 (0=1월)
        return 1;
    }
    
    // 형식 2: "MM/DD/YYYY HH:MM:SS"
    if (sscanf(date_str, "%d/%d/%d %d:%d:%d",
               &tm_info->tm_mon, &tm_info->tm_mday, &tm_info->tm_year,
               &tm_info->tm_hour, &tm_info->tm_min, &tm_info->tm_sec) == 6) {
        tm_info->tm_year -= 1900;
        tm_info->tm_mon -= 1;
        return 1;
    }
    
    // 형식 3: "YYYY-MM-DD" (시간은 00:00:00으로 설정)
    if (sscanf(date_str, "%d-%d-%d", 
               &tm_info->tm_year, &tm_info->tm_mon, &tm_info->tm_mday) == 3) {
        tm_info->tm_year -= 1900;
        tm_info->tm_mon -= 1;
        return 1;
    }
    
    // 형식 4: "HH:MM:SS" (오늘 날짜에 시간만 변경)
    if (sscanf(date_str, "%d:%d:%d", 
               &tm_info->tm_hour, &tm_info->tm_min, &tm_info->tm_sec) == 3) {
        time_t now = time(NULL);
        struct tm* current = localtime(&now);
        tm_info->tm_year = current->tm_year;
        tm_info->tm_mon = current->tm_mon;
        tm_info->tm_mday = current->tm_mday;
        return 1;
    }
    
    return 0; // 파싱 실패
}

// 시스템 시간 설정 함수
int set_system_time(struct tm* tm_info) {
#ifdef _WIN32
    SYSTEMTIME st = {0};
    st.wYear = tm_info->tm_year + 1900;
    st.wMonth = tm_info->tm_mon + 1;
    st.wDay = tm_info->tm_mday;
    st.wHour = tm_info->tm_hour;
    st.wMinute = tm_info->tm_min;
    st.wSecond = tm_info->tm_sec;
    
    return SetSystemTime(&st) ? 0 : -1;
#else
    struct timeval tv;
    time_t new_time = mktime(tm_info);
    
    if (new_time == -1) {
        return -1;
    }
    
    tv.tv_sec = new_time;
    tv.tv_usec = 0;
    
    return settimeofday(&tv, NULL);
#endif
}

void print_usage() {
    printf("사용법: date [옵션]\n");
    printf("옵션:\n");
    printf("  (옵션 없음)    현재 날짜 및 시간 출력\n");
    printf("  -s STRING      날짜/시간 설정 (관리자 권한 필요)\n");
    printf("\n");
    printf("날짜 형식 예시:\n");
    printf("  \"2024-12-25 15:30:00\"  (YYYY-MM-DD HH:MM:SS)\n");
    printf("  \"12/25/2024 15:30:00\"  (MM/DD/YYYY HH:MM:SS)\n");
    printf("  \"2024-12-25\"           (YYYY-MM-DD, 시간은 00:00:00)\n");
    printf("  \"15:30:00\"             (HH:MM:SS, 오늘 날짜)\n");
}

int main(int argc, char* argv[]) {
    // 인수가 없으면 현재 시간 출력
    if (argc == 1) {
        time_t current_time = time(NULL);
        char* time_str = ctime(&current_time);
        
        // ctime()의 결과에서 개행문자 제거
        if (time_str) {
            size_t len = strlen(time_str);
            if (len > 0 && time_str[len-1] == '\n') {
                time_str[len-1] = '\0';
            }
            printf("%s\n", time_str);
        }
        return 0;
    }
    
    // -s 옵션 처리
    if (argc >= 3 && strcmp(argv[1], "-s") == 0) {
        // 관리자 권한 확인
        if (!is_admin()) {
            fprintf(stderr, "오류: 시간 설정을 위해서는 관리자 권한이 필요합니다.\n");
#ifdef _WIN32
            fprintf(stderr, "관리자 권한으로 프로그램을 실행해주세요.\n");
#else
            fprintf(stderr, "sudo를 사용하여 실행해주세요.\n");
#endif
            return 1;
        }
        
        const char* date_string = argv[2];
        struct tm tm_info;
        
        // 날짜 문자열 파싱
        if (!parse_date_string(date_string, &tm_info)) {
            fprintf(stderr, "오류: 잘못된 날짜 형식입니다.\n");
            print_usage();
            return 1;
        }
        
        // 파싱된 날짜 정보 검증
        if (tm_info.tm_year < 70 || tm_info.tm_year > 200 ||  // 1970~2100년 범위
            tm_info.tm_mon < 0 || tm_info.tm_mon > 11 ||       // 0~11월
            tm_info.tm_mday < 1 || tm_info.tm_mday > 31 ||     // 1~31일
            tm_info.tm_hour < 0 || tm_info.tm_hour > 23 ||     // 0~23시
            tm_info.tm_min < 0 || tm_info.tm_min > 59 ||       // 0~59분
            tm_info.tm_sec < 0 || tm_info.tm_sec > 59) {       // 0~59초
            fprintf(stderr, "오류: 날짜/시간 값이 유효하지 않습니다.\n");
            return 1;
        }
        
        // 시스템 시간 설정
        if (set_system_time(&tm_info) != 0) {
            perror("시간 설정 실패");
            return 1;
        }
        
        // 설정된 시간 출력
        char buffer[100];
        strftime(buffer, sizeof(buffer), "%Y년 %m월 %d일 %H:%M:%S", &tm_info);
        printf("시간이 설정되었습니다: %s\n", buffer);
        
        return 0;
    }
    
    // 잘못된 옵션
    fprintf(stderr, "오류: 알 수 없는 옵션입니다.\n");
    print_usage();
    return 1;
}