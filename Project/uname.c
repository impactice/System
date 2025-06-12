#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

// 시스템 정보 옵션 플래그
typedef struct {
    int show_sysname;    // -s: 시스템 이름 (기본값)
    int show_nodename;   // -n: 네트워크 노드 호스트명
    int show_release;    // -r: 운영체제 릴리스
    int show_version;    // -v: 운영체제 버전
    int show_machine;    // -m: 머신 하드웨어 이름
    int show_processor;  // -p: 프로세서 타입
    int show_hwplatform; // -i: 하드웨어 플랫폼
    int show_opsystem;   // -o: 운영체제
    int show_all;        // -a: 모든 정보
} UnameOptions;

// 프로세서 정보를 /proc/cpuinfo에서 가져오기
int get_processor_info(char* processor, size_t size) {
    FILE* file = fopen("/proc/cpuinfo", "r");
    if (!file) {
        strncpy(processor, "unknown", size - 1);
        processor[size - 1] = '\0';
        return -1;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "model name", 10) == 0) {
            char* colon = strchr(line, ':');
            if (colon) {
                colon++; // ':' 다음으로 이동
                while (*colon == ' ' || *colon == '\t') colon++; // 공백 제거
                
                // 개행 문자 제거
                char* newline = strchr(colon, '\n');
                if (newline) *newline = '\0';
                
                strncpy(processor, colon, size - 1);
                processor[size - 1] = '\0';
                fclose(file);
                return 0;
            }
        }
    }
    
    // model name이 없는 경우 vendor_id 시도
    rewind(file);
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "vendor_id", 9) == 0) {
            char* colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                
                char* newline = strchr(colon, '\n');
                if (newline) *newline = '\0';
                
                strncpy(processor, colon, size - 1);
                processor[size - 1] = '\0';
                fclose(file);
                return 0;
            }
        }
    }
    
    fclose(file);
    strncpy(processor, "unknown", size - 1);
    processor[size - 1] = '\0';
    return -1;
}

// 하드웨어 플랫폼 정보 가져오기
void get_hardware_platform(char* platform, size_t size, const char* machine) {
    // 일반적인 아키텍처 매핑
    if (strstr(machine, "x86_64") || strstr(machine, "amd64")) {
        strncpy(platform, "x86_64", size - 1);
    } else if (strstr(machine, "i386") || strstr(machine, "i686")) {
        strncpy(platform, "i386", size - 1);
    } else if (strstr(machine, "arm") || strstr(machine, "aarch64")) {
        strncpy(platform, "arm", size - 1);
    } else if (strstr(machine, "mips")) {
        strncpy(platform, "mips", size - 1);
    } else {
        strncpy(platform, machine, size - 1);
    }
    platform[size - 1] = '\0';
}

// 시스템 정보 출력
void print_system_info(const UnameOptions* opts) {
    struct utsname sys_info;
    char processor[256] = "unknown";
    char hardware_platform[256];
    int first_output = 1;
    
    // uname 시스템 콜로 기본 정보 가져오기
    if (uname(&sys_info) != 0) {
        perror("uname");
        return;
    }
    
    // 추가 정보 수집
    get_processor_info(processor, sizeof(processor));
    get_hardware_platform(hardware_platform, sizeof(hardware_platform), sys_info.machine);
    
    // 시스템 이름 (기본값 또는 -s 옵션)
    if (opts->show_sysname || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", sys_info.sysname);
        first_output = 0;
    }
    
    // 네트워크 노드 호스트명 (-n 옵션)
    if (opts->show_nodename || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", sys_info.nodename);
        first_output = 0;
    }
    
    // 운영체제 릴리스 (-r 옵션)
    if (opts->show_release || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", sys_info.release);
        first_output = 0;
    }
    
    // 운영체제 버전 (-v 옵션)
    if (opts->show_version || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", sys_info.version);
        first_output = 0;
    }
    
    // 머신 하드웨어 이름 (-m 옵션)
    if (opts->show_machine || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", sys_info.machine);
        first_output = 0;
    }
    
    // 프로세서 타입 (-p 옵션)
    if (opts->show_processor || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", processor);
        first_output = 0;
    }
    
    // 하드웨어 플랫폼 (-i 옵션)
    if (opts->show_hwplatform || opts->show_all) {
        if (!first_output) printf(" ");
        printf("%s", hardware_platform);
        first_output = 0;
    }
    
    // 운영체제 (-o 옵션)
    if (opts->show_opsystem || opts->show_all) {
        if (!first_output) printf(" ");
        printf("GNU/Linux");
        first_output = 0;
    }
    
    printf("\n");
}

// 도움말 출력
void print_help() {
    printf("Usage: uname [OPTION]...\n");
    printf("Print certain system information. With no OPTION, same as -s.\n\n");
    printf("Options:\n");
    printf("  -a, --all                print all information, in the following order,\n");
    printf("                           except omit -p and -i if unknown:\n");
    printf("  -s, --kernel-name        print the kernel name\n");
    printf("  -n, --nodename           print the network node hostname\n");
    printf("  -r, --kernel-release     print the kernel release\n");
    printf("  -v, --kernel-version     print the kernel version\n");
    printf("  -m, --machine            print the machine hardware name\n");
    printf("  -p, --processor          print the processor type (non-portable)\n");
    printf("  -i, --hardware-platform  print the hardware platform (non-portable)\n");
    printf("  -o, --operating-system   print the operating system\n");
    printf("      --help               display this help and exit\n");
    printf("      --version            output version information and exit\n");
}

// 버전 정보 출력
void print_version() {
    printf("uname (custom implementation) 1.0\n");
    printf("Written for terminal implementation project.\n");
}

int main(int argc, char* argv[]) {
    UnameOptions opts = {0}; // 모든 플래그를 0으로 초기화
    int any_option_set = 0;
    
    // 명령행 인수 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) {
            opts.show_all = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--kernel-name") == 0) {
            opts.show_sysname = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--nodename") == 0) {
            opts.show_nodename = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--kernel-release") == 0) {
            opts.show_release = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--kernel-version") == 0) {
            opts.show_version = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--machine") == 0) {
            opts.show_machine = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--processor") == 0) {
            opts.show_processor = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--hardware-platform") == 0) {
            opts.show_hwplatform = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--operating-system") == 0) {
            opts.show_opsystem = 1;
            any_option_set = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        } else if (strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else {
            fprintf(stderr, "uname: invalid option -- '%s'\n", argv[i]);
            fprintf(stderr, "Try 'uname --help' for more information.\n");
            return 1;
        }
    }
    
    // 옵션이 지정되지 않은 경우 기본값은 -s (시스템 이름)
    if (!any_option_set) {
        opts.show_sysname = 1;
    }
    
    // 시스템 정보 출력
    print_system_info(&opts);
    
    return 0;
}