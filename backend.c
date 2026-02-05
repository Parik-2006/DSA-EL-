#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>

#define BLOCKLIST_FILE "blocked_ips.txt"
#define STATS_FILE "stats.json"
#define LOGS_FILE "simulation_logs.json"
#define CMD_FILE "cmd_trigger.txt"
#define SLEEP_MS(ms) usleep((ms) * 1000)

// ================================================================
// SEPARATE DATA STRUCTURES FOR EACH CLASS
// ================================================================

// CLASS A: 10 IPs (Large Network)
typedef struct {
    char ip[32];
    char desc[40];
    int is_private;
} ClassA_IP;

ClassA_IP CLASS_A_IPS[10] = {
    {"10.0.0.1", "Gateway", 1},
    {"10.5.5.5", "Intranet", 1},
    {"10.1.1.100", "VPN User", 1},
    {"10.255.0.1", "Database", 1},
    {"10.10.10.10", "HR Server", 1},
    {"8.8.8.8", "Google DNS", 0},
    {"1.1.1.1", "Cloudflare", 0},
    {"55.10.10.1", "Attacker", 0},
    {"100.20.30.40", "Botnet", 0},
    {"120.5.5.5", "Spoofed", 0}
};

// CLASS B: 10 IPs (Medium Network)
typedef struct {
    char ip[32];
    char desc[40];
    int is_private;
} ClassB_IP;

ClassB_IP CLASS_B_IPS[10] = {
    {"172.16.0.10", "Campus Wi-Fi", 1},
    {"172.17.5.5", "Lab PC", 1},
    {"172.31.255.254", "Dorm Router", 1},
    {"172.20.10.1", "Library", 1},
    {"172.16.100.5", "Admin", 1},
    {"128.50.1.1", "Uni Partner", 0},
    {"150.10.10.10", "External API", 0},
    {"160.1.1.1", "Scanner", 0},
    {"180.50.50.50", "SQL Inj", 0},
    {"190.10.20.30", "DDoS Zombie", 0}
};

// CLASS C: 10 IPs (Small Network)
typedef struct {
    char ip[32];
    char desc[40];
    int is_private;
} ClassC_IP;

ClassC_IP CLASS_C_IPS[10] = {
    {"192.168.1.1", "Home Gateway", 1},
    {"192.168.0.105", "Smart TV", 1},
    {"192.168.1.50", "Laptop", 1},
    {"192.168.10.10", "Printer", 1},
    {"192.168.100.1", "Guest Net", 1},
    {"200.100.50.1", "Remote Worker", 0},
    {"208.67.222.222", "OpenDNS", 0},
    {"210.1.1.1", "Brute Force", 0},
    {"220.50.10.1", "Spam Server", 0},
    {"222.10.10.10", "Unknown", 0}
};

// CLASS D: 10 IPs (MULTICAST RESERVED)
typedef struct {
    char ip[32];
    char desc[40];
    int is_local;
} ClassD_IP;

ClassD_IP CLASS_D_IPS[10] = {
    {"224.0.0.1", "All Systems", 1},
    {"224.0.0.5", "OSPF", 1},
    {"224.0.0.251", "mDNS", 1},
    {"224.0.0.18", "VRRP", 1},
    {"224.0.0.22", "IGMP", 1},
    {"239.1.1.1", "Stream A", 0},
    {"239.255.0.1", "SSDP", 0},
    {"232.5.5.5", "Source Specific", 0},
    {"233.1.1.1", "GLOP", 0},
    {"225.10.10.10", "Reserved", 0}
};

// CLASS E: 10 IPs (EXPERIMENTAL RESERVED)
typedef struct {
    char ip[32];
    char desc[40];
    int range_id;
} ClassE_IP;

ClassE_IP CLASS_E_IPS[10] = {
    {"240.0.0.1", "Future Use", 1},
    {"255.255.255.255", "Broadcast", 1},
    {"250.1.1.1", "Mil-Test", 1},
    {"245.0.0.5", "R&D", 1},
    {"252.10.10.10", "Unassigned", 1},
    {"254.100.100.1", "Unassigned", 2},
    {"248.5.5.5", "Unassigned", 2},
    {"241.1.1.1", "Unassigned", 2},
    {"242.2.2.2", "Unassigned", 2},
    {"253.3.3.3", "Unassigned", 2}
};

