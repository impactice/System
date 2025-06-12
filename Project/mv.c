#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <fcntl.h>
#include <dirent.h>

#define BUFFER_SIZE 4096

// 옵션 구조체
typedef struct {
    int force;        // -f 옵션
    int interactive;  // -i 옵션
} mv_options;

// 파일인지 디렉토리인지 확인하는 함수
int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

// 파일이 존재하는지 확인하는 함수
int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

// 사용자에게 확인을 요청하는 함수
int ask_user_confirmation(const char *message) {
    printf("%s (y/n): ", message);
    char response;
    scanf(" %c", &response);
    return (response == 'y' || response == 'Y');
}

// 경로 결합 함수
char* join_path(const char *dir, const char *file) {
    size_t dir_len = strlen(dir);
    size_t file_len = strlen(file);
    char *result = malloc(dir_len + file_len + 2);
    
    strcpy(result, dir);
    if (dir_len > 0 && dir[dir_len - 1] != '/') {
        strcat(result, "/");
    }
    strcat(result, file);
    
    return result;
}

// 대상 경로 생성 함수
char* build_dest_path(const char *src, const char *dest) {
    if (is_directory(dest)) {
        // 대상이 디렉토리인 경우, 소스 파일명을 추가
        char *src_copy = strdup(src);
        char *filename = basename(src_copy);
        char *result = join_path(dest, filename);
        free(src_copy);
        return result;
    } else {
        // 대상이 파일인 경우, 그대로 사용
        return strdup(dest);
    }
}

// 파일 복사 함수 (크로스 파일시스템 이동용)
int copy_file_content(const char *src, const char *dest) {
    int src_fd, dest_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    struct stat src_stat;
    
    // 소스 파일 열기
    src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        printf("mv: '%s'를 열 수 없습니다: %s\n", src, strerror(errno));
        return -1;
    }
    
    // 소스 파일 정보 가져오기
    if (fstat(src_fd, &src_stat) == -1) {
        printf("mv: '%s'의 정보를 가져올 수 없습니다: %s\n", src, strerror(errno));
        close(src_fd);
        return -1;
    }
    
    // 대상 파일 생성/열기
    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);
    if (dest_fd == -1) {
        printf("mv: '%s'를 생성할 수 없습니다: %s\n", dest, strerror(errno));
        close(src_fd);
        return -1;
    }
    
    // 파일 내용 복사
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            printf("mv: '%s' 쓰기 오류: %s\n", dest, strerror(errno));
            close(src_fd);
            close(dest_fd);
            unlink(dest);
            return -1;
        }
    }
    
    if (bytes_read == -1) {
        printf("mv: '%s' 읽기 오류: %s\n", src, strerror(errno));
        close(src_fd);
        close(dest_fd);
        unlink(dest);
        return -1;
    }
    
    close(src_fd);
    close(dest_fd);
    
    // 파일 권한 복사
    chmod(dest, src_stat.st_mode);
    
    return 0;
}

// 디렉토리 재귀적 복사 함수
int copy_directory_recursive(const char *src, const char *dest) {
    DIR *dir;
    struct dirent *entry;
    struct stat src_stat;
    char *src_path, *dest_path;
    int result = 0;
    
    // 소스 디렉토리 열기
    dir = opendir(src);
    if (dir == NULL) {
        printf("mv: '%s' 디렉토리를 열 수 없습니다: %s\n", src, strerror(errno));
        return -1;
    }
    
    // 소스 디렉토리 정보 가져오기
    if (stat(src, &src_stat) == -1) {
        printf("mv: '%s'의 정보를 가져올 수 없습니다: %s\n", src, strerror(errno));
        closedir(dir);
        return -1;
    }
    
    // 대상 디렉토리 생성
    if (mkdir(dest, src_stat.st_mode) == -1) {
        printf("mv: '%s' 디렉토리를 생성할 수 없습니다: %s\n", dest, strerror(errno));
        closedir(dir);
        return -1;
    }
    
    // 디렉토리 내용 복사
    while ((entry = readdir(dir)) != NULL) {
        // "."과 ".." 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        src_path = join_path(src, entry->d_name);
        dest_path = join_path(dest, entry->d_name);
        
        if (is_directory(src_path)) {
            if (copy_directory_recursive(src_path, dest_path) == -1) {
                result = -1;
            }
        } else {
            if (copy_file_content(src_path, dest_path) == -1) {
                result = -1;
            }
        }
        
        free(src_path);
        free(dest_path);
    }
    
    closedir(dir);
    return result;
}

