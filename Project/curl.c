#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <ctype.h>
#include <time.h>

#define BUFFER_SIZE 8192
#define MAX_URL_SIZE 2048
#define MAX_HEADER_SIZE 4096
#define MAX_HEADERS 50
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
    char status_text[128];
    char content_type[128];
    int chunked;
    char location[512];  // 리다이렉션용
} http_response_t;

typedef struct {
    char method[16];
    char *data;
    char *headers[MAX_HEADERS];
    int header_count;
    char *output_file;
    int include_headers;
    int verbose;
    int silent;
    int follow_redirects;
    int max_redirects;
    char *user_agent;
} curl_options_t;

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
        strcpy(info->protocol, "http");
        info->port = HTTP_PORT;
    }
    
    // 호스트명과 포트 추출
    const char *slash = strchr(ptr, '/');
    const char *colon = strchr(ptr, ':');
    
    if (colon && (slash == NULL || colon < slash)) {
        int hostname_len = colon - ptr;
        strncpy(info->hostname, ptr, hostname_len);
        info->hostname[hostname_len] = '\0';
        
        info->port = atoi(colon + 1);
        ptr = slash ? slash : ptr + strlen(ptr);
    } else {
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

// HTTP 응답 파싱
void parse_http_response(const char *response, http_response_t *info) {
    char *line, *response_copy;
    char *saveptr;
    
    response_copy = strdup(response);
    
    // 첫 번째 줄에서 상태 코드 추출
    line = strtok_r(response_copy, "\r\n", &saveptr);
    if (line && strncmp(line, "HTTP/", 5) == 0) {
        sscanf(line, "HTTP/%*s %d %127[^\r\n]", &info->status_code, info->status_text);
    }
    
    // 나머지 헤더 파싱
    while ((line = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
        if (strncasecmp(line, "Content-Length:", 15) == 0) {
            info->content_length = atol(line + 15);
        } else if (strncasecmp(line, "Content-Type:", 13) == 0) {
            sscanf(line + 13, " %127[^\r\n]", info->content_type);
        } else if (strncasecmp(line, "Transfer-Encoding:", 18) == 0) {
            if (strstr(line + 18, "chunked")) {
                info->chunked = 1;
            }
        } else if (strncasecmp(line, "Location:", 9) == 0) {
            sscanf(line + 9, " %511[^\r\n]", info->location);
        }
    }
    
    free(response_copy);
}

// HTTP 요청 생성
void build_http_request(const url_info_t *url_info, const curl_options_t *opts, char *request, size_t size) {
    int len = 0;
    
    // 요청 라인
    len += snprintf(request + len, size - len, "%s %s HTTP/1.1\r\n", opts->method, url_info->path);
    
    // 기본 헤더
    len += snprintf(request + len, size - len, "Host: %s\r\n", url_info->hostname);
    
    // User-Agent
    if (opts->user_agent) {
        len += snprintf(request + len, size - len, "User-Agent: %s\r\n", opts->user_agent);
    } else {
        len += snprintf(request + len, size - len, "User-Agent: curl/1.0\r\n");
    }
    
    // 사용자 정의 헤더
    for (int i = 0; i < opts->header_count; i++) {
        len += snprintf(request + len, size - len, "%s\r\n", opts->headers[i]);
    }
    
    // POST 데이터가 있는 경우
    if (opts->data) {
        len += snprintf(request + len, size - len, "Content-Type: application/x-www-form-urlencoded\r\n");
        len += snprintf(request + len, size - len, "Content-Length: %zu\r\n", strlen(opts->data));
    }
    
    len += snprintf(request + len, size - len, "Connection: close\r\n");
    len += snprintf(request + len, size - len, "\r\n");
    
    // POST 데이터 추가
    if (opts->data) {
        len += snprintf(request + len, size - len, "%s", opts->data);
    }
}

// HTTP 요청 처리
int process_http_request(const url_info_t *url_info, const curl_options_t *opts, FILE *output) {
    int sockfd;
    char request[4096];
    char buffer[BUFFER_SIZE];
    char header_buffer[MAX_HEADER_SIZE];
    http_response_t response_info = {0};
    int header_received = 0;
    int header_len = 0;
    
    if (opts->verbose) {
        printf("* Connecting to %s:%d...\n", url_info->hostname, url_info->port);
    }
    
    sockfd = connect_to_server(url_info->hostname, url_info->port);
    if (sockfd < 0) {
        return -1;
    }
    
    if (opts->verbose) {
        printf("* Connected to %s (%s) port %d\n", url_info->hostname, url_info->hostname, url_info->port);
    }
    
    // HTTP 요청 생성
    build_http_request(url_info, opts, request, sizeof(request));
    
    if (opts->verbose) {
        printf("> %s", request);
    }
    
    // 요청 전송
    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("send");
        close(sockfd);
        return -1;
    }
    
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
                
                if (opts->verbose) {
                    printf("< HTTP/1.1 %d %s\n", response_info.status_code, response_info.status_text);
                    
                    // 헤더 출력
                    char *line, *header_copy = strdup(header_buffer);
                    char *saveptr;
                    strtok_r(header_copy, "\r\n", &saveptr); // 첫 번째 줄 건너뛰기
                    while ((line = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
                        if (strlen(line) > 0) {
                            printf("< %s\n", line);
                        }
                    }
                    free(header_copy);
                    printf("< \n");
                }
                
                // 헤더 포함 옵션
                if (opts->include_headers) {
                    int header_only_len = (header_end - header_buffer) + 4;
                    fwrite(header_buffer, 1, header_only_len, output);
                }
                
                // 헤더 이후의 데이터 처리
                int body_start = (header_end - header_buffer) + 4;
                int remaining_data = header_len - body_start;
                if (remaining_data > 0) {
                    fwrite(header_buffer + body_start, 1, remaining_data, output);
                }
            }
        } else {
            // 바디 데이터 출력
            fwrite(buffer, 1, bytes_received, output);
        }
    }
    
    close(sockfd);
    
    // 리다이렉션 처리
    if (opts->follow_redirects && (response_info.status_code == 301 || response_info.status_code == 302 || 
        response_info.status_code == 303 || response_info.status_code == 307 || response_info.status_code == 308)) {
        if (strlen(response_info.location) > 0) {
            if (opts->verbose) {
                printf("* Following redirect to %s\n", response_info.location);
            }
            url_info_t new_url;
            parse_url(response_info.location, &new_url);
            return process_http_request(&new_url, opts, output);
        }
    }
    
    return response_info.status_code;
}

