#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>

// 사용법 출력 함수
void print_usage(const char* program_name) {
    printf("사용법: %s [옵션] 그룹명\n", program_name);
    printf("또는: %s [옵션] 사용자명 그룹명\n", program_name);
    printf("옵션:\n");
    printf("  -f, --force          강제 삭제 (확인 없이)\n");
    printf("  --only-if-empty      그룹이 비어있을 때만 삭제\n");
    printf("  -h, --help           도움말 출력\n");
    printf("\n사용 모드:\n");
    printf("  1. 그룹 삭제: %s 그룹명\n", program_name);
    printf("  2. 사용자를 그룹에서 제거: %s 사용자명 그룹명\n", program_name);
    printf("\n예시:\n");
    printf("  %s mygroup           # mygroup 그룹 삭제\n", program_name);
    printf("  %s user1 mygroup     # user1을 mygroup에서 제거\n", program_name);
    printf("  %s -f mygroup        # 강제로 mygroup 삭제\n", program_name);
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

// 그룹 존재 확인 함수
struct group* check_group_exists(const char* groupname) {
    return getgrnam(groupname);
}

// 사용자 존재 확인 함수
struct passwd* check_user_exists(const char* username) {
    return getpwnam(username);
}

// 그룹이 다른 사용자의 주 그룹인지 확인
int is_primary_group_for_users(gid_t gid) {
    struct passwd* pwd;
    setpwent();
    
    while ((pwd = getpwent()) != NULL) {
        if (pwd->pw_gid == gid) {
            endpwent();
            return 1; // 주 그룹으로 사용되고 있음
        }
    }
    
    endpwent();
    return 0; // 주 그룹으로 사용되지 않음
}

// 그룹 멤버 목록 출력
void print_group_members(struct group* grp) {
    if (grp->gr_mem && grp->gr_mem[0]) {
        printf("그룹 멤버: ");
        for (int i = 0; grp->gr_mem[i]; i++) {
            printf("%s", grp->gr_mem[i]);
            if (grp->gr_mem[i + 1]) {
                printf(", ");
            }
        }
        printf("\n");
    } else {
        printf("그룹 멤버: 없음\n");
    }
}

// 사용자가 그룹의 멤버인지 확인
int is_user_in_group(const char* username, struct group* grp) {
    if (!grp->gr_mem) return 0;
    
    for (int i = 0; grp->gr_mem[i]; i++) {
        if (strcmp(grp->gr_mem[i], username) == 0) {
            return 1;
        }
    }
    return 0;
}

// 그룹 삭제 확인 함수
int confirm_group_deletion(const char* groupname, struct group* grp, int only_if_empty) {
    char response[10];
    
    printf("그룹 정보:\n");
    printf("  그룹명: %s\n", grp->gr_name);
    printf("  GID: %d\n", grp->gr_gid);
    print_group_members(grp);
    
    // 주 그룹 확인
    if (is_primary_group_for_users(grp->gr_gid)) {
        printf("  경고: 이 그룹은 일부 사용자의 주 그룹입니다!\n");
    }
    
    // only-if-empty 옵션 확인
    if (only_if_empty) {
        if (grp->gr_mem && grp->gr_mem[0]) {
            fprintf(stderr, "오류: 그룹이 비어있지 않습니다. (--only-if-empty 옵션)\n");
            return 0;
        }
        if (is_primary_group_for_users(grp->gr_gid)) {
            fprintf(stderr, "오류: 그룹이 사용자의 주 그룹으로 사용되고 있습니다.\n");
            return 0;
        }
    }
    
    printf("\n그룹 '%s'를 삭제하시겠습니까? [y/N]: ", groupname);
    
    if (fgets(response, sizeof(response), stdin) == NULL) {
        return 0;
    }
    
    return (response[0] == 'y' || response[0] == 'Y');
}

// /etc/group 파일에서 그룹 제거
int remove_group_from_file(const char* groupname) {
    FILE* group_file;
    char temp_file[] = "/tmp/group_temp_XXXXXX";
    FILE* temp_group;
    char line[1024];
    int temp_fd;
    int found = 0;
    
    // 임시 파일 생성
    temp_fd = mkstemp(temp_file);
    if (temp_fd == -1) {
        perror("임시 파일 생성 실패");
        return 0;
    }
    close(temp_fd);
    
    // /etc/group 파일 처리
    group_file = fopen("/etc/group", "r");
    temp_group = fopen(temp_file, "w");
    
    if (group_file == NULL || temp_group == NULL) {
        perror("group 파일 열기 실패");
        if (group_file) fclose(group_file);
        if (temp_group) fclose(temp_group);
        unlink(temp_file);
        return 0;
    }
    
    while (fgets(line, sizeof(line), group_file)) {
        char* line_copy = strdup(line);
        char* current_group = strtok(line_copy, ":");
        
        if (current_group && strcmp(current_group, groupname) == 0) {
            found = 1;
            printf("group 파일에서 그룹 제거: %s\n", groupname);
        } else {
            fputs(line, temp_group);
        }
        
        free(line_copy);
    }
    
    fclose(group_file);
    fclose(temp_group);
    
    if (!found) {
        fprintf(stderr, "경고: group 파일에서 그룹을 찾을 수 없습니다.\n");
        unlink(temp_file);
        return 0;
    }
    
    // 파일 권한 설정
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

// /etc/gshadow 파일에서 그룹 제거
int remove_group_from_gshadow(const char* groupname) {
    FILE* gshadow_file;
    char temp_file[] = "/tmp/gshadow_temp_XXXXXX";
    FILE* temp_gshadow;
    char line[1024];
    int temp_fd;
    int found = 0;
    
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
    
    while (fgets(line, sizeof(line), gshadow_file)) {
        char* line_copy = strdup(line);
        char* current_group = strtok(line_copy, ":");
        
        if (current_group && strcmp(current_group, groupname) == 0) {
            found = 1;
            printf("gshadow 파일에서 그룹 제거: %s\n", groupname);
        } else {
            fputs(line, temp_gshadow);
        }
        
        free(line_copy);
    }
    
    fclose(gshadow_file);
    fclose(temp_gshadow);
    
    if (!found) {
        printf("gshadow 파일에 그룹이 없습니다.\n");
        unlink(temp_file);
        return 1; // 없어도 성공으로 처리
    }
    
    // 파일 권한 설정
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

// 모든 그룹에서 사용자 제거
int remove_user_from_all_groups(const char* username) {
    FILE* group_file;
    char temp_file[] = "/tmp/group_temp_XXXXXX";
    FILE* temp_group;
    char line[1024];
    int temp_fd;
    int modified = 0;
    
    // 임시 파일 생성
    temp_fd = mkstemp(temp_file);
    if (temp_fd == -1) {
        perror("임시 파일 생성 실패");
        return 0;
    }
    close(temp_fd);
    
    group_file = fopen("/etc/group", "r");
    temp_group = fopen(temp_file, "w");
    
    if (group_file == NULL || temp_group == NULL) {
        perror("group 파일 열기 실패");
        if (group_file) fclose(group_file);
        if (temp_group) fclose(temp_group);
        unlink(temp_file);
        return 0;
    }
    
    while (fgets(line, sizeof(line), group_file)) {
        char* line_copy = strdup(line);
        char* group_name = strtok(line_copy, ":");
        char* password = strtok(NULL, ":");
        char* gid_str = strtok(NULL, ":");
        char* members = strtok(NULL, ":");
        
        if (group_name && password && gid_str) {
            // 멤버 목록에서 사용자 제거
            if (members && strlen(members) > 0) {
                char new_members[1024] = "";
                char* member = strtok(members, ",");
                int first = 1;
                
                while (member) {
                    // 공백 제거
                    while (*member == ' ') member++;
                    char* end = member + strlen(member) - 1;
                    while (end > member && (*end == ' ' || *end == '\n' || *end == '\r')) {
                        *end = '\0';
                        end--;
                    }
                    
                    if (strcmp(member, username) != 0) {
                        if (!first) {
                            strcat(new_members, ",");
                        }
                        strcat(new_members, member);
                        first = 0;
                    } else {
                        modified = 1;
                        printf("그룹 %s에서 사용자 %s 제거\n", group_name, username);
                    }
                    member = strtok(NULL, ",");
                }
                
                fprintf(temp_group, "%s:%s:%s:%s\n", group_name, password, gid_str, new_members);
            } else {
                fprintf(temp_group, "%s:%s:%s:\n", group_name, password, gid_str);
            }
        }
        
        free(line_copy);
    }
    
    fclose(group_file);
    fclose(temp_group);
    
    if (!modified) {
        unlink(temp_file);
        return 1; // 변경사항 없어도 성공
    }
    
    // 파일 권한 설정
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

// 특정 그룹에서 사용자 제거
int remove_user_from_group(const char* username, const char* groupname) {
    struct group* grp = check_group_exists(groupname);
    if (!grp) {
        fprintf(stderr, "오류: 그룹 '%s'가 존재하지 않습니다.\n", groupname);
        return 0;
    }
    
    struct passwd* pwd = check_user_exists(username);
    if (!pwd) {
        fprintf(stderr, "오류: 사용자 '%s'가 존재하지 않습니다.\n", username);
        return 0;
    }
    
    // 사용자가 그룹의 멤버인지 확인
    if (!is_user_in_group(username, grp)) {
        printf("사용자 '%s'는 그룹 '%s'의 멤버가 아닙니다.\n", username, groupname);
        return 1;
    }
    
    // 주 그룹인지 확인
    if (pwd->pw_gid == grp->gr_gid) {
        fprintf(stderr, "오류: '%s'는 사용자 '%s'의 주 그룹입니다. 주 그룹에서는 제거할 수 없습니다.\n", 
                groupname, username);
        return 0;
    }
    
    // 시스템 명령어 시도
    char command[256];
    snprintf(command, sizeof(command), "gpasswd -d %s %s", username, groupname);
    printf("시스템 명령어 실행: %s\n", command);
    
    int result = system(command);
    if (result == 0) {
        printf("사용자 '%s'가 그룹 '%s'에서 제거되었습니다.\n", username, groupname);
        return 1;
    }
    
    // 시스템 명령어 실패 시 수동 처리
    printf("시스템 명령어 실패, 수동 처리를 시도합니다...\n");
    return remove_user_from_all_groups(username);
}

// 시스템 명령어를 사용한 그룹 삭제
int delete_group_system_command(const char* groupname) {
    char command[256];
    snprintf(command, sizeof(command), "groupdel %s", groupname);
    
    printf("시스템 명령어 실행: %s\n", command);
    int result = system(command);
    
    if (result == 0) {
        printf("그룹 '%s'가 성공적으로 삭제되었습니다.\n", groupname);
        return 1;
    } else {
        fprintf(stderr, "그룹 삭제 중 오류가 발생했습니다.\n");
        return 0;
    }
}

// 수동 그룹 삭제
int delete_group_manual(const char* groupname) {
    // /etc/group에서 제거
    if (!remove_group_from_file(groupname)) {
        return 0;
    }
    
    // /etc/gshadow에서 제거
    if (!remove_group_from_gshadow(groupname)) {
        fprintf(stderr, "경고: gshadow 파일 업데이트에 실패했습니다.\n");
    }
    
    printf("그룹 '%s'가 성공적으로 삭제되었습니다.\n", groupname);
    return 1;
}

// delgroup 명령어 구현
int delgroup_command(int argc, char* argv[]) {
    int force = 0;
    int only_if_empty = 0;
    char* username = NULL;
    char* groupname = NULL;
    
    // 인자 파싱
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            force = 1;
        } else if (strcmp(argv[i], "--only-if-empty") == 0) {
            only_if_empty = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            if (username == NULL) {
                username = argv[i];
            } else if (groupname == NULL) {
                groupname = argv[i];
            } else {
                fprintf(stderr, "오류: 너무 많은 인자입니다.\n");
                print_usage(argv[0]);
                return 1;
            }
        } else {
            fprintf(stderr, "알 수 없는 옵션: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // 인자 검증
    if (username == NULL) {
        fprintf(stderr, "오류: 그룹명 또는 사용자명을 입력해주세요.\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // 관리자 권한 확인
    if (!check_admin_privileges()) {
        return 1;
    }
    
    // 모드 결정: 사용자를 그룹에서 제거 vs 그룹 삭제
    if (groupname != NULL) {
        // 사용자를 그룹에서 제거
        return remove_user_from_group(username, groupname) ? 0 : 1;
    } else {
        // 그룹 삭제
        groupname = username;
        
        // 그룹 존재 확인
        struct group* grp = check_group_exists(groupname);
        if (!grp) {
            fprintf(stderr, "오류: 그룹 '%s'가 존재하지 않습니다.\n", groupname);
            return 1;
        }
        
        // 시스템 그룹 보호
        if (grp->gr_gid < 100) {
            fprintf(stderr, "오류: 시스템 그룹 '%s' (GID: %d)는 삭제할 수 없습니다.\n", 
                    groupname, grp->gr_gid);
            return 1;
        }
        
        // root 그룹 보호
        if (strcmp(groupname, "root") == 0) {
            fprintf(stderr, "오류: root 그룹은 삭제할 수 없습니다.\n");
            return 1;
        }
        
        // 확인 (force 옵션이 없는 경우)
        if (!force) {
            if (!confirm_group_deletion(groupname, grp, only_if_empty)) {
                printf("그룹 삭제가 취소되었습니다.\n");
                return 0;
            }
        } else if (only_if_empty) {
            // force와 only-if-empty가 함께 사용된 경우에도 빈 그룹 확인
            if ((grp->gr_mem && grp->gr_mem[0]) || is_primary_group_for_users(grp->gr_gid)) {
                fprintf(stderr, "오류: 그룹이 비어있지 않습니다. (--only-if-empty 옵션)\n");
                return 1;
            }
        }
        
        // 그룹 삭제 시도 (시스템 명령어 우선)
        if (delete_group_system_command(groupname)) {
            return 0;
        }
        
        // 시스템 명령어 실패 시 수동 삭제 시도
        printf("시스템 명령어 실패, 수동 삭제를 시도합니다...\n");
        if (delete_group_manual(groupname)) {
            return 0;
        }
        
        fprintf(stderr, "그룹 삭제에 실패했습니다.\n");
        return 1;
    }
}

int main(int argc, char* argv[]) {
    return delgroup_command(argc, argv);
}