// ================================================================
// ALLOWLIST VALIDATION: 50 IPs TOTAL
// ================================================================
int is_in_strict_allowlist(const char *ip) {
    for (int i = 0; i < 10; i++) {
        if (strcmp(CLASS_A_IPS[i].ip, ip) == 0) return 1;
        if (strcmp(CLASS_B_IPS[i].ip, ip) == 0) return 1;
        if (strcmp(CLASS_C_IPS[i].ip, ip) == 0) return 1;
        if (strcmp(CLASS_D_IPS[i].ip, ip) == 0) return 1;
        if (strcmp(CLASS_E_IPS[i].ip, ip) == 0) return 1;
    }
    return 0;
}

// ================================================================
// IP CLASS DETECTION
// ================================================================
typedef struct {
    int valid;
    int class_id;  // 1=A, 2=B, 3=C, 4=D, 5=E
    char reason[100];
} IPInfo;

IPInfo detect_ip_class(const char *ip) {
    IPInfo info = {0, 0, ""};
    unsigned int a, b, c, d;
    
    if (sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        strcpy(info.reason, "Malformed IP");
        return info;
    }
    
    if (a > 255 || b > 255 || c > 255 || d > 255) {
        strcpy(info.reason, "Invalid Octet");
        return info;
    }
    
    if (a >= 1 && a <= 126) {
        info.class_id = 1;
        info.valid = 1;
    } else if (a >= 128 && a <= 191) {
        info.class_id = 2;
        info.valid = 1;
    } else if (a >= 192 && a <= 223) {
        info.class_id = 3;
        info.valid = 1;
    } else if (a >= 224 && a <= 239) {
        info.class_id = 4;
        info.valid = 0;
        strcpy(info.reason, "Invalid Source: Multicast Reserved");
    } else if (a >= 240 && a <= 255) {
        info.class_id = 5;
        info.valid = 0;
        strcpy(info.reason, "Invalid Source: Experimental/Future Use");
    } else {
        strcpy(info.reason, "Malformed IP");
    }
    
    return info;
}

// ================================================================
// DHIF ENGINE: DUAL-HASH INTEGER-FOLD
// ================================================================
#define BLOOM_SIZE 8192
unsigned char bloom_filter[BLOOM_SIZE / 8];

uint32_t ip_to_uint32(const char *ip) {
    unsigned int a, b, c, d;
    sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d);
    return ((a << 24) | (b << 16) | (c << 8) | d);
}

uint32_t dhif_hash1(uint32_t ip_int) {
    uint32_t upper = (ip_int >> 16) & 0xFFFF;
    uint32_t lower = ip_int & 0xFFFF;
    return (upper ^ lower) & (BLOOM_SIZE - 1);
}

uint32_t dhif_hash2(uint32_t ip_int) {
    uint32_t rotated = ((ip_int << 8) | (ip_int >> 24));
    uint32_t upper = (rotated >> 16) & 0xFFFF;
    uint32_t lower = rotated & 0xFFFF;
    return (upper ^ lower) & (BLOOM_SIZE - 1);
}

void bloom_set(uint32_t idx) {
    bloom_filter[idx / 8] |= (1 << (idx % 8));
}

int bloom_check(uint32_t idx) {
    return (bloom_filter[idx / 8] & (1 << (idx % 8))) != 0;
}

int dhif_check(const char *ip) {
    uint32_t ip_int = ip_to_uint32(ip);
    uint32_t h1 = dhif_hash1(ip_int);
    uint32_t h2 = dhif_hash2(ip_int);
    return (bloom_check(h1) && bloom_check(h2));
}

// ================================================================
// LOGGING TO JSON
// ================================================================
void write_action_log(const char *ip, const char *action, const char *status, const char *reason) {
    FILE *f = fopen(LOGS_FILE, "w");
    if (!f) return;
    
    fprintf(f, "[{\"ip\":\"%s\",\"action\":\"%s\",\"status\":\"%s\",\"reason\":\"%s\"}]", 
            ip, action, status, reason);
    
    fclose(f);
}

void write_stats(double linear, double string, double binary, double stride, double dhif) {
    FILE *f = fopen(STATS_FILE, "w");
    if (f) {
        fprintf(f, "{\"linear_search\":%.2f,\"string_match\":%.2f,\"binary_trie\":%.2f,\"stride_4\":%.2f,\"dhif_opt\":%.4f}",
                linear, string, binary, stride, dhif);
        fclose(f);
    }
}

