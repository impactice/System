#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <ctype.h>
#include <time.h>

#define BUFFER_SIZE 8192
#define MAX_URL_SIZE 2048
#define MAX_HEADER_SIZE 4096
#define HTTP_PORT 80
#define HTTPS_PORT 443

typedef struct {
    char protocol[10];
    char hostname[256];
    int port;
    char path[1024];
} url_info_t;

typedef struct {
    long content_length;
    int status_code;
    char filename[256];
    char content_type[128];
    int chunked;
} http_response_t;

// URL 파싱
int parse_url(const char *url, url_info_t *info) {
    const char *ptr = url;
    
    // 프로토콜 추출
    if (strncmp(ptr, "http://", 7) == 0) {
        strcpy(info->protocol, "http");
        info->port = HTTP_PORT;
        ptr += 7;
    } else if (strncmp(ptr, "https://", 8) == 0) {
        strcpy(info->protocol, "https");
        info->port = HTTPS_PORT;
        ptr += 8;
    } else {
        // 프로토콜이 없으면 http로 가정
        strcpy(info->protocol, "http");
        info->port = HTTP_PORT;
    }
    
    // 호스트명과 포트 추출
    const char *slash = strchr(ptr, '/');
    const char *colon = strchr(ptr, ':');
    
    if (colon && (slash == NULL || colon < slash)) {
        // 포트가 지정된 경우
        int hostname_len = colon - ptr;
        strncpy(info->hostname, ptr, hostname_len);
        info->hostname[hostname_len] = '\0';
        
        info->port = atoi(colon + 1);
        ptr = slash ? slash : ptr + strlen(ptr);
    } else {
        // 포트가 지정되지 않은 경우
        int hostname_len = slash ? (slash - ptr) : strlen(ptr);
        strncpy(info->hostname, ptr, hostname_len);
        info->hostname[hostname_len] = '\0';
        ptr = slash ? slash : ptr + strlen(ptr);
    }
    
    // 경로 추출
    if (*ptr == '\0') {
        strcpy(info->path, "/");
    } else {
        strcpy(info->path, ptr);
    }
    
    return 0;
}

// 파일명 추출
void extract_filename(const char *url, const char *path, char *filename) {
    const char *last_slash = strrchr(path, '/');
    if (last_slash && strlen(last_slash + 1) > 0) {
        strcpy(filename, last_slash + 1);
    } else {
        // URL에서 호스트명 추출하여 기본 파일명으로 사용
        const char *host_start = strstr(url, "://");
        if (host_start) {
            host_start += 3;
            const char *host_end = strchr(host_start, '/');
            if (host_end) {
                int len = host_end - host_start;
                strncpy(filename, host_start, len);
                filename[len] = '\0';
            } else {
                strcpy(filename, host_start);
            }
        } else {
            strcpy(filename, "index.html");
        }
    }
}

// 소켓 연결
int connect_to_server(const char *hostname, int port) {
    struct sockaddr_in server_addr;
    struct hostent *server;
    int sockfd;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "Host not found: %s\n", hostname);
        close(sockfd);
        return -1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// HTTP 헤더 파싱
