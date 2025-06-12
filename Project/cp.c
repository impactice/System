m#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>

#define BUFFER_SIZE 4096

// 옵션 구조체
typedef struct {
    int recursive;    // -r, -R 옵션
    int force;        // -f 옵션
    int interactive;  // -i 옵션
} cp_options;

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

// 단일 파일 복사 함수
int copy_file(const char *src, const char *dest, cp_options *opts) {
    int src_fd, dest_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    struct stat src_stat;
    
    // 소스 파일 열기
    src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        printf("cp: '%s'를 열 수 없습니다: %s\n", src, strerror(errno));
        return -1;
    }
    
    // 소스 파일 정보 가져오기
    if (fstat(src_fd, &src_stat) == -1) {
        printf("cp: '%s'의 정보를 가져올 수 없습니다: %s\n", src, strerror(errno));
        close(src_fd);
        return -1;
    }
    
    // 대상 파일이 존재하는 경우 처리
    if (file_exists(dest)) {
        if (opts->interactive && !opts->force) {
            char msg[512];
            snprintf(msg, sizeof(msg), "cp: '%s'를 덮어쓰시겠습니까?", dest);
            if (!ask_user_confirmation(msg)) {
                printf("cp: '%s' 복사를 건너뜁니다\n", dest);
                close(src_fd);
                return 0;
            }
        } else if (!opts->force && !opts->interactive) {
            printf("cp: '%s'가 이미 존재합니다 (-f 옵션 없음)\n", dest);
            close(src_fd);
            return -1;
        }
    }
    
    // 대상 파일 생성/열기
    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);
    if (dest_fd == -1) {
        printf("cp: '%s'를 생성할 수 없습니다: %s\n", dest, strerror(errno));
        close(src_fd);
        return -1;
    }
    
    // 파일 내용 복사
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            printf("cp: '%s' 쓰기 오류: %s\n", dest, strerror(errno));
            close(src_fd);
            close(dest_fd);
            unlink(dest);  // 실패한 파일 삭제
            return -1;
        }
    }
    
    if (bytes_read == -1) {
        printf("cp: '%s' 읽기 오류: %s\n", src, strerror(errno));
        close(src_fd);
        close(dest_fd);
        unlink(dest);
        return -1;
    }
    
    close(src_fd);
    close(dest_fd);
    
    // 파일 권한 복사
    chmod(dest, src_stat.st_mode);
    
    printf("'%s' -> '%s'\n", src, dest);
    return 0;
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

// 디렉토리 재귀적 복사 함수
int copy_directory(const char *src, const char *dest, cp_options *opts) {
    DIR *dir;
    struct dirent *entry;
    struct stat src_stat;
    char *src_path, *dest_path;
    int result = 0;
    
    // 소스 디렉토리 열기
    dir = opendir(src);
    if (dir == NULL) {
        printf("cp: '%s' 디렉토리를 열 수 없습니다: %s\n", src, strerror(errno));
        return -1;
    }
    
    // 소스 디렉토리 정보 가져오기
    if (stat(src, &src_stat) == -1) {
        printf("cp: '%s'의 정보를 가져올 수 없습니다: %s\n", src, strerror(errno));
        closedir(dir);
        return -1;
    }
    
    // 대상 디렉토리 생성
    if (!file_exists(dest)) {
        if (mkdir(dest, src_stat.st_mode) == -1) {
            printf("cp: '%s' 디렉토리를 생성할 수 없습니다: %s\n", dest, strerror(errno));
            closedir(dir);
            return -1;
        }
        printf("디렉토리 '%s' 생성\n", dest);
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
            // 하위 디렉토리 재귀적 복사
            if (copy_directory(src_path, dest_path, opts) == -1) {
                result = -1;
            }
        } else {
            // 파일 복사
            if (copy_file(src_path, dest_path, opts) == -1) {
                result = -1;
            }
        }
        
        free(src_path);
        free(dest_path);
    }
    
    closedir(dir);
    return result;
}

// 대상 경로 생성 함수 (파일명 추출)
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

