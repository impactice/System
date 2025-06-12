#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

// 사용법 출력
void print_usage(const char* program_name) {
    printf("Usage: %s [OPTION]... [OWNER][:[GROUP]] FILE...\n", program_name);
    printf("       %s [OPTION]... --reference=RFILE FILE...\n", program_name);
    printf("\n");
    printf("Change the owner and/or group of each FILE to OWNER and/or GROUP.\n");
    printf("\n");
    printf("Options:\n");
    printf("  -R, --recursive    operate on files and directories recursively\n");
    printf("  -v, --verbose      output a diagnostic for every file processed\n");
    printf("  -h, --help         display this help and exit\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s root file.txt           # Change owner to root\n", program_name);
    printf("  %s root:admin file.txt     # Change owner to root, group to admin\n", program_name);
    printf("  %s :admin file.txt         # Change group to admin only\n", program_name);
    printf("  %s -R user:group dir/      # Recursively change ownership\n", program_name);
}

// 사용자 이름을 UID로 변환
uid_t get_uid_from_name(const char* name) {
    if (!name || *name == '\0') return -1;
    
    // 숫자인지 확인
    char* endptr;
    long uid = strtol(name, &endptr, 10);
    if (*endptr == '\0') {
        return (uid_t)uid;
    }
    
    // 사용자 이름으로 검색
    struct passwd* pwd = getpwnam(name);
    if (pwd == NULL) {
        return -1;
    }
    return pwd->pw_uid;
}

// 그룹 이름을 GID로 변환
gid_t get_gid_from_name(const char* name) {
    if (!name || *name == '\0') return -1;
    
    // 숫자인지 확인
    char* endptr;
    long gid = strtol(name, &endptr, 10);
    if (*endptr == '\0') {
        return (gid_t)gid;
    }
    
    // 그룹 이름으로 검색
    struct group* grp = getgrnam(name);
    if (grp == NULL) {
        return -1;
    }
    return grp->gr_gid;
}

// 소유자:그룹 문자열 파싱
int parse_owner_group(const char* spec, uid_t* uid, gid_t* gid, int* change_uid, int* change_gid) {
    *change_uid = 0;
    *change_gid = 0;
    
    char* spec_copy = strdup(spec);
    if (!spec_copy) {
        perror("strdup");
        return -1;
    }
    
    char* colon = strchr(spec_copy, ':');
    
    if (colon) {
        *colon = '\0';
        
        // 소유자 부분
        if (strlen(spec_copy) > 0) {
            *uid = get_uid_from_name(spec_copy);
            if (*uid == (uid_t)-1) {
                fprintf(stderr, "chown: invalid user: '%s'\n", spec_copy);
                free(spec_copy);
                return -1;
            }
            *change_uid = 1;
        }
        
        // 그룹 부분
        if (strlen(colon + 1) > 0) {
            *gid = get_gid_from_name(colon + 1);
            if (*gid == (gid_t)-1) {
                fprintf(stderr, "chown: invalid group: '%s'\n", colon + 1);
                free(spec_copy);
                return -1;
            }
            *change_gid = 1;
        }
    } else {
        // 콜론이 없으면 소유자만 변경
        *uid = get_uid_from_name(spec_copy);
        if (*uid == (uid_t)-1) {
            fprintf(stderr, "chown: invalid user: '%s'\n", spec_copy);
            free(spec_copy);
            return -1;
        }
        *change_uid = 1;
    }
    
    free(spec_copy);
    return 0;
}

// 단일 파일/디렉토리 소유권 변경
int chown_single(const char* path, uid_t uid, gid_t gid, int change_uid, int change_gid, int verbose) {
    struct stat st;
    if (lstat(path, &st) == -1) {
        fprintf(stderr, "chown: cannot access '%s': %s\n", path, strerror(errno));
        return -1;
    }
    
    uid_t new_uid = change_uid ? uid : st.st_uid;
    gid_t new_gid = change_gid ? gid : st.st_gid;
    
    if (chown(path, new_uid, new_gid) == -1) {
        fprintf(stderr, "chown: changing ownership of '%s': %s\n", path, strerror(errno));
        return -1;
    }
    
    if (verbose) {
        struct passwd* pwd = getpwuid(new_uid);
        struct group* grp = getgrgid(new_gid);
        printf("ownership of '%s' changed to %s:%s\n", 
               path,
               pwd ? pwd->pw_name : "unknown",
               grp ? grp->gr_name : "unknown");
    }
    
    return 0;
}

// 재귀적으로 디렉토리 처리
int chown_recursive(const char* path, uid_t uid, gid_t gid, int change_uid, int change_gid, int verbose) {
    // 현재 디렉토리/파일 소유권 변경
    if (chown_single(path, uid, gid, change_uid, change_gid, verbose) == -1) {
        return -1;
    }
    
    struct stat st;
    if (lstat(path, &st) == -1) {
        return -1;
    }
    
    // 디렉토리가 아니면 종료
    if (!S_ISDIR(st.st_mode)) {
        return 0;
    }
    
    DIR* dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "chown: cannot open directory '%s': %s\n", path, strerror(errno));
        return -1;
    }
    
    struct dirent* entry;
    int result = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // . 과 .. 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (chown_recursive(full_path, uid, gid, change_uid, change_gid, verbose) == -1) {
            result = -1;
        }
    }
    
    closedir(dir);
    return result;
}

int main(int argc, char* argv[]) {
    int recursive = 0;
    int verbose = 0;
    int opt_index = 1;
    
    // 옵션 파싱
    while (opt_index < argc && argv[opt_index][0] == '-') {
        if (strcmp(argv[opt_index], "-R") == 0 || strcmp(argv[opt_index], "--recursive") == 0) {
            recursive = 1;
        } else if (strcmp(argv[opt_index], "-v") == 0 || strcmp(argv[opt_index], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[opt_index], "-h") == 0 || strcmp(argv[opt_index], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "chown: invalid option -- '%s'\n", argv[opt_index]);
            print_usage(argv[0]);
            return 1;
        }
        opt_index++;
    }
    
    // 최소 2개 인자 필요 (소유자:그룹, 파일)
    if (argc - opt_index < 2) {
        fprintf(stderr, "chown: missing operand\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // 소유자:그룹 파싱
    uid_t uid;
    gid_t gid;
    int change_uid, change_gid;
    
    if (parse_owner_group(argv[opt_index], &uid, &gid, &change_uid, &change_gid) == -1) {
        return 1;
    }
    
    if (!change_uid && !change_gid) {
        fprintf(stderr, "chown: no owner or group specified\n");
        return 1;
    }
    
    opt_index++; // 소유자:그룹 인자 넘어가기
    
    // 권한 확인 (실제 시스템에서는 root 권한이 필요할 수 있음)
    if (getuid() != 0 && (change_uid || change_gid)) {
        fprintf(stderr, "chown: changing ownership requires superuser privileges\n");
        // 경고만 출력하고 계속 진행 (테스트 목적)
    }
    
    int result = 0;
    
    // 각 파일에 대해 작업 수행
    for (int i = opt_index; i < argc; i++) {
        if (recursive) {
            if (chown_recursive(argv[i], uid, gid, change_uid, change_gid, verbose) == -1) {
                result = 1;
            }
        } else {
            if (chown_single(argv[i], uid, gid, change_uid, change_gid, verbose) == -1) {
                result = 1;
            }
        }
    }
    
    return result;
}