void parse_http_response(const char *response, http_response_t *info) {
    char *line, *response_copy;
    char *saveptr;
    
    response_copy = strdup(response);
    
    // 첫 번째 줄에서 상태 코드 추출
    line = strtok_r(response_copy, "\r\n", &saveptr);
    if (line && strncmp(line, "HTTP/", 5) == 0) {
        sscanf(line, "HTTP/%*s %d", &info->status_code);
    }
    
    // 나머지 헤더 파싱
    while ((line = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
        if (strncasecmp(line, "Content-Length:", 15) == 0) {
            info->content_length = atol(line + 15);
        } else if (strncasecmp(line, "Content-Type:", 13) == 0) {
            sscanf(line + 13, "%127s", info->content_type);
        } else if (strncasecmp(line, "Transfer-Encoding:", 18) == 0) {
            if (strstr(line + 18, "chunked")) {
                info->chunked = 1;
            }
        }
    }
    
    free(response_copy);
}

// 청크 데이터 디코딩
int decode_chunked_data(const char *data, int data_len, char *output, int output_size) {
    int i = 0, output_len = 0;
    
    while (i < data_len && output_len < output_size - 1) {
        // 청크 크기 읽기
        char chunk_size_str[16] = {0};
        int j = 0;
        
        while (i < data_len && data[i] != '\r' && j < 15) {
            chunk_size_str[j++] = data[i++];
        }
        
        if (i >= data_len - 1) break;
        
        // \r\n 건너뛰기
        i += 2;
        
        int chunk_size = strtol(chunk_size_str, NULL, 16);
        if (chunk_size == 0) break; // 마지막 청크
        
        // 청크 데이터 복사
        int copy_len = (chunk_size < output_size - output_len - 1) ? 
                       chunk_size : output_size - output_len - 1;
        memcpy(output + output_len, data + i, copy_len);
        output_len += copy_len;
        i += chunk_size + 2; // 청크 데이터 + \r\n
    }
    
    return output_len;
}

// 진행률 표시
void show_progress(long downloaded, long total, time_t start_time) {
    time_t current_time = time(NULL);
    double elapsed = difftime(current_time, start_time);
    
    if (total > 0) {
        int percent = (int)((downloaded * 100) / total);
        double speed = downloaded / (elapsed > 0 ? elapsed : 1);
        
        printf("\r%ld/%ld bytes (%d%%) downloaded at %.1f KB/s", 
               downloaded, total, percent, speed / 1024);
    } else {
        double speed = downloaded / (elapsed > 0 ? elapsed : 1);
        printf("\r%ld bytes downloaded at %.1f KB/s", downloaded, speed / 1024);
    }
    fflush(stdout);
}

// HTTP 다운로드
int download_http(const url_info_t *url_info, const char *output_filename) {
    int sockfd;
    char request[1024];
    char buffer[BUFFER_SIZE];
    char header_buffer[MAX_HEADER_SIZE];
    FILE *output_file;
    http_response_t response_info = {0};
    long downloaded = 0;
    int header_received = 0;
    int header_len = 0;
    time_t start_time;
    
    printf("Connecting to %s:%d...\n", url_info->hostname, url_info->port);
    
    sockfd = connect_to_server(url_info->hostname, url_info->port);
    if (sockfd < 0) {
        return -1;
    }
    
    printf("Connected.\n");
    
    // HTTP 요청 생성
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: wget/1.0\r\n"
             "Connection: close\r\n"
             "\r\n",
             url_info->path, url_info->hostname);
    
    // 요청 전송
    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("send");
        close(sockfd);
        return -1;
    }
    
    printf("HTTP request sent, awaiting response...\n");
    
    // 출력 파일 열기
    output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("fopen");
        close(sockfd);
        return -1;
    }
    
    start_time = time(NULL);
    
    // 응답 수신
    while (1) {
        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) break;
        
        buffer[bytes_received] = '\0';
        
        if (!header_received) {
            // 헤더와 바디를 분리
            int copy_len = (header_len + bytes_received < MAX_HEADER_SIZE - 1) ?
                          bytes_received : MAX_HEADER_SIZE - 1 - header_len;
            memcpy(header_buffer + header_len, buffer, copy_len);
            header_len += copy_len;
            header_buffer[header_len] = '\0';
            
            char *header_end = strstr(header_buffer, "\r\n\r\n");
            if (header_end) {
                header_received = 1;
                parse_http_response(header_buffer, &response_info);
                
                printf("Response: %d\n", response_info.status_code);
                if (response_info.content_length > 0) {
                    printf("Length: %ld bytes\n", response_info.content_length);
                }
                printf("Saving to: %s\n\n", output_filename);
                
                if (response_info.status_code != 200) {
                    printf("HTTP Error: %d\n", response_info.status_code);
                    fclose(output_file);
                    close(sockfd);
                    unlink(output_filename);
                    return -1;
                }
                
                // 헤더 이후의 데이터 처리
                int body_start = (header_end - header_buffer) + 4;
                int remaining_data = header_len - body_start;
                if (remaining_data > 0) {
                    fwrite(header_buffer + body_start, 1, remaining_data, output_file);
                    downloaded += remaining_data;
                }
            }
        } else {
            // 바디 데이터 저장
            fwrite(buffer, 1, bytes_received, output_file);
            downloaded += bytes_received;
        }
        
        if (header_received) {
            show_progress(downloaded, response_info.content_length, start_time);
        }
    }
    
    printf("\n\nDownload completed: %s (%ld bytes)\n", output_filename, downloaded);
    
    fclose(output_file);
    close(sockfd);
    return 0;
}