// 디렉토리 재귀적 삭제 함수
int remove_directory_recursive(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char *full_path;
    int result = 0;
    
    dir = opendir(path);
    if (dir == NULL) {
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        full_path = join_path(path, entry->d_name);
        
        if (is_directory(full_path)) {
            if (remove_directory_recursive(full_path) == -1) {
                result = -1;
            }
        } else {
            if (unlink(full_path) == -1) {
                result = -1;
            }
        }
        
        free(full_path);
    }
    
    closedir(dir);
    
    if (rmdir(path) == -1) {
        result = -1;
    }
    
    return result;
}

// 단일 파일/디렉토리 이동 함수
int move_item(const char *src, const char *dest, mv_options *opts) {
    struct stat src_stat, dest_stat;
    
    // 소스 존재 확인
    if (stat(src, &src_stat) == -1) {
        printf("mv: '%s'가 존재하지 않습니다: %s\n", src, strerror(errno));
        return -1;
    }
    
    // 대상이 존재하는 경우 처리
    if (stat(dest, &dest_stat) == 0) {
        // 같은 파일인지 확인
        if (src_stat.st_dev == dest_stat.st_dev && src_stat.st_ino == dest_stat.st_ino) {
            printf("mv: '%s'와 '%s'는 같은 파일입니다\n", src, dest);
            return -1;
        }
        
        // 덮어쓰기 확인
        if (opts->interactive && !opts->force) {
            char msg[512];
            snprintf(msg, sizeof(msg), "mv: '%s'를 덮어쓰시겠습니까?", dest);
            if (!ask_user_confirmation(msg)) {
                printf("mv: '%s' 이동을 건너뜁니다\n", src);
                return 0;
            }
        } else if (!opts->force && !opts->interactive) {
            printf("mv: '%s'가 이미 존재합니다 (-f 옵션 없음)\n", dest);
            return -1;
        }
        
        // 대상이 디렉토리이고 비어있지 않은 경우
        if (S_ISDIR(dest_stat.st_mode) && !S_ISDIR(src_stat.st_mode)) {
            printf("mv: '%s'는 디렉토리입니다\n", dest);
            return -1;
        }
    }
    
    // rename()으로 이동 시도 (같은 파일시스템 내에서)
    if (rename(src, dest) == 0) {
        printf("'%s' -> '%s'\n", src, dest);
        return 0;
    }
    
    // 크로스 파일시스템 이동인 경우 복사 후 삭제
    if (errno == EXDEV) {
        printf("크로스 파일시스템 이동: '%s' -> '%s'\n", src, dest);
        
        if (S_ISDIR(src_stat.st_mode)) {
            // 디렉토리 복사
            if (copy_directory_recursive(src, dest) == 0) {
                if (remove_directory_recursive(src) == 0) {
                    printf("'%s' -> '%s' (디렉토리)\n", src, dest);
                    return 0;
                } else {
                    printf("mv: 소스 디렉토리 '%s' 삭제 실패\n", src);
                    return -1;
                }
            }
        } else {
            // 파일 복사
            if (copy_file_content(src, dest) == 0) {
                if (unlink(src) == 0) {
                    printf("'%s' -> '%s'\n", src, dest);
                    return 0;
                } else {
                    printf("mv: 소스 파일 '%s' 삭제 실패\n", src);
                    return -1;
                }
            }
        }
    } else {
        printf("mv: '%s'를 '%s'로 이동할 수 없습니다: %s\n", src, dest, strerror(errno));
    }
    
    return -1;
}

