#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <mntent.h>

// 파일시스템 정보를 저장할 구조체
typedef struct {
    char filesystem[256];
    char mountpoint[256];
    char fstype[64];
    unsigned long long total;
    unsigned long long used;
    unsigned long long available;
    int use_percent;
} FSInfo;

// 바이트를 사람이 읽기 쉬운 형태로 변환
void format_size(unsigned long long bytes, char* buffer, int human_readable) {
    if (human_readable) {
        if (bytes >= 1099511627776ULL) { // 1TB
            snprintf(buffer, 32, "%.1fT", (double)bytes / 1099511627776.0);
        } else if (bytes >= 1073741824ULL) { // 1GB
            snprintf(buffer, 32, "%.1fG", (double)bytes / 1073741824.0);
        } else if (bytes >= 1048576ULL) { // 1MB
            snprintf(buffer, 32, "%.1fM", (double)bytes / 1048576.0);
        } else if (bytes >= 1024ULL) { // 1KB
            snprintf(buffer, 32, "%.1fK", (double)bytes / 1024.0);
        } else {
            snprintf(buffer, 32, "%lluB", bytes);
        }
    } else {
        // 1K 블록 단위로 출력 (기본값)
        snprintf(buffer, 32, "%llu", bytes / 1024);
    }
}

// 특정 파일시스템 타입을 제외할지 확인
int should_skip_fstype(const char* fstype) {
    const char* skip_types[] = {
        "proc", "sysfs", "devtmpfs", "devpts", "tmpfs", "securityfs",
        "cgroup", "pstore", "efivarfs", "bpf", "cgroup2", "hugetlbfs",
        "debugfs", "tracefs", "fusectl", "configfs", "ramfs", "autofs",
        "rpc_pipefs", "nfsd", NULL
    };
    
    for (int i = 0; skip_types[i]; i++) {
        if (strcmp(fstype, skip_types[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// 파일시스템 정보 가져오기
int get_filesystem_info(const char* mountpoint, FSInfo* fs_info) {
    struct statvfs vfs;
    
    if (statvfs(mountpoint, &vfs) != 0) {
        return -1;
    }
    
    // 블록 크기 계산
    unsigned long long block_size = vfs.f_frsize ? vfs.f_frsize : vfs.f_bsize;
    
    // 총 공간, 사용 공간, 사용 가능 공간 계산
    fs_info->total = vfs.f_blocks * block_size;
    fs_info->available = vfs.f_bavail * block_size;
    fs_info->used = fs_info->total - (vfs.f_bfree * block_size);
    
    // 사용 퍼센트 계산
    if (fs_info->total > 0) {
        fs_info->use_percent = (int)((fs_info->used * 100) / fs_info->total);
    } else {
        fs_info->use_percent = 0;
    }
    
    return 0;
}

// 마운트된 파일시스템 목록 출력
void print_filesystems(int human_readable, int show_all, const char* specific_path) {
    FILE* mounts;
    struct mntent* entry;
    FSInfo fs_info;
    char total_str[32], used_str[32], avail_str[32];
    
    // 헤더 출력
    if (human_readable) {
        printf("%-20s %6s %6s %6s %4s %s\n", 
               "Filesystem", "Size", "Used", "Avail", "Use%", "Mounted on");
    } else {
        printf("%-20s %10s %10s %10s %4s %s\n", 
               "Filesystem", "1K-blocks", "Used", "Available", "Use%", "Mounted on");
    }
    
    // 특정 경로가 지정된 경우
    if (specific_path) {
        struct stat path_stat;
        if (stat(specific_path, &path_stat) == 0) {
            // 경로가 존재하는 경우, 해당 경로의 파일시스템 정보만 출력
            if (get_filesystem_info(specific_path, &fs_info) == 0) {
                format_size(fs_info.total, total_str, human_readable);
                format_size(fs_info.used, used_str, human_readable);
                format_size(fs_info.available, avail_str, human_readable);
                
                printf("%-20s %10s %10s %10s %3d%% %s\n",
                       "filesystem", total_str, used_str, avail_str,
                       fs_info.use_percent, specific_path);
            }
        } else {
            fprintf(stderr, "df: %s: No such file or directory\n", specific_path);
        }
        return;
    }
    
    // /proc/mounts 파일 열기
    mounts = setmntent("/proc/mounts", "r");
    if (!mounts) {
        perror("Cannot open /proc/mounts");
        return;
    }
    
    // 각 마운트 포인트에 대해 정보 수집
    while ((entry = getmntent(mounts)) != NULL) {
        // 가상 파일시스템 제외 (show_all 옵션이 없는 경우)
        if (!show_all && should_skip_fstype(entry->mnt_type)) {
            continue;
        }
        
        // 파일시스템 정보 가져오기
        if (get_filesystem_info(entry->mnt_dir, &fs_info) != 0) {
            continue;
        }
        
        // 0 크기 파일시스템 제외
        if (!show_all && fs_info.total == 0) {
            continue;
        }
        
        // 구조체에 추가 정보 저장
        strncpy(fs_info.filesystem, entry->mnt_fsname, sizeof(fs_info.filesystem) - 1);
        strncpy(fs_info.mountpoint, entry->mnt_dir, sizeof(fs_info.mountpoint) - 1);
        strncpy(fs_info.fstype, entry->mnt_type, sizeof(fs_info.fstype) - 1);
        
        // 크기 포맷팅
        format_size(fs_info.total, total_str, human_readable);
        format_size(fs_info.used, used_str, human_readable);
        format_size(fs_info.available, avail_str, human_readable);
        
        // 출력
        printf("%-20s %10s %10s %10s %3d%% %s\n",
               fs_info.filesystem, total_str, used_str, avail_str,
               fs_info.use_percent, fs_info.mountpoint);
    }
    
    endmntent(mounts);
}

// 도움말 출력
void print_help() {
    printf("Usage: df [OPTION]... [FILE]...\n");
    printf("Show information about the file system on which each FILE resides,\n");
    printf("or all file systems by default.\n\n");
    printf("Options:\n");
    printf("  -a, --all             include dummy file systems\n");
    printf("  -h, --human-readable  print sizes in human readable format (e.g., 1K 234M 2G)\n");
    printf("  -k                    like --block-size=1K\n");
    printf("  -T, --print-type      print file system type\n");
    printf("      --help            display this help and exit\n");
    printf("      --version         output version information and exit\n");
}

// 버전 정보 출력
void print_version() {
    printf("df (custom implementation) 1.0\n");
    printf("Written for terminal implementation project.\n");
}

int main(int argc, char* argv[]) {
    int human_readable = 0;
    int show_all = 0;
    int show_type = 0;
    char* specific_path = NULL;
    
    // 명령행 인수 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--human-readable") == 0) {
            human_readable = 1;
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) {
            show_all = 1;
        } else if (strcmp(argv[i], "-k") == 0) {
            human_readable = 0; // 명시적으로 KB 단위
        } else if (strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--print-type") == 0) {
            show_type = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        } else if (strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (argv[i][0] != '-') {
            // 경로 인수
            specific_path = argv[i];
        } else {
            fprintf(stderr, "df: invalid option -- '%s'\n", argv[i]);
            fprintf(stderr, "Try 'df --help' for more information.\n");
            return 1;
        }
    }
    
    // 파일시스템 정보 출력
    print_filesystems(human_readable, show_all, specific_path);
    
    return 0;
}