// HTTPS 다운로드
int download_https(const url_info_t *url_info, const char *output_filename) {
    int sockfd;
    SSL_CTX *ctx;
    SSL *ssl;
    char request[1024];
    char buffer[BUFFER_SIZE];
    char header_buffer[MAX_HEADER_SIZE];
    FILE *output_file;
    http_response_t response_info = {0};
    long downloaded = 0;
    int header_received = 0;
    int header_len = 0;
    time_t start_time;
    
    // SSL 초기화
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    printf("Connecting to %s:%d...\n", url_info->hostname, url_info->port);
    
    sockfd = connect_to_server(url_info->hostname, url_info->port);
    if (sockfd < 0) {
        SSL_CTX_free(ctx);
        return -1;
    }
    
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    
    if (SSL_connect(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    
    printf("SSL connection established.\n");
    
    // HTTPS 요청 생성
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: wget/1.0\r\n"
             "Connection: close\r\n"
             "\r\n",
             url_info->path, url_info->hostname);
    
    // 요청 전송
    if (SSL_write(ssl, request, strlen(request)) < 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    
    printf("HTTPS request sent, awaiting response...\n");
    
    // 출력 파일 열기
    output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("fopen");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    
    start_time = time(NULL);
    
    // 응답 수신
    while (1) {
        int bytes_received = SSL_read(ssl, buffer, BUFFER_SIZE - 1);
        if (bytes_received <= 0) break;
        
        buffer[bytes_received] = '\0';
        
        if (!header_received) {
            // 헤더와 바디를 분리
            int copy_len = (header_len + bytes_received < MAX_HEADER_SIZE - 1) ?
                          bytes_received : MAX_HEADER_SIZE - 1 - header_len;
            memcpy(header_buffer + header_len, buffer, copy_len);
            header_len += copy_len;
            header_buffer[header_len] = '\0';
            
            char *header_end = strstr(header_buffer, "\r\n\r\n");
            if (header_end) {
                header_received = 1;
                parse_http_response(header_buffer, &response_info);
                
                printf("Response: %d\n", response_info.status_code);
                if (response_info.content_length > 0) {
                    printf("Length: %ld bytes\n", response_info.content_length);
                }
                printf("Saving to: %s\n\n", output_filename);
                
                if (response_info.status_code != 200) {
                    printf("HTTP Error: %d\n", response_info.status_code);
                    fclose(output_file);
                    SSL_free(ssl);
                    SSL_CTX_free(ctx);
                    close(sockfd);
                    unlink(output_filename);
                    return -1;
                }
                
                // 헤더 이후의 데이터 처리
                int body_start = (header_end - header_buffer) + 4;
                int remaining_data = header_len - body_start;
                if (remaining_data > 0) {
                    fwrite(header_buffer + body_start, 1, remaining_data, output_file);
                    downloaded += remaining_data;
                }
            }
        } else {
            // 바디 데이터 저장
            fwrite(buffer, 1, bytes_received, output_file);
            downloaded += bytes_received;
        }
        
        if (header_received) {
            show_progress(downloaded, response_info.content_length, start_time);
        }
    }
    
    printf("\n\nDownload completed: %s (%ld bytes)\n", output_filename, downloaded);
    
    fclose(output_file);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sockfd);
    return 0;
}

// 사용법 출력
void print_usage(const char *program_name) {
    printf("Usage: %s [OPTION]... [URL]...\n", program_name);
    printf("Download files from web servers via HTTP/HTTPS.\n\n");
    printf("Options:\n");
    printf("  -O file    save document to file\n");
    printf("  -h, --help display this help and exit\n");
    printf("\nExamples:\n");
    printf("  %s http://example.com/file.txt\n", program_name);
    printf("  %s -O myfile.txt https://example.com/file.txt\n", program_name);
}

int main(int argc, char *argv[]) {
    char *url = NULL;
    char *output_filename = NULL;
    char default_filename[256];
    url_info_t url_info;
    int i;
    
    // 인자 파싱
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-O") == 0 && i + 1 < argc) {
            output_filename = argv[++i];
        } else if (argv[i][0] != '-') {
            url = argv[i];
        }
    }
    
    if (!url) {
        printf("Error: No URL specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // URL 파싱
    if (parse_url(url, &url_info) != 0) {
        printf("Error: Invalid URL format\n");
        return 1;
    }
    
    // 출력 파일명 결정
    if (!output_filename) {
        extract_filename(url, url_info.path, default_filename);
        output_filename = default_filename;
    }
    
    printf("URL: %s\n", url);
    printf("Protocol: %s\n", url_info.protocol);
    printf("Host: %s\n", url_info.hostname);
    printf("Port: %d\n", url_info.port);
    printf("Path: %s\n", url_info.path);
    printf("Output: %s\n\n", output_filename);
    
    // 프로토콜에 따라 다운로드
    if (strcmp(url_info.protocol, "https") == 0) {
        return download_https(&url_info, output_filename);
    } else {
        return download_http(&url_info, output_filename);
    }
}