// HTTPS 요청 처리
int process_https_request(const url_info_t *url_info, const curl_options_t *opts, FILE *output) {
    int sockfd;
    SSL_CTX *ctx;
    SSL *ssl;
    char request[4096];
    char buffer[BUFFER_SIZE];
    char header_buffer[MAX_HEADER_SIZE];
    http_response_t response_info = {0};
    int header_received = 0;
    int header_len = 0;
    
    // SSL 초기화
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    if (opts->verbose) {
        printf("* Connecting to %s:%d...\n", url_info->hostname, url_info->port);
    }
    
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
    
    if (opts->verbose) {
        printf("* SSL connection established\n");
    }
    
    // HTTPS 요청 생성
    build_http_request(url_info, opts, request, sizeof(request));
    
    if (opts->verbose) {
        printf("> %s", request);
    }
    
    // 요청 전송
    if (SSL_write(ssl, request, strlen(request)) < 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    
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
                
                if (opts->verbose) {
                    printf("< HTTP/1.1 %d %s\n", response_info.status_code, response_info.status_text);
                    
                    // 헤더 출력
                    char *line, *header_copy = strdup(header_buffer);
                    char *saveptr;
                    strtok_r(header_copy, "\r\n", &saveptr); // 첫 번째 줄 건너뛰기
                    while ((line = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
                        if (strlen(line) > 0) {
                            printf("< %s\n", line);
                        }
                    }
                    free(header_copy);
                    printf("< \n");
                }
                
                // 헤더 포함 옵션
                if (opts->include_headers) {
                    int header_only_len = (header_end - header_buffer) + 4;
                    fwrite(header_buffer, 1, header_only_len, output);
                }
                
                // 헤더 이후의 데이터 처리
                int body_start = (header_end - header_buffer) + 4;
                int remaining_data = header_len - body_start;
                if (remaining_data > 0) {
                    fwrite(header_buffer + body_start, 1, remaining_data, output);
                }
            }
        } else {
            // 바디 데이터 출력
            fwrite(buffer, 1, bytes_received, output);
        }
    }
    
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sockfd);
    
    // 리다이렉션 처리
    if (opts->follow_redirects && (response_info.status_code == 301 || response_info.status_code == 302 || 
        response_info.status_code == 303 || response_info.status_code == 307 || response_info.status_code == 308)) {
        if (strlen(response_info.location) > 0) {
            if (opts->verbose) {
                printf("* Following redirect to %s\n", response_info.location);
            }
            url_info_t new_url;
            parse_url(response_info.location, &new_url);
            if (strcmp(new_url.protocol, "https") == 0) {
                return process_https_request(&new_url, opts, output);
            } else {
                return process_http_request(&new_url, opts, output);
            }
        }
    }
    
    return response_info.status_code;
}

