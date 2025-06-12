#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>

// MAC 주소를 문자열로 변환
void format_mac_address(unsigned char *mac, char *buffer) {
    sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// 바이트 단위를 읽기 쉬운 형태로 변환
void format_bytes(unsigned long bytes, char *buffer) {
    if (bytes >= 1024 * 1024 * 1024) {
        sprintf(buffer, "%.1f GB", (double)bytes / (1024 * 1024 * 1024));
    } else if (bytes >= 1024 * 1024) {
        sprintf(buffer, "%.1f MB", (double)bytes / (1024 * 1024));
    } else if (bytes >= 1024) {
        sprintf(buffer, "%.1f KB", (double)bytes / 1024);
    } else {
        sprintf(buffer, "%lu B", bytes);
    }
}

// 인터페이스 플래그를 문자열로 변환
void format_flags(unsigned int flags, char *buffer) {
    buffer[0] = '\0';
    
    if (flags & IFF_UP) strcat(buffer, "UP ");
    if (flags & IFF_BROADCAST) strcat(buffer, "BROADCAST ");
    if (flags & IFF_DEBUG) strcat(buffer, "DEBUG ");
    if (flags & IFF_LOOPBACK) strcat(buffer, "LOOPBACK ");
    if (flags & IFF_POINTOPOINT) strcat(buffer, "POINTOPOINT ");
    if (flags & IFF_RUNNING) strcat(buffer, "RUNNING ");
    if (flags & IFF_NOARP) strcat(buffer, "NOARP ");
    if (flags & IFF_PROMISC) strcat(buffer, "PROMISC ");
    if (flags & IFF_NOTRAILERS) strcat(buffer, "NOTRAILERS ");
    if (flags & IFF_ALLMULTI) strcat(buffer, "ALLMULTI ");
    if (flags & IFF_MASTER) strcat(buffer, "MASTER ");
    if (flags & IFF_SLAVE) strcat(buffer, "SLAVE ");
    if (flags & IFF_MULTICAST) strcat(buffer, "MULTICAST ");
    
    // 마지막 공백 제거
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == ' ') {
        buffer[len - 1] = '\0';
    }
}

// 네트워크 통계 정보 읽기
int get_interface_stats(const char *interface, unsigned long *rx_bytes, 
                       unsigned long *tx_bytes, unsigned long *rx_packets, 
                       unsigned long *tx_packets) {
    FILE *fp;
    char path[256];
    char buffer[256];
    
    // RX bytes
    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/rx_bytes", interface);
    fp = fopen(path, "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            *rx_bytes = strtoul(buffer, NULL, 10);
        }
        fclose(fp);
    }
    
    // TX bytes
    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/tx_bytes", interface);
    fp = fopen(path, "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            *tx_bytes = strtoul(buffer, NULL, 10);
        }
        fclose(fp);
    }
    
    // RX packets
    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/rx_packets", interface);
    fp = fopen(path, "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            *rx_packets = strtoul(buffer, NULL, 10);
        }
        fclose(fp);
    }
    
    // TX packets
    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/tx_packets", interface);
    fp = fopen(path, "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            *tx_packets = strtoul(buffer, NULL, 10);
        }
        fclose(fp);
    }
    
    return 0;
}

// MTU 크기 가져오기
int get_mtu(const char *interface) {
    int sockfd;
    struct ifreq ifr;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return -1;
    
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    
    if (ioctl(sockfd, SIOCGIFMTU, &ifr) < 0) {
        close(sockfd);
        return -1;
    }
    
    close(sockfd);
    return ifr.ifr_mtu;
}

