#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

// 디렉토리 생성 함수 (단일 디렉토리)
int create_directory(const char *path) {
    if (mkdir(path, 0755) == 0) {
        printf("디렉토리 '%s' 생성 완료\n", path);
        return 0;
    } else {
        if (errno == EEXIST) {
            printf("디렉토리 '%s'는 이미 존재합니다\n", path);
            return 0;  // -p 옵션에서는 이미 존재해도 성공으로 처리
        } else {
            perror("mkdir");
            return -1;
        }
    }
}

// 상위 디렉토리까지 재귀적으로 생성하는 함수
int create_directory_recursive(const char *path) {
    char *path_copy = strdup(path);
    char *p = path_copy;
    
    // 절대 경로인 경우 첫 번째 '/'를 건너뛰기
    if (*p == '/') {
        p++;
    }
    
    // 경로를 '/'로 분할하며 순차적으로 디렉토리 생성
    while ((p = strchr(p, '/'))) {
        *p = '\0';  // 임시로 문자열 종료
        
        // 현재 경로까지 디렉토리 생성
        if (mkdir(path_copy, 0755) == -1 && errno != EEXIST) {
            perror("mkdir");
            free(path_copy);
            return -1;
        }
        
        *p = '/';   // 다시 '/' 복원
        p++;
    }
    
    // 최종 디렉토리 생성
    int result = create_directory(path);
    free(path_copy);
    return result;
}

// mkdir 명령어 구현
int cmd_mkdir(int argc, char *argv[]) {
    int p_flag = 0;  // -p 옵션 플래그
    int i;
    
    // 인수가 부족한 경우
    if (argc < 2) {
        printf("사용법: mkdir [-p] <디렉토리명> [디렉토리명...]\n");
        return -1;
    }
    
    // 옵션 파싱
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            p_flag = 1;
        } else {
            printf("알 수 없는 옵션: %s\n", argv[i]);
            printf("사용법: mkdir [-p] <디렉토리명> [디렉토리명...]\n");
            return -1;
        }
    }
    
    // 디렉토리명이 제공되지 않은 경우
    if (i >= argc) {
        printf("디렉토리명을 입력해주세요\n");
        printf("사용법: mkdir [-p] <디렉토리명> [디렉토리명...]\n");
        return -1;
    }
    
    // 각 디렉토리 생성
    for (; i < argc; i++) {
        if (p_flag) {
            // -p 옵션: 상위 디렉토리도 함께 생성
            if (create_directory_recursive(argv[i]) == -1) {
                printf("디렉토리 '%s' 생성 실패\n", argv[i]);
            }
        } else {
            // 일반 모드: 해당 디렉토리만 생성
            if (create_directory(argv[i]) == -1) {
                printf("디렉토리 '%s' 생성 실패\n", argv[i]);
            }
        }
    }
    
    return 0;
}

// 테스트용 메인 함수
int main(int argc, char *argv[]) {
    printf("=== mkdir 명령어 테스트 ===\n");
    
    // 명령행 인수가 있으면 그대로 실행
    if (argc > 1) {
        return cmd_mkdir(argc, argv);
    }
    
    // 테스트 케이스들
    printf("\n1. 단일 디렉토리 생성 테스트:\n");
    char *test1[] = {"mkdir", "test_dir"};
    cmd_mkdir(2, test1);
    
    printf("\n2. 여러 디렉토리 생성 테스트:\n");
    char *test2[] = {"mkdir", "dir1", "dir2", "dir3"};
    cmd_mkdir(4, test2);
    
    printf("\n3. -p 옵션으로 중첩 디렉토리 생성 테스트:\n");
    char *test3[] = {"mkdir", "-p", "parent/child/grandchild"};
    cmd_mkdir(3, test3);
    
    printf("\n4. -p 옵션으로 여러 중첩 디렉토리 생성 테스트:\n");
    char *test4[] = {"mkdir", "-p", "a/b/c", "x/y/z"};
    cmd_mkdir(4, test4);
    
    printf("\n5. 잘못된 사용법 테스트:\n");
    char *test5[] = {"mkdir"};
    cmd_mkdir(1, test5);
    
    return 0;
}