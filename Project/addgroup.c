#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>

// 사용법 출력 함수
void print_usage(const char* program_name) {
    printf("사용법: %s [옵션] 그룹명\n", program_name);
    printf("옵션:\n");
    printf("  -g, --gid GID        그룹 ID 지정\n");
    printf("  -r, --system         시스템 그룹으로 생성\n");
    printf("  -f, --force          그룹이 이미 존재해도 성공으로 처리\n");
    printf("  -h, --help           도움말 출력\n");
    printf("\n예시:\n");
    printf("  %s mygroup           # mygroup 그룹 생성\n", program_name);
    printf("  %s -g 1500 mygroup   # GID 1500으로 mygroup 생성\n", program_name);
    printf("  %s -r sysgroup       # 시스템 그룹 sysgroup 생성\n", program_name);
}

// 관리자 권한 확인 함수
int check_admin_privileges() {
    if (getuid() != 0) {
        fprintf(stderr, "오류: 이 명령어는 관리자 권한이 필요합니다.\n");
        fprintf(stderr, "sudo를 사용하여 실행하세요.\n");
        return 0;
    }
    return 1;
}

// 그룹 이름 유효성 검사 함수
int validate_groupname(const char* groupname) {
    int len = strlen(groupname);
    
    // 길이 체크 (최대 32자)
    if (len == 0 || len > 32) {
        fprintf(stderr, "오류: 그룹명은 1-32자 사이여야 합니다.\n");
        return 0;
    }
    
    // 첫 문자는 알파벳 또는 _
    if (!isalpha(groupname[0]) && groupname[0] != '_') {
        fprintf(stderr, "오류: 그룹명은 알파벳 또는 '_'로 시작해야 합니다.\n");
        return 0;
    }
    
    // 나머지 문자는 알파벳, 숫자, _, - 만 허용
    for (int i = 1; i < len; i++) {
        if (!isalnum(groupname[i]) && groupname[i] != '_' && groupname[i] != '-') {
            fprintf(stderr, "오류: 그룹명에는 알파벳, 숫자, '_', '-'만 사용할 수 있습니다.\n");
            return 0;
        }
    }
    
    // 예약된 이름 확인
    if (strcmp(groupname, "root") == 0 || 
        strcmp(groupname, "daemon") == 0 ||
        strcmp(groupname, "sys") == 0) {
        fprintf(stderr, "오류: '%s'는 예약된 그룹명입니다.\n", groupname);
        return 0;
    }
    
    return 1;
}

// 그룹 존재 확인 함수
int check_group_exists(const char* groupname) {
    struct group* grp = getgrnam(groupname);
    return (grp != NULL);
}

// GID 존재 확인 함수
int check_gid_exists(gid_t gid) {
    struct group* grp = getgrgid(gid);
    return (grp != NULL);
}

// 다음 사용 가능한 GID 찾기 함수
gid_t find_next_available_gid(int is_system) {
    gid_t start_gid, end_gid;
    
    if (is_system) {
        start_gid = 100;  // 시스템 그룹 시작 GID
        end_gid = 999;    // 시스템 그룹 끝 GID
    } else {
        start_gid = 1000; // 일반 그룹 시작 GID
        end_gid = 60000;  // 일반 그룹 끝 GID
    }
    
    for (gid_t gid = start_gid; gid <= end_gid; gid++) {
        if (!check_gid_exists(gid)) {
            return gid;
        }
    }
    
    return (gid_t)-1; // 사용 가능한 GID 없음
}