// cp 명령어 구현
int cmd_cp(int argc, char *argv[]) {
    cp_options opts = {0, 0, 0};  // recursive, force, interactive
    int i;
    
    // 인수가 부족한 경우
    if (argc < 3) {
        printf("사용법: cp [-r|-R] [-f] [-i] <소스> [소스...] <대상>\n");
        printf("  -r, -R: 디렉토리 재귀적 복사\n");
        printf("  -f: 강제 덮어쓰기\n");
        printf("  -i: 덮어쓰기 전 확인\n");
        return -1;
    }
    
    // 옵션 파싱
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        char *opt = argv[i];
        if (strcmp(opt, "-r") == 0 || strcmp(opt, "-R") == 0) {
            opts.recursive = 1;
        } else if (strcmp(opt, "-f") == 0) {
            opts.force = 1;
        } else if (strcmp(opt, "-i") == 0) {
            opts.interactive = 1;
        } else if (strcmp(opt, "-rf") == 0 || strcmp(opt, "-fr") == 0 ||
                   strcmp(opt, "-Rf") == 0 || strcmp(opt, "-fR") == 0) {
            opts.recursive = 1;
            opts.force = 1;
        } else if (strcmp(opt, "-ri") == 0 || strcmp(opt, "-ir") == 0 ||
                   strcmp(opt, "-Ri") == 0 || strcmp(opt, "-iR") == 0) {
            opts.recursive = 1;
            opts.interactive = 1;
        } else if (strcmp(opt, "-fi") == 0 || strcmp(opt, "-if") == 0) {
            opts.force = 1;
            opts.interactive = 1;
        } else {
            printf("cp: 알 수 없는 옵션 '%s'\n", opt);
            return -1;
        }
    }
    
    // 소스와 대상 확인
    if (i >= argc - 1) {
        printf("cp: 소스와 대상을 모두 지정해야 합니다\n");
        return -1;
    }
    
    char *dest = argv[argc - 1];  // 마지막 인수가 대상
    int success_count = 0;
    int total_count = argc - i - 1;
    
    // 각 소스 파일/디렉토리 복사
    for (; i < argc - 1; i++) {
        char *src = argv[i];
        
        // 소스 존재 확인
        if (!file_exists(src)) {
            printf("cp: '%s'가 존재하지 않습니다\n", src);
            continue;
        }
        
        // 대상 경로 생성
        char *actual_dest = build_dest_path(src, dest);
        
        if (is_directory(src)) {
            if (!opts.recursive) {
                printf("cp: '%s'는 디렉토리입니다 (-r 옵션이 필요합니다)\n", src);
                free(actual_dest);
                continue;
            }
            
            if (copy_directory(src, actual_dest, &opts) == 0) {
                success_count++;
            }
        } else {
            if (copy_file(src, actual_dest, &opts) == 0) {
                success_count++;
            }
        }
        
        free(actual_dest);
    }
    
    // 결과 요약
    if (total_count > 1) {
        printf("\n총 %d개 항목 중 %d개 복사 완료\n", total_count, success_count);
    }
    
    return (success_count == total_count) ? 0 : -1;
}

// 테스트용 메인 함수
int main(int argc, char *argv[]) {
    printf("=== cp 명령어 테스트 ===\n");
    
    // 명령행 인수가 있으면 그대로 실행
    if (argc > 1) {
        return cmd_cp(argc, argv);
    }
    
    // 테스트용 파일/디렉토리 생성
    printf("\n테스트 환경 설정 중...\n");
    system("echo 'Hello World' > test_file.txt");
    system("mkdir -p test_dir");
    system("echo 'File in directory' > test_dir/subfile.txt");
    system("mkdir -p test_dir/subdir");
    system("echo 'Nested file' > test_dir/subdir/nested.txt");
    
    printf("\n1. 단일 파일 복사 테스트:\n");
    char *test1[] = {"cp", "test_file.txt", "copy_file.txt"};
    cmd_cp(3, test1);
    
    printf("\n2. 파일을 디렉토리로 복사 테스트:\n");
    system("mkdir -p dest_dir");
    char *test2[] = {"cp", "test_file.txt", "dest_dir/"};
    cmd_cp(3, test2);
    
    printf("\n3. 디렉토리 재귀적 복사 테스트:\n");
    char *test3[] = {"cp", "-r", "test_dir", "copy_dir"};
    cmd_cp(4, test3);
    
    printf("\n4. 강제 덮어쓰기 테스트:\n");
    char *test4[] = {"cp", "-f", "test_file.txt", "copy_file.txt"};
    cmd_cp(4, test4);
    
    printf("\n5. 여러 파일 복사 테스트:\n");
    system("echo 'File 2' > file2.txt");
    system("echo 'File 3' > file3.txt");
    char *test5[] = {"cp", "file2.txt", "file3.txt", "dest_dir/"};
    cmd_cp(4, test5);
    
    printf("\n6. 잘못된 사용법 테스트:\n");
    char *test6[] = {"cp", "test_dir", "copy_fail"};  // -r 없이 디렉토리 복사
    cmd_cp(3, test6);
    
    printf("\n생성된 파일들 확인:\n");
    system("ls -la copy_file.txt dest_dir/ copy_dir/ 2>/dev/null");
    
    // 정리
    printf("\n테스트 파일 정리 중...\n");
    system("rm -rf test_file.txt test_dir copy_file.txt dest_dir copy_dir file2.txt file3.txt");
    
    return 0;
}