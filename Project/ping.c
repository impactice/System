#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>

#define PACKET_SIZE 64
#define MAX_WAIT_TIME 5
#define PING_SLEEP_RATE 1000000

struct ping_packet {
    struct icmphdr hdr;
    char msg[PACKET_SIZE - sizeof(struct icmphdr)];
};

int sockfd;
int ping_count = 0;
int packets_sent = 0;
int packets_received = 0;
char *ping_target_ip;
char *ping_target_hostname;
struct sockaddr_in ping_addr;

// 체크섬 계산 함수
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    // 2바이트씩 더하기
    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }

    // 홀수 바이트가 남았을 경우
    if (len == 1) {
        sum += *(unsigned char*)buf << 8;
    }

    // 캐리 비트를 더하기
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    result = ~sum;
    return result;
}

// DNS 호스트명을 IP로 변환
char* dns_lookup(char *addr_host, struct sockaddr_in *addr_con) {
    struct hostent *host_entity;
    char *ip = malloc(16);
    
    if ((host_entity = gethostbyname(addr_host)) == NULL) {
        return NULL;
    }
    
    strcpy(ip, inet_ntoa(*((struct in_addr *)host_entity->h_addr)));
    (*addr_con).sin_family = host_entity->h_addrtype;
    (*addr_con).sin_port = 0;
    (*addr_con).sin_addr.s_addr = *((unsigned long *)host_entity->h_addr);
    
    return ip;
}

// 역방향 DNS 조회
char* reverse_dns_lookup(char *ip_addr) {
    struct sockaddr_in addr;
    socklen_t len;
    char buf[NI_MAXHOST], *ret_buf;
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    len = sizeof(struct sockaddr_in);
    
    if (getnameinfo((struct sockaddr *) &addr, len, buf, sizeof(buf), NULL, 0, NI_NAMEREQD)) {
        return NULL;
    }
    
    ret_buf = malloc(strlen(buf) + 1);
    strcpy(ret_buf, buf);
    return ret_buf;
}

// ping 패킷 전송
void send_ping(int ping_sockfd, struct sockaddr_in *ping_addr, char *ping_dom, char *ping_ip, char *rev_host) {
    int ttl_val = 64, msg_count = 0, flag = 1, msg_received_count = 0;
    
    struct ping_packet pckt;
    struct sockaddr_in r_addr;
    struct timespec time_start, time_end, tfs, tfe;
    long double rtt_msec = 0, total_msec = 0;
    struct timeval tv_out;
    tv_out.tv_sec = MAX_WAIT_TIME;
    tv_out.tv_usec = 0;

    clock_gettime(CLOCK_MONOTONIC, &tfs);

    // TTL 설정
    if (setsockopt(ping_sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0) {
        printf("\nTTL 설정 실패\n");
        return;
    }

    // 수신 타임아웃 설정
    setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof tv_out);

    printf("PING %s (%s) %d(%d) bytes of data.\n", ping_dom, ping_ip, PACKET_SIZE, PACKET_SIZE + 28);

    // ping 전송
    while (1) {
        // ping_count가 설정되어 있고 그 수만큼 보냈으면 종료
        if (ping_count > 0 && msg_count >= ping_count) {
            break;
        }

        flag = 1;

        // 패킷 초기화
        bzero(&pckt, sizeof(pckt));

        pckt.hdr.type = ICMP_ECHO;
        pckt.hdr.un.echo.id = getpid();
        
        for (int i = 0; i < sizeof(pckt.msg) - 1; i++) {
            pckt.msg[i] = i + '0';
        }
        
        pckt.msg[sizeof(pckt.msg) - 1] = 0;
        pckt.hdr.un.echo.sequence = msg_count++;
        pckt.hdr.checksum = 0;
        pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));

        usleep(PING_SLEEP_RATE);

        // 전송 시간 기록
        clock_gettime(CLOCK_MONOTONIC, &time_start);
        
        if (sendto(ping_sockfd, &pckt, sizeof(pckt), 0, (struct sockaddr*) ping_addr, sizeof(*ping_addr)) <= 0) {
            printf("\n패킷 전송 실패\n");
            flag = 0;
        }

        packets_sent++;

        // 응답 수신
        socklen_t addr_len = sizeof(r_addr);

        if (recvfrom(ping_sockfd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, &addr_len) <= 0 && msg_count > 1) {
            printf("\n패킷 수신 실패\n");
        } else {
            clock_gettime(CLOCK_MONOTONIC, &time_end);
            
            double timeElapsed = ((double)(time_end.tv_nsec - time_start.tv_nsec)) / 1000000.0;
            rtt_msec = (time_end.tv_sec - time_start.tv_sec) * 1000.0 + timeElapsed;
            
            if (flag) {
                if (!(pckt.hdr.type == 69 && pckt.hdr.code == 0)) {
                    printf("오류..패킷 수신됨 타입 %d 코드 %d\n", pckt.hdr.type, pckt.hdr.code);
                } else {
                    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.2Lf ms\n", 
                           PACKET_SIZE, ping_ip, msg_count, ttl_val, rtt_msec);
                    msg_received_count++;
                    packets_received++;
                }
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &tfe);
    double timeElapsed = ((double)(tfe.tv_nsec - tfs.tv_nsec)) / 1000000.0;
    total_msec = (tfe.tv_sec - tfs.tv_sec) * 1000.0 + timeElapsed;
    
    printf("\n--- %s ping statistics ---\n", ping_ip);
    printf("%d packets transmitted, %d received, %d%% packet loss, time %.0fms\n", 
           packets_sent, packets_received, 
           ((packets_sent - packets_received) / packets_sent) * 100, total_msec);
}

