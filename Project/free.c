#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 메모리 정보를 저장할 구조체
typedef struct {
    long total;
    long free;
    long available;
    long buffers;
    long cached;
    long swap_total;
    long swap_free;
} MemInfo;

// /proc/meminfo에서 특정 값을 추출하는 함수
long extract_value(const char* line, const char* key) {
    if (strncmp(line, key, strlen(key)) == 0) {
        char* ptr = strchr(line, ':');
        if (ptr) {
            ptr++; // ':' 다음으로 이동
            while (*ptr == ' ' || *ptr == '\t') ptr++; // 공백 건너뛰기
            return atol(ptr);
        }
    }
    return -1;
}

// /proc/meminfo 파일을 읽어서 메모리 정보 파싱
int get_memory_info(MemInfo* mem_info) {
    FILE* file = fopen("/proc/meminfo", "r");
    if (!file) {
        perror("Cannot open /proc/meminfo");
        return -1;
    }

    char line[256];
    long value;

    // 구조체 초기화
    memset(mem_info, 0, sizeof(MemInfo));

    while (fgets(line, sizeof(line), file)) {
        if ((value = extract_value(line, "MemTotal")) != -1) {
            mem_info->total = value;
        } else if ((value = extract_value(line, "MemFree")) != -1) {
            mem_info->free = value;
        } else if ((value = extract_value(line, "MemAvailable")) != -1) {
            mem_info->available = value;
        } else if ((value = extract_value(line, "Buffers")) != -1) {
            mem_info->buffers = value;
        } else if ((value = extract_value(line, "Cached")) != -1) {
            mem_info->cached = value;
        } else if ((value = extract_value(line, "SwapTotal")) != -1) {
            mem_info->swap_total = value;
        } else if ((value = extract_value(line, "SwapFree")) != -1) {
            mem_info->swap_free = value;
        }
    }

    fclose(file);
    return 0;
}

// KB를 다른 단위로 변환하는 함수
void print_memory_size(long kb, int human_readable) {
    if (human_readable) {
        if (kb >= 1024 * 1024) {
            printf("%6.1fG", (double)kb / (1024 * 1024));
        } else if (kb >= 1024) {
            printf("%6.1fM", (double)kb / 1024);
        } else {
            printf("%6ldK", kb);
        }
    } else {
        printf("%12ld", kb);
    }
}

// 메모리 정보를 출력하는 함수
void print_memory_info(MemInfo* mem_info, int human_readable, int show_buffers) {
    long used = mem_info->total - mem_info->free;
    long used_without_cache = used - mem_info->buffers - mem_info->cached;
    long swap_used = mem_info->swap_total - mem_info->swap_free;

    // 헤더 출력
    if (human_readable) {
        printf("%-12s %7s %7s %7s %7s %7s %7s\n", 
               "", "total", "used", "free", "shared", "buff/cache", "available");
    } else {
        printf("%-12s %12s %12s %12s %12s %12s %12s\n", 
               "", "total", "used", "free", "shared", "buff/cache", "available");
    }

    // 메모리 정보 출력
    printf("%-12s ", "Mem:");
    print_memory_size(mem_info->total, human_readable);
    print_memory_size(used, human_readable);
    print_memory_size(mem_info->free, human_readable);
    print_memory_size(0, human_readable); // shared (정확한 값을 얻기 어려우므로 0으로 표시)
    print_memory_size(mem_info->buffers + mem_info->cached, human_readable);
    print_memory_size(mem_info->available, human_readable);
    printf("\n");

    // 버퍼/캐시 정보 출력 (옵션)
    if (show_buffers) {
        printf("%-12s ", "Buffers:");
        print_memory_size(0, human_readable);
        print_memory_size(0, human_readable);
        print_memory_size(mem_info->buffers, human_readable);
        printf("\n");

        printf("%-12s ", "Cache:");
        print_memory_size(0, human_readable);
        print_memory_size(0, human_readable);
        print_memory_size(mem_info->cached, human_readable);
        printf("\n");
    }

    // 스왑 정보 출력
    printf("%-12s ", "Swap:");
    print_memory_size(mem_info->swap_total, human_readable);
    print_memory_size(swap_used, human_readable);
    print_memory_size(mem_info->swap_free, human_readable);
    printf("\n");
}

// 도움말 출력
void print_help() {
    printf("Usage: free [options]\n");
    printf("Options:\n");
    printf("  -h, --human     show human-readable output\n");
    printf("  -b, --bytes     show output in bytes\n");
    printf("  -k, --kilo      show output in kilobytes (default)\n");
    printf("  -m, --mega      show output in megabytes\n");
    printf("  -g, --giga      show output in gigabytes\n");
    printf("  -w, --wide      show buffers and cache separately\n");
    printf("  --help          display this help and exit\n");
}

int main(int argc, char* argv[]) {
    MemInfo mem_info;
    int human_readable = 0;
    int show_buffers = 0;
    int unit_divider = 1; // 1=KB (기본값), 1024=MB, 1024*1024=GB

    // 명령행 인수 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--human") == 0) {
            human_readable = 1;
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bytes") == 0) {
            unit_divider = 1;
            human_readable = 0;
        } else if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--kilo") == 0) {
            unit_divider = 1;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mega") == 0) {
            unit_divider = 1024;
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--giga") == 0) {
            unit_divider = 1024 * 1024;
        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--wide") == 0) {
            show_buffers = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_help();
            return 1;
        }
    }

    // 메모리 정보 가져오기
    if (get_memory_info(&mem_info) != 0) {
        return 1;
    }

    // 단위 변환 (bytes 옵션인 경우 1024를 곱해서 바이트로 변환)
    if (strcmp(argv[1] ? argv[1] : "", "-b") == 0 || strcmp(argv[1] ? argv[1] : "", "--bytes") == 0) {
        mem_info.total *= 1024;
        mem_info.free *= 1024;
        mem_info.available *= 1024;
        mem_info.buffers *= 1024;
        mem_info.cached *= 1024;
        mem_info.swap_total *= 1024;
        mem_info.swap_free *= 1024;
    } else if (unit_divider > 1) {
        mem_info.total /= unit_divider;
        mem_info.free /= unit_divider;
        mem_info.available /= unit_divider;
        mem_info.buffers /= unit_divider;
        mem_info.cached /= unit_divider;
        mem_info.swap_total /= unit_divider;
        mem_info.swap_free /= unit_divider;
    }

    // 메모리 정보 출력
    print_memory_info(&mem_info, human_readable, show_buffers);

    return 0;
}