#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char type(mode_t);
char *perm(mode_t);
void printStat(char*, char*, struct stat*);

/* 디렉터리 내용을 자세히 리스트한다. */
int main(int argc, char **argv) {
    DIR *dp;
    char *dir;
    struct stat st;
    struct dirent *d;
    char path[BUFSIZ+1];
    
    if (argc == 1)
        dir = ".";
    else
        dir = argv[1];
    
    if ((dp = opendir(dir)) == NULL) { // 디렉터리 열기
        perror(dir);
        exit(1);
    }
    
    while ((d = readdir(dp)) != NULL) { // 디렉터리의 각 파일에 대해
        sprintf(path, "%s/%s", dir, d->d_name); // 파일 경로명 만들기
        if (lstat(path, &st) < 0) { // 파일 상태 정보 가져오기
            perror(path);
            continue;
        }
        printStat(path, d->d_name, &st); // 상태 정보 출력
        putchar('\n');
    }
    
    closedir(dp);
    exit(0);
}

/* 파일 상태 정보를 출력 */
void printStat(char *pathname, char *file, struct stat *st) {
    printf("%5lld ", (long long)st->st_blocks); // long long 타입으로 수정
    printf("%c%s ", type(st->st_mode), perm(st->st_mode));
    printf("%3d ", (int)st->st_nlink);
    printf("%s %s ", getpwuid(st->st_uid)->pw_name,
                     getgrgid(st->st_gid)->gr_name);
    printf("%9lld ", (long long)st->st_size); // long long 타입으로 수정
    printf("%.12s ", ctime(&st->st_mtime)+4);
    printf("%s", file);
}

/* 파일 타입을 리턴 */
char type(mode_t mode) {
    if (S_ISREG(mode)) return('-');
    if (S_ISDIR(mode)) return('d');
    if (S_ISCHR(mode)) return('c');
    if (S_ISBLK(mode)) return('b');
    if (S_ISLNK(mode)) return('l');
    if (S_ISFIFO(mode)) return('p');
    if (S_ISSOCK(mode)) return('s');
    return('?');
}

/* 파일 사용권한을 리턴 */
char* perm(mode_t mode) {
    static char perms[10];
    
    perms[0] = (mode & S_IRUSR) ? 'r' : '-';
    perms[1] = (mode & S_IWUSR) ? 'w' : '-';
    perms[2] = (mode & S_IXUSR) ? 'x' : '-';
    perms[3] = (mode & S_IRGRP) ? 'r' : '-';
    perms[4] = (mode & S_IWGRP) ? 'w' : '-';
    perms[5] = (mode & S_IXGRP) ? 'x' : '-';
    perms[6] = (mode & S_IROTH) ? 'r' : '-';
    perms[7] = (mode & S_IWOTH) ? 'w' : '-';
    perms[8] = (mode & S_IXOTH) ? 'x' : '-';
    perms[9] = '\0';
    
    return perms;
}