// /etc/group 파일에 그룹 추가 함수
int add_group_to_file(const char* groupname, gid_t gid) {
    FILE* group_file;
    char temp_file[] = "/tmp/group_temp_XXXXXX";
    FILE* temp_group;
    char line[1024];
    int temp_fd;
    int found_insert_pos = 0;
    
    // 임시 파일 생성
    temp_fd = mkstemp(temp_file);
    if (temp_fd == -1) {
        perror("임시 파일 생성 실패");
        return 0;
    }
    close(temp_fd);
    
    // /etc/group 파일 읽기
    group_file = fopen("/etc/group", "r");
    temp_group = fopen(temp_file, "w");
    
    if (group_file == NULL || temp_group == NULL) {
        perror("group 파일 열기 실패");
        if (group_file) fclose(group_file);
        if (temp_group) fclose(temp_group);
        unlink(temp_file);
        return 0;
    }
    
    // 기존 내용 복사하면서 적절한 위치에 새 그룹 삽입
    while (fgets(line, sizeof(line), group_file)) {
        char* line_copy = strdup(line);
        char* current_group = strtok(line_copy, ":");
        char* gid_str = strtok(NULL, ":");
        
        if (current_group && gid_str) {
            gid_t current_gid = atoi(gid_str);
            
            // GID 순서로 정렬하여 삽입
            if (!found_insert_pos && current_gid > gid) {
                fprintf(temp_group, "%s:x:%d:\n", groupname, gid);
                found_insert_pos = 1;
            }
        }
        
        fputs(line, temp_group);
        free(line_copy);
    }
    
    // 파일 끝에 도달했는데 아직 삽입하지 않은 경우
    if (!found_insert_pos) {
        fprintf(temp_group, "%s:x:%d:\n", groupname, gid);
    }
    
    fclose(group_file);
    fclose(temp_group);
    
    // 파일 권한 설정 (644)
    if (chmod(temp_file, 0644) != 0) {
        perror("파일 권한 설정 실패");
        unlink(temp_file);
        return 0;
    }
    
    // 원본 파일 교체
    if (rename(temp_file, "/etc/group") != 0) {
        perror("group 파일 교체 실패");
        unlink(temp_file);
        return 0;
    }
    
    return 1;
}

// /etc/gshadow 파일에 그룹 추가 함수
int add_group_to_gshadow(const char* groupname) {
    FILE* gshadow_file;
    char temp_file[] = "/tmp/gshadow_temp_XXXXXX";
    FILE* temp_gshadow;
    char line[1024];
    int temp_fd;
    
    // gshadow 파일이 없으면 건너뛰기
    if (access("/etc/gshadow", F_OK) != 0) {
        return 1; // gshadow 없어도 성공으로 처리
    }
    
    // 임시 파일 생성
    temp_fd = mkstemp(temp_file);
    if (temp_fd == -1) {
        perror("gshadow 임시 파일 생성 실패");
        return 0;
    }
    close(temp_fd);
    
    // /etc/gshadow 파일 처리
    gshadow_file = fopen("/etc/gshadow", "r");
    temp_gshadow = fopen(temp_file, "w");
    
    if (gshadow_file == NULL || temp_gshadow == NULL) {
        perror("gshadow 파일 열기 실패");
        if (gshadow_file) fclose(gshadow_file);
        if (temp_gshadow) fclose(temp_gshadow);
        unlink(temp_file);
        return 0;
    }
    
    // 기존 내용 복사
    while (fgets(line, sizeof(line), gshadow_file)) {
        fputs(line, temp_gshadow);
    }
    
    // 새 그룹 추가
    fprintf(temp_gshadow, "%s:!::\n", groupname);
    
    fclose(gshadow_file);
    fclose(temp_gshadow);
    
    // 파일 권한 설정 (640)
    if (chmod(temp_file, 0640) != 0) {
        perror("gshadow 파일 권한 설정 실패");
        unlink(temp_file);
        return 0;
    }
    
    // 원본 파일 교체
    if (rename(temp_file, "/etc/gshadow") != 0) {
        perror("gshadow 파일 교체 실패");
        unlink(temp_file);
        return 0;
    }
    
    return 1;
}

