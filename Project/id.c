#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <string.h>

// 기본 id 명령어 구현 (uid, gid, groups 모두 출력)
int cmd_id() {
    uid_t uid = getuid();        // 실제 사용자 ID
    uid_t euid = geteuid();      // 유효 사용자 ID
    gid_t gid = getgid();        // 실제 그룹 ID
    gid_t egid = getegid();      // 유효 그룹 ID
    
    struct passwd *pw = getpwuid(uid);
    struct group *gr = getgrgid(gid);
    
    // uid 출력
    printf("uid=%d", uid);
    if (pw != NULL) {
        printf("(%s)", pw->pw_name);
    }
    
    // gid 출력
    printf(" gid=%d", gid);
    if (gr != NULL) {
        printf("(%s)", gr->gr_name);
    }
    
    // euid가 uid와 다르면 출력
    if (euid != uid) {
        struct passwd *epw = getpwuid(euid);
        printf(" euid=%d", euid);
        if (epw != NULL) {
            printf("(%s)", epw->pw_name);
        }
    }
    
    // egid가 gid와 다르면 출력
    if (egid != gid) {
        struct group *egr = getgrgid(egid);
        printf(" egid=%d", egid);
        if (egr != NULL) {
            printf("(%s)", egr->gr_name);
        }
    }
    
    // 보조 그룹들 출력
    int ngroups = getgroups(0, NULL);  // 그룹 개수 확인
    if (ngroups > 0) {
        gid_t *groups = malloc(ngroups * sizeof(gid_t));
        if (groups != NULL) {
            if (getgroups(ngroups, groups) != -1) {
                printf(" groups=");
                for (int i = 0; i < ngroups; i++) {
                    if (i > 0) printf(",");
                    printf("%d", groups[i]);
                    
                    struct group *g = getgrgid(groups[i]);
                    if (g != NULL) {
                        printf("(%s)", g->gr_name);
                    }
                }
            }
            free(groups);
        }
    }
    
    printf("\n");
    return 0;
}

// -u 옵션: 사용자 ID만 출력
int cmd_id_user() {
    printf("%d\n", getuid());
    return 0;
}

// -g 옵션: 그룹 ID만 출력
int cmd_id_group() {
    printf("%d\n", getgid());
    return 0;
}

// -un 옵션: 사용자 이름 출력
int cmd_id_user_name() {
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    
    if (pw == NULL) {
        printf("%d\n", uid);  // 이름을 찾을 수 없으면 숫자로 출력
    } else {
        printf("%s\n", pw->pw_name);
    }
    return 0;
}

// -gn 옵션: 그룹 이름 출력
int cmd_id_group_name() {
    gid_t gid = getgid();
    struct group *gr = getgrgid(gid);
    
    if (gr == NULL) {
        printf("%d\n", gid);  // 이름을 찾을 수 없으면 숫자로 출력
    } else {
        printf("%s\n", gr->gr_name);
    }
    return 0;
}

// -G 옵션: 모든 그룹 ID 출력
int cmd_id_all_groups() {
    int ngroups = getgroups(0, NULL);
    if (ngroups > 0) {
        gid_t *groups = malloc(ngroups * sizeof(gid_t));
        if (groups != NULL) {
            if (getgroups(ngroups, groups) != -1) {
                for (int i = 0; i < ngroups; i++) {
                    if (i > 0) printf(" ");
                    printf("%d", groups[i]);
                }
                printf("\n");
            }
            free(groups);
        }
    }
    return 0;
}

// 터미널에서 사용할 메인 함수
int execute_id(char **args) {
    if (args[1] == NULL) {
        // 옵션이 없으면 기본 출력
        return cmd_id();
    }
    
    // 옵션 파싱
    if (strcmp(args[1], "-u") == 0) {
        return cmd_id_user();
    } else if (strcmp(args[1], "-g") == 0) {
        return cmd_id_group();
    } else if (strcmp(args[1], "-un") == 0) {
        return cmd_id_user_name();
    } else if (strcmp(args[1], "-gn") == 0) {
        return cmd_id_group_name();
    } else if (strcmp(args[1], "-G") == 0) {
        return cmd_id_all_groups();
    } else {
        fprintf(stderr, "id: invalid option '%s'\n", args[1]);
        fprintf(stderr, "Usage: id [-u|-g|-G|-un|-gn]\n");
        return 1;
    }
}

// 테스트용 main 함수
int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Basic id output:\n");
        cmd_id();
        
        printf("\nUser ID: ");
        cmd_id_user();
        
        printf("Group ID: ");
        cmd_id_group();
        
        printf("User name: ");
        cmd_id_user_name();
        
        printf("Group name: ");
        cmd_id_group_name();
        
        printf("All groups: ");
        cmd_id_all_groups();
    } else {
        return execute_id(argv);
    }
    
    return 0;
}