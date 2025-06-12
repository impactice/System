#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

// 사용법 출력 함수
void print_usage(const char* program_name) {
    printf("사용법: %s [옵션] 사용자명\n", program_name);
    printf("옵션:\n");
    printf("  -r, --remove-home    홈 디렉터리도 함께 삭제\n");
    printf("  -f, --force          강제 삭제 (확인 없이)\n");
    printf("  -h, --help           도움말 출력\n");
    printf("\n예시:\n");
    printf("  %s testuser          # testuser 삭제\n", program_name);
    printf("  %s -r testuser       # testuser와 홈 디렉터리 삭제\n", program_name);
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

// 사용자 존재 확인 함수
int check_user_exists(const char* username) {
    struct passwd* pwd = getpwnam(username);
    return (pwd != NULL);
}

// 디렉터리 재귀적 삭제 함수
int remove_directory_recursive(const char* path) {
    DIR* dir;
    struct dirent* entry;
    struct stat statbuf;
    char full_path[1024];
    
    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        // . 과 .. 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (lstat(full_path, &statbuf) == -1) {
            perror("lstat");
            continue;
        }
        
        if (S_ISDIR(statbuf.st_mode)) {
            // 디렉터리인 경우 재귀적으로 삭제
            remove_directory_recursive(full_path);
        } else {
            // 파일인 경우 삭제
            if (unlink(full_path) == -1) {
                fprintf(stderr, "파일 삭제 실패: %s (%s)\n", full_path, strerror(errno));
            } else {
                printf("파일 삭제됨: %s\n", full_path);
            }
        }
    }
    
    closedir(dir);
    
    // 빈 디렉터리 삭제
    if (rmdir(path) == -1) {
        fprintf(stderr, "디렉터리 삭제 실패: %s (%s)\n", path, strerror(errno));
        return -1;
    } else {
        printf("디렉터리 삭제됨: %s\n", path);
    }
    
    return 0;
}

// 사용자 확인 함수
int confirm_deletion(const char* username, int remove_home) {
    char response[10];
    
    printf("사용자 '%s'를 삭제하시겠습니까?", username);
    if (remove_home) {
        printf(" (홈 디렉터리 포함)");
    }
    printf(" [y/N]: ");
    
    if (fgets(response, sizeof(response), stdin) == NULL) {
        return 0;
    }
    
    return (response[0] == 'y' || response[0] == 'Y');
}

// 시스템 사용자 삭제 함수 (userdel 명령어 사용)
int delete_system_user(const char* username, int remove_home) {
    char command[256];
    int result;
    
    if (remove_home) {
        snprintf(command, sizeof(command), "userdel -r %s", username);
    } else {
        snprintf(command, sizeof(command), "userdel %s", username);
    }
    
    printf("시스템 명령어 실행: %s\n", command);
    result = system(command);
    
    if (result == 0) {
        printf("사용자 '%s'가 성공적으로 삭제되었습니다.\n", username);
        return 1;
    } else {
        fprintf(stderr, "사용자 삭제 중 오류가 발생했습니다.\n");
        return 0;
    }
}