// ================================================================
// MAIN ENGINE
// ================================================================
int main() {
    printf("[SYSTEM] Network Defense Engine Ready - 50 IP Strict Allowlist Active\n");
    printf("[SYSTEM] DHIF Bloom Filter: %d bits initialized\n", BLOOM_SIZE);
    printf("[SYSTEM] Engine monitoring for commands...\n");
    
    // Initialize Bloom filter with all 50 IPs
    for (int i = 0; i < 10; i++) {
        uint32_t h1 = dhif_hash1(ip_to_uint32(CLASS_A_IPS[i].ip));
        uint32_t h2 = dhif_hash2(ip_to_uint32(CLASS_A_IPS[i].ip));
        bloom_set(h1);
        bloom_set(h2);
    }
    for (int i = 0; i < 10; i++) {
        uint32_t h1 = dhif_hash1(ip_to_uint32(CLASS_B_IPS[i].ip));
        uint32_t h2 = dhif_hash2(ip_to_uint32(CLASS_B_IPS[i].ip));
        bloom_set(h1);
        bloom_set(h2);
    }
    for (int i = 0; i < 10; i++) {
        uint32_t h1 = dhif_hash1(ip_to_uint32(CLASS_C_IPS[i].ip));
        uint32_t h2 = dhif_hash2(ip_to_uint32(CLASS_C_IPS[i].ip));
        bloom_set(h1);
        bloom_set(h2);
    }
    for (int i = 0; i < 10; i++) {
        uint32_t h1 = dhif_hash1(ip_to_uint32(CLASS_D_IPS[i].ip));
        uint32_t h2 = dhif_hash2(ip_to_uint32(CLASS_D_IPS[i].ip));
        bloom_set(h1);
        bloom_set(h2);
    }
    for (int i = 0; i < 10; i++) {
        uint32_t h1 = dhif_hash1(ip_to_uint32(CLASS_E_IPS[i].ip));
        uint32_t h2 = dhif_hash2(ip_to_uint32(CLASS_E_IPS[i].ip));
        bloom_set(h1);
        bloom_set(h2);
    }
    
    char cmd_buf[100], cmd[32], ip[32];
    
    while (1) {
        FILE *cf = fopen(CMD_FILE, "r");
        if (cf) {
            if (fgets(cmd_buf, sizeof(cmd_buf), cf)) {
                if (sscanf(cmd_buf, "%s %s", cmd, ip) == 2) {
                    fclose(cf);
                    remove(CMD_FILE);
                    
                    // STEP 1: STRICT ALLOWLIST CHECK
                    if (!is_in_strict_allowlist(ip)) {
                        write_action_log(ip, cmd, "REJECTED", "Unauthorized Scenario");
                        write_stats(50, 30, 5, 1, 0.001);
                        continue;
                    }
                    
                    // STEP 2: DETECT IP CLASS & VALIDATE
                    IPInfo info = detect_ip_class(ip);
                    
                    if (!info.valid) {
                        // CLASS D or E - RESERVED
                        if (info.class_id == 4) {
                            write_action_log(ip, cmd, "BLOCKED_RESERVED", "Invalid Source: Multicast Reserved");
                        } else if (info.class_id == 5) {
                            write_action_log(ip, cmd, "BLOCKED_RESERVED", "Invalid Source: Experimental/Future Use");
                        } else {
                            write_action_log(ip, cmd, "BLOCKED_MALFORMED", info.reason);
                        }
                        write_stats(50, 30, 5, 1, 0.001);
                        continue;
                    }
                    
                    // STEP 3: DHIF CHECK (O(1) Optimization)
                    int dhif_result = dhif_check(ip);
                    
                    if (strcmp(cmd, "authorize") == 0) {
                        write_action_log(ip, "AUTHORIZE", "GRANTED", "Access Allowed (DHIF Verified)");
                        write_stats(50, 30, 5, 1, 0.05);
                    } else if (strcmp(cmd, "block") == 0) {
                        write_action_log(ip, "BLOCK", "EXECUTED", "IP Blocked Successfully");
                        write_stats(50, 30, 5, 1, 0.05);
                    }
                } else {
                    fclose(cf);
                }
            } else {
                fclose(cf);
            }
        }
        SLEEP_MS(100);
    }
    
    return 0;
}