// 시그널 핸들러 (Ctrl+C)
void intHandler(int dummy) {
    printf("\n--- %s ping statistics ---\n", ping_target_ip);
    printf("%d packets transmitted, %d received, %d%% packet loss\n", 
           packets_sent, packets_received, 
           ((packets_sent - packets_received) / packets_sent) * 100);
    close(sockfd);
    exit(0);
}

int main(int argc, char *argv[]) {
    int sockfd;
    char *ip_str, *reverse_hostname;
    struct sockaddr_in addr_con;
    int addrlen = sizeof(addr_con);
    char net_buf[NI_MAXHOST];
    
    // 인자 파싱
    if (argc < 2) {
        printf("사용법: %s [-c count] hostname\n", argv[0]);
        return -1;
    }
    
    char *hostname = NULL;
    
    // -c 옵션 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            ping_count = atoi(argv[i + 1]);
            i++; // 다음 인자는 건너뛰기
        } else {
            hostname = argv[i];
        }
    }
    
    if (hostname == NULL) {
        printf("호스트명을 입력해주세요.\n");
        return -1;
    }

    // IP 주소로 변환
    ip_str = dns_lookup(hostname, &addr_con);
    if (ip_str == NULL) {
        printf("\nDNS 조회 실패.. 호스트명이나 IP를 확인해주세요\n");
        return -1;
    }

    ping_target_hostname = hostname;
    ping_target_ip = ip_str;
    ping_addr = addr_con;
    
    reverse_hostname = reverse_dns_lookup(ip_str);
    
    printf("PING %s (%s) %d(%d) bytes of data.\n", 
           reverse_hostname != NULL ? reverse_hostname : hostname, 
           ip_str, PACKET_SIZE, PACKET_SIZE + 28);

    // 원시 소켓 생성 (root 권한 필요)
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        printf("소켓 생성 실패. root 권한으로 실행해주세요.\n");
        return -1;
    }

    // 시그널 핸들러 등록
    signal(SIGINT, intHandler);

    // ping 전송
    send_ping(sockfd, &addr_con, reverse_hostname, ip_str, reverse_hostname);

    close(sockfd);
    return 0;
}