// 사용법 출력
void print_usage(const char *program_name) {
    printf("Usage: %s [options...] <url>\n", program_name);
    printf("Transfer data from or to a server\n\n");
    printf("Options:\n");
    printf("  -X, --request <method>     HTTP method (GET, POST, PUT, DELETE, etc.)\n");
    printf("  -d, --data <data>          HTTP POST data\n");
    printf("  -H, --header <header>      Custom header to pass to server\n");
    printf("  -o, --output <file>        Write output to file instead of stdout\n");
    printf("  -i, --include              Include HTTP headers in output\n");
    printf("  -v, --verbose              Make the operation more talkative\n");
    printf("  -s, --silent               Silent mode\n");
    printf("  -L, --location             Follow redirects\n");
    printf("  -A, --user-agent <agent>   User-Agent to send to server\n");
    printf("  -h, --help                 Display this help and exit\n");
    printf("\nExamples:\n");
    printf("  %s http://example.com\n", program_name);
    printf("  %s -X POST -d \"name=value\" http://example.com/api\n", program_name);
    printf("  %s -H \"Authorization: Bearer token\" https://api.example.com\n", program_name);
    printf("  %s -o output.html -L http://example.com\n", program_name);
}

int main(int argc, char *argv[]) {
    char *url = NULL;
    curl_options_t opts = {0};
    url_info_t url_info;
    FILE *output = stdout;
    int result;
    
    // 기본값 설정
    strcpy(opts.method, "GET");
    opts.follow_redirects = 0;
    opts.max_redirects = 10;
    
    // 인자 파싱
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if ((strcmp(argv[i], "-X") == 0 || strcmp(argv[i], "--request") == 0) && i + 1 < argc) {
            strncpy(opts.method, argv[++i], sizeof(opts.method) - 1);
        } else if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0) && i + 1 < argc) {
            opts.data = argv[++i];
            if (strcmp(opts.method, "GET") == 0) {
                strcpy(opts.method, "POST");
            }
        } else if ((strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--header") == 0) && i + 1 < argc) {
            if (opts.header_count < MAX_HEADERS) {
                opts.headers[opts.header_count++] = argv[++i];
            }
        } else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i + 1 < argc) {
            opts.output_file = argv[++i];
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--include") == 0) {
            opts.include_headers = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            opts.verbose = 1;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--silent") == 0) {
            opts.silent = 1;
        } else if (strcmp(argv[i], "-L") == 0 || strcmp(argv[i], "--location") == 0) {
            opts.follow_redirects = 1;
        } else if ((strcmp(argv[i], "-A") == 0 || strcmp(argv[i], "--user-agent") == 0) && i + 1 < argc) {
            opts.user_agent = argv[++i];
        } else if (argv[i][0] != '-') {
            url = argv[i];
        }
    }
    
    if (!url) {
        fprintf(stderr, "Error: No URL specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // URL 파싱
    if (parse_url(url, &url_info) != 0) {
        fprintf(stderr, "Error: Invalid URL format\n");
        return 1;
    }
    
    // 출력 파일 열기
    if (opts.output_file) {
        output = fopen(opts.output_file, "wb");
        if (!output) {
            perror("fopen");
            return 1;
        }
    }
    
    // 요청 처리
    if (strcmp(url_info.protocol, "https") == 0) {
        result = process_https_request(&url_info, &opts, output);
    } else {
        result = process_http_request(&url_info, &opts, output);
    }
    
    if (output != stdout) {
        fclose(output);
    }
    
    if (!opts.silent && opts.verbose) {
        printf("* Request completed with status code: %d\n", result);
    }
    
    return (result >= 200 && result < 300) ? 0 : 1;
}