// 시스템 명령어를 사용한 그룹 추가 함수
int add_group_system_command(const char* groupname, gid_t gid, int is_system) {
    char command[256];
    int result;
    
    if (gid != (gid_t)-1) {
        if (is_system) {
            snprintf(command, sizeof(command), "groupadd -r -g %d %s", gid, groupname);
        } else {
            snprintf(command, sizeof(command), "groupadd -g %d %s", gid, groupname);
        }
    } else {
        if (is_system) {
            snprintf(command, sizeof(command), "groupadd -r %s", groupname);
        } else {
            snprintf(command, sizeof(command), "groupadd %s", groupname);
        }
    }
    
    printf("시스템 명령어 실행: %s\n", command);
    result = system(command);
    
    if (result == 0) {
        printf("그룹 '%s'가 성공적으로 생성되었습니다.\n", groupname);
        return 1;
    } else {
        fprintf(stderr, "그룹 생성 중 오류가 발생했습니다.\n");
        return 0;
    }
}

// 수동 그룹 추가 함수
int add_group_manual(const char* groupname, gid_t gid, int is_system) {
    gid_t final_gid;
    
    if (gid != (gid_t)-1) {
        // 지정된 GID 사용
        if (check_gid_exists(gid)) {
            fprintf(stderr, "오류: GID %d는 이미 사용 중입니다.\n", gid);
            return 0;
        }
        final_gid = gid;
    } else {
        // 자동으로 GID 할당
        final_gid = find_next_available_gid(is_system);
        if (final_gid == (gid_t)-1) {
            fprintf(stderr, "오류: 사용 가능한 GID를 찾을 수 없습니다.\n");
            return 0;
        }
    }
    
    printf("그룹 '%s'를 GID %d로 생성합니다.\n", groupname, final_gid);
    
    // /etc/group에 추가
    if (!add_group_to_file(groupname, final_gid)) {
        return 0;
    }
    
    // /etc/gshadow에 추가 (선택적)
    if (!add_group_to_gshadow(groupname)) {
        fprintf(stderr, "경고: gshadow 파일 업데이트에 실패했습니다.\n");
    }
    
    printf("그룹 '%s' (GID: %d)가 성공적으로 생성되었습니다.\n", groupname, final_gid);
    return 1;
}

// addgroup 명령어 구현
int addgroup_command(int argc, char* argv[]) {
    gid_t gid = (gid_t)-1;
    int is_system = 0;
    int force = 0;
    char* groupname = NULL;
    
    // 인자 파싱
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--gid") == 0) {
            if (i + 1 < argc) {
                gid = atoi(argv[++i]);
                if (gid <= 0) {
                    fprintf(stderr, "오류: 유효하지 않은 GID입니다.\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "오류: -g 옵션에 GID가 필요합니다.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--system") == 0) {
            is_system = 1;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            force = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            groupname = argv[i];
        } else {
            fprintf(stderr, "알 수 없는 옵션: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // 그룹명 확인
    if (groupname == NULL) {
        fprintf(stderr, "오류: 그룹명을 입력해주세요.\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // 관리자 권한 확인
    if (!check_admin_privileges()) {
        return 1;
    }
    
    // 그룹명 유효성 확인
    if (!validate_groupname(groupname)) {
        return 1;
    }
    
    // 그룹 존재 확인
    if (check_group_exists(groupname)) {
        if (force) {
            printf("그룹 '%s'가 이미 존재합니다. (force 옵션으로 무시)\n", groupname);
            return 0;
        } else {
            fprintf(stderr, "오류: 그룹 '%s'가 이미 존재합니다.\n", groupname);
            return 1;
        }
    }
    
    // 시스템 그룹인데 일반 GID 범위 지정한 경우 확인
    if (is_system && gid != (gid_t)-1 && gid >= 1000) {
        fprintf(stderr, "경고: 시스템 그룹에 일반 사용자 GID 범위를 지정했습니다.\n");
    }
    
    // 그룹 생성 시도 (시스템 명령어 우선)
    if (add_group_system_command(groupname, gid, is_system)) {
        return 0;
    }
    
    // 시스템 명령어 실패 시 수동 생성 시도
    printf("시스템 명령어 실패, 수동 생성을 시도합니다...\n");
    if (add_group_manual(groupname, gid, is_system)) {
        return 0;
    }
    
    fprintf(stderr, "그룹 생성에 실패했습니다.\n");
    return 1;
}

int main(int argc, char* argv[]) {
    return addgroup_command(argc, argv);
}