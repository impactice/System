#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

// 디렉토리가 비어있는지 확인하는 함수
int is_directory_empty(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return -1;  // 디렉토리를 열 수 없음
    }
    
    struct dirent *entry;
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // "."과 ".."은 제외하고 카운트
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }
    
    closedir(dir);
    return (count == 0) ? 1 : 0;  // 1: 비어있음, 0: 비어있지 않음
}

// 디렉토리가 존재하는지 확인하는 함수
int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

// 단일 디렉토리 삭제 함수
int remove_directory(const char *path) {
    // 경로가 존재하는지 확인
    if (access(path, F_OK) != 0) {
        printf("rmdir: '%s': 그런 파일이나 디렉토리가 없습니다\n", path);
        return -1;
    }
    
    // 디렉토리인지 확인
    if (!is_directory(path)) {
        printf("rmdir: '%s': 디렉토리가 아닙니다\n", path);
        return -1;
    }
    
    // 디렉토리가 비어있는지 확인
    int empty_check = is_directory_empty(path);
    if (empty_check == -1) {
        printf("rmdir: '%s': 디렉토리를 읽을 수 없습니다\n", path);
        return -1;
    } else if (empty_check == 0) {
        printf("rmdir: '%s': 디렉토리가 비어있지 않습니다\n", path);
        return -1;
    }
    
    // 디렉토리 삭제
    if (rmdir(path) == 0) {
        printf("디렉토리 '%s' 삭제 완료\n", path);
        return 0;
    } else {
        switch (errno) {
            case ENOTEMPTY:
                printf("rmdir: '%s': 디렉토리가 비어있지 않습니다\n", path);
                break;
            case EACCES:
                printf("rmdir: '%s': 권한이 거부되었습니다\n", path);
                break;
            case EBUSY:
                printf("rmdir: '%s': 디렉토리가 사용 중입니다\n", path);
                break;
            case EINVAL:
                printf("rmdir: '%s': 잘못된 인수입니다\n", path);
                break;
            case ENOTDIR:
                printf("rmdir: '%s': 디렉토리가 아닙니다\n", path);
                break;
            default:
                perror("rmdir");
                break;
        }
        return -1;
    }
}

// rmdir 명령어 구현
int cmd_rmdir(int argc, char *argv[]) {
    int success_count = 0;
    int total_count = 0;
    
    // 인수가 부족한 경우
    if (argc < 2) {
        printf("사용법: rmdir <디렉토리명> [디렉토리명...]\n");
        return -1;
    }
    
    // 각 디렉토리에 대해 삭제 시도
    for (int i = 1; i < argc; i++) {
        // 옵션 처리 (현재는 옵션 없음)
        if (argv[i][0] == '-') {
            printf("rmdir: 알 수 없는 옵션 '%s'\n", argv[i]);
            printf("사용법: rmdir <디렉토리명> [디렉토리명...]\n");
            continue;
        }
        
        total_count++;
        if (remove_directory(argv[i]) == 0) {
            success_count++;
        }
    }
    
    // 결과 요약
    if (total_count > 1) {
        printf("\n총 %d개 디렉토리 중 %d개 삭제 완료\n", total_count, success_count);
    }
    
    return (success_count == total_count) ? 0 : -1;
}

// 현재 디렉토리의 내용을 보여주는 도우미 함수 (테스트용)
void show_directory_contents(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        printf("디렉토리 '%s'를 열 수 없습니다\n", path);
        return;
    }
    
    printf("디렉토리 '%s'의 내용:\n", path);
    struct dirent *entry;
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            printf("  - %s\n", entry->d_name);
            count++;
        }
    }
    
    if (count == 0) {
        printf("  (비어있음)\n");
    }
    
    closedir(dir);
}

// 테스트용 메인 함수
int main(int argc, char *argv[]) {
    printf("=== rmdir 명령어 테스트 ===\n");
    
    // 명령행 인수가 있으면 그대로 실행
    if (argc > 1) {
        return cmd_rmdir(argc, argv);
    }
    
    // 테스트를 위한 디렉토리 생성
    printf("\n테스트용 디렉토리 생성 중...\n");
    system("mkdir -p test_empty");
    system("mkdir -p test_nonempty");
    system("touch test_nonempty/file.txt");  // 비어있지 않은 디렉토리 생성
    system("mkdir -p dir1 dir2 dir3");
    
    printf("\n1. 비어있는 디렉토리 삭제 테스트:\n");
    char *test1[] = {"rmdir", "test_empty"};
    cmd_rmdir(2, test1);
    
    printf("\n2. 비어있지 않은 디렉토리 삭제 테스트 (실패해야 함):\n");
    show_directory_contents("test_nonempty");
    char *test2[] = {"rmdir", "test_nonempty"};
    cmd_rmdir(2, test2);
    
    printf("\n3. 여러 디렉토리 삭제 테스트:\n");
    char *test3[] = {"rmdir", "dir1", "dir2", "dir3"};
    cmd_rmdir(4, test3);
    
    printf("\n4. 존재하지 않는 디렉토리 삭제 테스트 (실패해야 함):\n");
    char *test4[] = {"rmdir", "nonexistent_dir"};
    cmd_rmdir(2, test4);
    
    printf("\n5. 잘못된 사용법 테스트:\n");
    char *test5[] = {"rmdir"};
    cmd_rmdir(1, test5);
    
    // 정리
    printf("\n테스트 정리 중...\n");
    system("rm -rf test_nonempty");  // 테스트용 파일 정리
    
    return 0;
}