// 수동 사용자 삭제 함수 (passwd, shadow 파일 수정)
int delete_user_manual(const char* username, int remove_home) {
    struct passwd* pwd;
    char temp_file[] = "/tmp/passwd_temp_XXXXXX";
    char temp_shadow[] = "/tmp/shadow_temp_XXXXXX";
    FILE* passwd_file, *shadow_file;
    FILE* temp_passwd, *temp_shadow;
    char line[1024];
    char* user_field;
    int found = 0;
    
    // 사용자 정보 가져오기
    pwd = getpwnam(username);
    if (pwd == NULL) {
        fprintf(stderr, "사용자 '%s'를 찾을 수 없습니다.\n", username);
        return 0;
    }
    
    // 임시 파일 생성
    int temp_fd = mkstemp(temp_file);
    if (temp_fd == -1) {
        perror("임시 파일 생성 실패");
        return 0;
    }
    close(temp_fd);
    
    int temp_shadow_fd = mkstemp(temp_shadow);
    if (temp_shadow_fd == -1) {
        perror("임시 shadow 파일 생성 실패");
        unlink(temp_file);
        return 0;
    }
    close(temp_shadow_fd);
    
    // /etc/passwd 파일 수정
    passwd_file = fopen("/etc/passwd", "r");
    temp_passwd = fopen(temp_file, "w");
    
    if (passwd_file == NULL || temp_passwd == NULL) {
        perror("passwd 파일 열기 실패");
        if (passwd_file) fclose(passwd_file);
        if (temp_passwd) fclose(temp_passwd);
        unlink(temp_file);
        unlink(temp_shadow);
        return 0;
    }
    
    while (fgets(line, sizeof(line), passwd_file)) {
        user_field = strtok(line, ":");
        if (user_field != NULL && strcmp(user_field, username) == 0) {
            found = 1;
            printf("passwd에서 사용자 제거: %s\n", username);
        } else {
            fputs(line, temp_passwd);
            // strtok로 인해 변경된 줄을 복원하기 위해 원본 다시 읽기
            fseek(passwd_file, ftell(passwd_file) - strlen(line), SEEK_SET);
            fgets(line, sizeof(line), passwd_file);
            fputs(line, temp_passwd);
        }
    }
    
    fclose(passwd_file);
    fclose(temp_passwd);
    
    if (!found) {
        fprintf(stderr, "passwd 파일에서 사용자를 찾을 수 없습니다.\n");
        unlink(temp_file);
        unlink(temp_shadow);
        return 0;
    }
    
    // /etc/shadow 파일 수정
    shadow_file = fopen("/etc/shadow", "r");
    temp_shadow = fopen(temp_shadow, "w");
    
    if (shadow_file != NULL && temp_shadow != NULL) {
        while (fgets(line, sizeof(line), shadow_file)) {
            user_field = strtok(line, ":");
            if (user_field == NULL || strcmp(user_field, username) != 0) {
                // strtok로 인해 변경된 줄을 복원
                fseek(shadow_file, ftell(shadow_file) - strlen(line), SEEK_SET);
                fgets(line, sizeof(line), shadow_file);
                fputs(line, temp_shadow);
            } else {
                printf("shadow에서 사용자 제거: %s\n", username);
            }
        }
        fclose(shadow_file);
        fclose(temp_shadow);
        
        // shadow 파일 교체
        if (rename(temp_shadow, "/etc/shadow") != 0) {
            perror("shadow 파일 교체 실패");
        }
    } else {
        if (shadow_file) fclose(shadow_file);
        if (temp_shadow) fclose(temp_shadow);
        unlink(temp_shadow);
    }
    
    // passwd 파일 교체
    if (rename(temp_file, "/etc/passwd") != 0) {
        perror("passwd 파일 교체 실패");
        unlink(temp_file);
        return 0;
    }
    
    // 홈 디렉터리 삭제
    if (remove_home && pwd->pw_dir != NULL) {
        printf("홈 디렉터리 삭제 중: %s\n", pwd->pw_dir);
        if (remove_directory_recursive(pwd->pw_dir) == 0) {
            printf("홈 디렉터리가 삭제되었습니다.\n");
        }
    }
    
    return 1;
}

// deluser 명령어 구현
int deluser_command(int argc, char* argv[]) {
    int remove_home = 0;
    int force = 0;
    char* username = NULL;
    
    // 인자 파싱
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--remove-home") == 0) {
            remove_home = 1;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            force = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            username = argv[i];
        } else {
            fprintf(stderr, "알 수 없는 옵션: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // 사용자명 확인
    if (username == NULL) {
        fprintf(stderr, "오류: 사용자명을 입력해주세요.\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // 관리자 권한 확인
    if (!check_admin_privileges()) {
        return 1;
    }
    
    // 사용자 존재 확인
    if (!check_user_exists(username)) {
        fprintf(stderr, "오류: 사용자 '%s'가 존재하지 않습니다.\n", username);
        return 1;
    }
    
    // root 사용자 삭제 방지
    if (strcmp(username, "root") == 0) {
        fprintf(stderr, "오류: root 사용자는 삭제할 수 없습니다.\n");
        return 1;
    }
    
    // 확인 (force 옵션이 없는 경우)
    if (!force) {
        if (!confirm_deletion(username, remove_home)) {
            printf("사용자 삭제가 취소되었습니다.\n");
            return 0;
        }
    }
    
    // 사용자 삭제 시도 (시스템 명령어 우선)
    if (delete_system_user(username, remove_home)) {
        return 0;
    }
    
    // 시스템 명령어 실패 시 수동 삭제 시도
    printf("시스템 명령어 실패, 수동 삭제를 시도합니다...\n");
    if (delete_user_manual(username, remove_home)) {
        printf("사용자 '%s'가 성공적으로 삭제되었습니다.\n", username);
        return 0;
    }
    
    fprintf(stderr, "사용자 삭제에 실패했습니다.\n");
    return 1;
}

int main(int argc, char* argv[]) {
    return deluser_command(argc, argv);
}