// mv 명령어 구현
int cmd_mv(int argc, char *argv[]) {
    mv_options opts = {0, 0};  // force, interactive
    int i;
    
    // 인수가 부족한 경우
    if (argc < 3) {
        printf("사용법: mv [-f] [-i] <소스> [소스...] <대상>\n");
        printf("  -f: 강제 덮어쓰기\n");
        printf("  -i: 덮어쓰기 전 확인\n");
        return -1;
    }
    
    // 옵션 파싱
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        char *opt = argv[i];
        if (strcmp(opt, "-f") == 0) {
            opts.force = 1;
        } else if (strcmp(opt, "-i") == 0) {
            opts.interactive = 1;
        } else if (strcmp(opt, "-fi") == 0 || strcmp(opt, "-if") == 0) {
            opts.force = 1;
            opts.interactive = 1;
        } else {
            printf("mv: 알 수 없는 옵션 '%s'\n", opt);
            return -1;
        }
    }
    
    // 소스와 대상 확인
    if (i >= argc - 1) {
        printf("mv: 소스와 대상을 모두 지정해야 합니다\n");
        return -1;
    }
    
    char *dest = argv[argc - 1];  // 마지막 인수가 대상
    int success_count = 0;
    int total_count = argc - i - 1;
    
    // 여러 소스가 있는 경우 대상이 디렉토리여야 함
    if (total_count > 1 && !is_directory(dest)) {
        printf("mv: 여러 소스를 이동할 때 대상은 디렉토리여야 합니다\n");
        return -1;
    }
    
    // 각 소스 파일/디렉토리 이동
    for (; i < argc - 1; i++) {
        char *src = argv[i];
        
        // 소스 존재 확인
        if (!file_exists(src)) {
            printf("mv: '%s'가 존재하지 않습니다\n", src);
            continue;
        }
        
        // 대상 경로 생성
        char *actual_dest = build_dest_path(src, dest);
        
        if (move_item(src, actual_dest, &opts) == 0) {
            success_count++;
        }
        
        free(actual_dest);
    }
    
    // 결과 요약
    if (total_count > 1) {
        printf("\n총 %d개 항목 중 %d개 이동 완료\n", total_count, success_count);
    }
    
    return (success_count == total_count) ? 0 : -1;
}

// 테스트용 메인 함수
int main(int argc, char *argv[]) {
    printf("=== mv 명령어 테스트 ===\n");
    
    // 명령행 인수가 있으면 그대로 실행
    if (argc > 1) {
        return cmd_mv(argc, argv);
    }
    
    // 테스트용 파일/디렉토리 생성
    printf("\n테스트 환경 설정 중...\n");
    system("echo 'Original content' > original.txt");
    system("echo 'Test file 1' > test1.txt");
    system("echo 'Test file 2' > test2.txt");
    system("mkdir -p test_dir");
    system("echo 'File in directory' > test_dir/file.txt");
    system("mkdir -p dest_dir");
    
    printf("\n1. 파일 이름 변경 테스트:\n");
    char *test1[] = {"mv", "original.txt", "renamed.txt"};
    cmd_mv(3, test1);
    
    printf("\n2. 파일을 디렉토리로 이동 테스트:\n");
    char *test2[] = {"mv", "test1.txt", "dest_dir/"};
    cmd_mv(3, test2);
    
    printf("\n3. 디렉토리 이동 테스트:\n");
    char *test3[] = {"mv", "test_dir", "moved_dir"};
    cmd_mv(3, test3);
    
    printf("\n4. 여러 파일을 디렉토리로 이동 테스트:\n");
    system("echo 'File A' > fileA.txt");
    system("echo 'File B' > fileB.txt");
    char *test4[] = {"mv", "fileA.txt", "fileB.txt", "dest_dir/"};
    cmd_mv(4, test4);
    
    printf("\n5. 강제 덮어쓰기 테스트:\n");
    system("echo 'New content' > new_file.txt");
    system("echo 'Existing content' > dest_dir/existing.txt");
    char *test5[] = {"mv", "-f", "new_file.txt", "dest_dir/existing.txt"};
    cmd_mv(4, test5);
    
    printf("\n6. 잘못된 사용법 테스트:\n");
    char *test6[] = {"mv", "nonexistent.txt", "somewhere.txt"};
    cmd_mv(3, test6);
    
    printf("\n현재 디렉토리 상태:\n");
    system("ls -la");
    printf("\ndest_dir 내용:\n");
    system("ls -la dest_dir/");
    
    // 정리
    printf("\n테스트 파일 정리 중...\n");
    system("rm -rf renamed.txt moved_dir dest_dir test2.txt");
    
    return 0;
}