// 특정 인터페이스 정보 출력
void show_interface(const char *interface_name) {
    struct ifaddrs *ifaddr, *ifa;
    char mac_str[18];
    char flags_str[256];
    char rx_bytes_str[32], tx_bytes_str[32];
    unsigned long rx_bytes = 0, tx_bytes = 0, rx_packets = 0, tx_packets = 0;
    int mtu;
    int found = 0;
    
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        // 특정 인터페이스만 출력하거나 모든 인터페이스 출력
        if (interface_name && strcmp(ifa->ifa_name, interface_name) != 0) {
            continue;
        }
        
        // 중복 출력 방지
        if (found && interface_name && strcmp(ifa->ifa_name, interface_name) == 0) {
            if (ifa->ifa_addr->sa_family != AF_INET && ifa->ifa_addr->sa_family != AF_INET6) {
                continue;
            }
        }
        
        if (!found || !interface_name || strcmp(ifa->ifa_name, interface_name) == 0) {
            // 인터페이스 헤더 출력 (처음 한 번만)
            static char last_interface[IFNAMSIZ] = "";
            if (strcmp(last_interface, ifa->ifa_name) != 0) {
                strcpy(last_interface, ifa->ifa_name);
                
                printf("%s: ", ifa->ifa_name);
                
                // 플래그 정보
                format_flags(ifa->ifa_flags, flags_str);
                printf("flags=%d<%s> ", ifa->ifa_flags, flags_str);
                
                // MTU 정보
                mtu = get_mtu(ifa->ifa_name);
                if (mtu > 0) {
                    printf("mtu %d\n", mtu);
                } else {
                    printf("\n");
                }
                
                // 통계 정보 가져오기
                get_interface_stats(ifa->ifa_name, &rx_bytes, &tx_bytes, &rx_packets, &tx_packets);
            }
        }
        
        // IP 주소 정보
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
            struct sockaddr_in *netmask_in = (struct sockaddr_in *)ifa->ifa_netmask;
            struct sockaddr_in *broadcast_in = (struct sockaddr_in *)ifa->ifa_broadaddr;
            
            printf("        inet %s", inet_ntoa(addr_in->sin_addr));
            
            if (netmask_in) {
                printf("  netmask %s", inet_ntoa(netmask_in->sin_addr));
            }
            
            if (broadcast_in && (ifa->ifa_flags & IFF_BROADCAST)) {
                printf("  broadcast %s", inet_ntoa(broadcast_in->sin_addr));
            }
            printf("\n");
        }
        
        // IPv6 주소 정보
        else if (ifa->ifa_addr->sa_family == AF_INET6) {
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)ifa->ifa_addr;
            char addr_str[INET6_ADDRSTRLEN];
            
            if (inet_ntop(AF_INET6, &addr_in6->sin6_addr, addr_str, INET6_ADDRSTRLEN)) {
                printf("        inet6 %s", addr_str);
                
                // Scope ID 출력
                if (addr_in6->sin6_scope_id > 0) {
                    printf("%%%d", addr_in6->sin6_scope_id);
                }
                
                // 프리픽스 길이 (간단히 /64로 가정)
                printf("/64");
                
                // 주소 타입
                if (IN6_IS_ADDR_LINKLOCAL(&addr_in6->sin6_addr)) {
                    printf("  scopeid 0x%x<link>", addr_in6->sin6_scope_id);
                } else if (IN6_IS_ADDR_LOOPBACK(&addr_in6->sin6_addr)) {
                    printf("  scopeid 0x%x<host>", addr_in6->sin6_scope_id);
                }
                printf("\n");
            }
        }
        
        // MAC 주소 정보 (패킷 소켓)
        else if (ifa->ifa_addr->sa_family == AF_PACKET) {
            struct sockaddr_ll *s = (struct sockaddr_ll *)ifa->ifa_addr;
            if (s->sll_halen == 6) {  // Ethernet MAC
                format_mac_address(s->sll_addr, mac_str);
                printf("        ether %s  txqueuelen 1000  (Ethernet)\n", mac_str);
                
                // 통계 정보 출력
                format_bytes(rx_bytes, rx_bytes_str);
                format_bytes(tx_bytes, tx_bytes_str);
                
                printf("        RX packets %lu  bytes %lu (%s)\n", rx_packets, rx_bytes, rx_bytes_str);
                printf("        RX errors 0  dropped 0  overruns 0  frame 0\n");
                printf("        TX packets %lu  bytes %lu (%s)\n", tx_packets, tx_bytes, tx_bytes_str);
                printf("        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0\n");
                printf("\n");
            }
        }
        
        found = 1;
        if (interface_name) break;
    }
    
    if (interface_name && !found) {
        printf("%s: error fetching interface information: Device not found\n", interface_name);
    }
    
    freeifaddrs(ifaddr);
}

// 사용법 출력
void print_usage(const char *program_name) {
    printf("Usage: %s [interface]\n", program_name);
    printf("Show configuration of network interface(s)\n");
    printf("\nOptions:\n");
    printf("  interface    Show only specified interface\n");
    printf("  (no args)    Show all interfaces\n");
}

int main(int argc, char *argv[]) {
    // 도움말 옵션
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return 0;
    }
    
    // 특정 인터페이스 지정
    if (argc == 2) {
        show_interface(argv[1]);
    } 
    // 모든 인터페이스 출력
    else if (argc == 1) {
        show_interface(NULL);
    } 
    // 잘못된 인자
    else {
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}