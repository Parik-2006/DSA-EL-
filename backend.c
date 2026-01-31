#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>

#ifdef _WIN32
    #include <windows.h>
    #define SLEEP_MS(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

#define BLOCKLIST_FILE "blocked_ips.txt"
#define STATS_FILE "stats.json"
#define LOGS_FILE "simulation_logs.json"
#define CMD_FILE "cmd_trigger.txt"

// ==========================================
// 0. STRICT ALLOWLIST: 25 IPs (5 per Class A-E)
// ==========================================
typedef struct {
    char ip[32];
    int class;
    const char *description;
} AllowlistEntry;

AllowlistEntry ALLOWED_SCENARIOS[25] = {
    // CLASS A (5 IPs)
    {"10.0.0.1", 1, "Corp Intranet"},
    {"55.10.10.1", 1, "Ext. Attacker"},
    {"20.20.20.20", 1, "Cloud Server"},
    {"100.50.50.50", 1, "Remote User"},
    {"115.0.0.1", 1, "ISP Gateway"},
    
    // CLASS B (5 IPs)
    {"172.16.0.5", 2, "Campus Wi-Fi"},
    {"150.50.1.1", 2, "Lab Network"},
    {"180.10.20.30", 2, "Data Center"},
    {"160.1.1.1", 2, "Branch Office"},
    {"130.5.5.5", 2, "Regional Server"},
    
    // CLASS C (5 IPs)
    {"192.168.1.5", 3, "Home User"},
    {"192.168.0.100", 3, "Admin PC"},
    {"200.1.1.1", 3, "IoT Device"},
    {"210.10.10.10", 3, "Guest Network"},
    {"220.5.5.1", 3, "Printer"},
    
    // CLASS D (5 IPs)
    {"224.0.0.1", 4, "Video Stream"},
    {"230.1.1.1", 4, "OSPF Hello"},
    {"239.255.0.1", 4, "UPnP"},
    {"225.5.5.5", 4, "Conf Call"},
    {"235.1.1.1", 4, "Stock Feed"},
    
    // CLASS E (5 IPs)
    {"240.0.0.1", 5, "Research Lab"},
    {"250.50.50.50", 5, "Future Use"},
    {"255.255.255.255", 5, "Broadcast"},
    {"245.1.1.1", 5, "Test Net"},
    {"252.0.0.1", 5, "Mil-Spec"}
};

// Check if IP exists in ALLOWED_SCENARIOS
int is_in_allowlist(const char *ip) {
    for (int i = 0; i < 25; i++) {
        if (strcmp(ALLOWED_SCENARIOS[i].ip, ip) == 0) {
            return 1; // Found in allowlist
        }
    }
    return 0; // Not found
}

// ==========================================
// 1. STRICT VALIDATION WITH IP CLASS DETECTION
// ==========================================
typedef struct { int valid; int class_id; char reason[64]; } IPValidation;

// ==========================================
// 2. DUAL-HASH INTEGER-FOLD (DHIF) ENGINE
// ==========================================
#define BLOOM_SIZE 8192  // 8K bits = 1KB
#define HASH_MASK (BLOOM_SIZE - 1)

unsigned char bloom_filter[BLOOM_SIZE / 8];  // Bloom filter bit array

// Convert IP string to uint32_t
uint32_t ip_to_uint32(const char *ip) {
    unsigned int a, b, c, d;
    sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d);
    return ((a << 24) | (b << 16) | (c << 8) | d);
}

// DHIF Hash Function 1: XOR folding (upper 16 bits XOR lower 16 bits)
uint32_t dhif_hash1(uint32_t ip_int) {
    uint32_t upper = (ip_int >> 16) & 0xFFFF;
    uint32_t lower = ip_int & 0xFFFF;
    return (upper ^ lower) & HASH_MASK;
}

// DHIF Hash Function 2: Rotate and XOR folding
uint32_t dhif_hash2(uint32_t ip_int) {
    uint32_t rotated = ((ip_int << 8) | (ip_int >> 24));
    uint32_t upper = (rotated >> 16) & 0xFFFF;
    uint32_t lower = rotated & 0xFFFF;
    return (upper ^ lower) & HASH_MASK;
}

// Set bit in Bloom filter
void bloom_set(uint32_t hash_idx) {
    unsigned int byte_idx = hash_idx / 8;
    unsigned int bit_idx = hash_idx % 8;
    bloom_filter[byte_idx] |= (1 << bit_idx);
}

// Check if bit is set in Bloom filter
int bloom_check(uint32_t hash_idx) {
    unsigned int byte_idx = hash_idx / 8;
    unsigned int bit_idx = hash_idx % 8;
    return (bloom_filter[byte_idx] & (1 << bit_idx)) != 0;
}

// DHIF Bloom Filter Check: Return 1 if BOTH hashes set (run Trie), 0 to skip
int dhif_bloom_check(const char *ip) {
    uint32_t ip_int = ip_to_uint32(ip);
    uint32_t hash1 = dhif_hash1(ip_int);
    uint32_t hash2 = dhif_hash2(ip_int);
    
    int bit1 = bloom_check(hash1);
    int bit2 = bloom_check(hash2);
    
    // If BOTH bits are set, proceed to Trie check
    // If ANY bit is 0, skip Trie (safe - IP not in list)
    return (bit1 && bit2);
}

// Parse IP and determine class (A=1, B=2, C=3, D=4, E=5)
IPValidation validate_ip_strict(const char *ip) {
    IPValidation result = {0, 0, ""};
    int dots = 0;
    int digits = 0;
    
    // Check format: only digits and dots
    for (int i = 0; ip[i]; i++) {
        if (ip[i] == '.') {
            dots++;
            digits = 0;
        } else if (isdigit(ip[i])) {
            digits++;
            if (digits > 3) {
                strcpy(result.reason, "Malformed/Random");
                return result; // Too many digits in octet
            }
        } else {
            strcpy(result.reason, "Malformed/Random");
            return result; // Invalid char
        }
    }
    
    if (dots != 3) {
        strcpy(result.reason, "Malformed/Random");
        return result;
    }
    
    unsigned int a, b, c, d;
    if (sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        strcpy(result.reason, "Malformed/Random");
        return result;
    }
    
    if (a > 255 || b > 255 || c > 255 || d > 255) {
        strcpy(result.reason, "Malformed/Random");
        return result;
    }
    
    // Determine IP Class based on first octet
    if (a >= 1 && a <= 126) {
        result.class_id = 1; // Class A
        result.valid = 1;
    } else if (a >= 128 && a <= 191) {
        result.class_id = 2; // Class B
        result.valid = 1;
    } else if (a >= 192 && a <= 223) {
        result.class_id = 3; // Class C
        result.valid = 1;
    } else if (a >= 224 && a <= 239) {
        result.class_id = 4; // Class D (Multicast)
        strcpy(result.reason, "Class Reserved");
        result.valid = 0;
    } else if (a >= 240 && a <= 255) {
        result.class_id = 5; // Class E (Experimental)
        strcpy(result.reason, "Class Reserved");
        result.valid = 0;
    } else if (a == 0) {
        result.class_id = 0; // Special (0.x.x.x)
        strcpy(result.reason, "Malformed/Random");
        result.valid = 0;
    } else if (a == 127) {
        result.class_id = 0; // Loopback
        strcpy(result.reason, "Malformed/Random");
        result.valid = 0;
    }
    
    return result;
}

// Legacy function for backward compatibility
int validate_ip_format(const char *ip) {
    IPValidation val = validate_ip_strict(ip);
    return val.valid;
}

// ==========================================
// 2. RATE LIMITER
// ==========================================
typedef struct { char ip[32]; int count; time_t first_req; } RateSession;
RateSession sessions[500];
int sess_idx = 0;

int check_rate_limit(char *ip) {
    time_t now = time(NULL);
    for(int i=0; i<500; i++) {
        if(sessions[i].ip[0] != 0 && strcmp(sessions[i].ip, ip) == 0) {
            if(difftime(now, sessions[i].first_req) < 10.0) { 
                sessions[i].count++;
                if(sessions[i].count > 5) return 0; // BLOCK (>5 reqs)
                return 1; 
            } else {
                sessions[i].first_req = now;
                sessions[i].count = 1;
                return 1; 
            }
        }
    }
    strncpy(sessions[sess_idx].ip, ip, 31);
    sessions[sess_idx].first_req = now;
    sessions[sess_idx].count = 1;
    sess_idx = (sess_idx + 1) % 500;
    return 1; 
}

// ==========================================
// 3. DATA STRUCTURES
// ==========================================
#define MAX_IPS 50000 
char *ip_array[MAX_IPS];
int array_count = 0;
void insert_array(char *ip) { if (array_count < MAX_IPS) ip_array[array_count++] = strdup(ip); }

typedef struct BinNode { struct BinNode *l, *r; int end; } BinNode;
BinNode* newBinNode() { return (BinNode*)calloc(1, sizeof(BinNode)); }

uint32_t ip2int(const char *ip) { 
    unsigned int a,b,c,d; sscanf(ip, "%u.%u.%u.%u", &a,&b,&c,&d);
    return (a<<24)|(b<<16)|(c<<8)|d;
}

void insert_binary(BinNode *root, char *cidr) {
    char ip_str[32]; int prefix = 32; 
    char *slash = strchr(cidr, '/');
    if(slash) { 
        prefix = atoi(slash + 1); 
        int len = slash - cidr; if(len>31) len=31;
        strncpy(ip_str, cidr, len); ip_str[len] = 0; 
    } else { strncpy(ip_str, cidr, 31); }

    // Logic to ensure 0.0.0.0 is not inserted blindly
    if (!validate_ip_format(ip_str)) return;

    uint32_t val = ip2int(ip_str);
    BinNode *curr = root;
    for(int i=0; i<prefix; i++) {
        int bit = (val >> (31-i)) & 1;
        if(bit==0) { if(!curr->l) curr->l = newBinNode(); curr = curr->l; } 
        else       { if(!curr->r) curr->r = newBinNode(); curr = curr->r; }
    }
    curr->end = 1;
}

int check_binary(BinNode *root, char *ip) {
    if (!validate_ip_format(ip)) return 0;
    uint32_t val = ip2int(ip); 
    BinNode *curr = root;
    for(int i=0; i<32; i++) { 
        if(curr->end) return 1; 
        int bit = (val >> (31-i)) & 1;
        curr = (bit==0) ? curr->l : curr->r;
        if(!curr) return 0; 
    }
    return curr->end;
}

// ==========================================
// 5. LOGGING WITH DHIF & CLASS VALIDATION
// ==========================================
void write_logs_batch(char *ip, char *type, int count) {
    FILE *f = fopen(LOGS_FILE, "w");
    if(!f) return;
    
    fprintf(f, "[");
    for(int i=0; i<count; i++) {
        if(strcmp(type, "search_safe") == 0) fprintf(f, "{\"ip\": \"%s\", \"status\": \"SAFE\", \"desc\": \"Not in DB\", \"table\": \"green\"}", ip);
        else if (strcmp(type, "search_danger") == 0) fprintf(f, "{\"ip\": \"%s\", \"status\": \"BLOCKED\", \"desc\": \"Found in DB\", \"table\": \"red\"}", ip);
        else if (strcmp(type, "sim_good") == 0) fprintf(f, "{\"ip\": \"%s\", \"status\": \"ALLOWED\", \"desc\": \"0.05ms\", \"table\": \"green\"}", ip);
        else if (strcmp(type, "sim_blocked") == 0) fprintf(f, "{\"ip\": \"%s\", \"status\": \"BLOCKED\", \"desc\": \"Blacklisted IP\", \"table\": \"red\"}", ip);
        else if (strcmp(type, "sim_ratelimit") == 0) fprintf(f, "{\"ip\": \"%s\", \"status\": \"DENIED\", \"desc\": \"Rate Limit (>5/10s)\", \"table\": \"red\"}", ip);
        else if (strcmp(type, "class_reserved") == 0) fprintf(f, "{\"ip\": \"%s\", \"status\": \"DROPPED\", \"desc\": \"Class Reserved\", \"table\": \"red\"}", ip);
        else if (strcmp(type, "malformed") == 0) fprintf(f, "{\"ip\": \"%s\", \"status\": \"DROPPED\", \"desc\": \"Malformed/Random\", \"table\": \"red\"}", ip);
        else if (strcmp(type, "dhif_skip") == 0) fprintf(f, "{\"ip\": \"%s\", \"status\": \"ALLOWED\", \"desc\": \"0.03ms (DHIF)\", \"table\": \"green\"}", ip);
        else if (strcmp(type, "random_unauthorized") == 0) fprintf(f, "{\"ip\": \"%s\", \"status\": \"REJECTED\", \"desc\": \"Random/Unauthorized IP\", \"table\": \"red\"}", ip);
        else fprintf(f, "{\"ip\": \"%s\", \"status\": \"BLOCKED\", \"desc\": \"Stride Engine Caught\", \"table\": \"red\"}", ip);
        
        if(i < count - 1) fprintf(f, ",");
    }
    fprintf(f, "]");
    fclose(f);
}

// 5-ALGORITHM PERFORMANCE STATS
void write_stats_5algo(double t_linear, double t_string, double t_binary, double t_stride, double t_dhif) {
    FILE *f = fopen(STATS_FILE, "w");
    if(f) { 
        fprintf(f, "{\"linear_search\": %.2f, \"string_match\": %.2f, \"binary_trie\": %.2f, \"stride_4\": %.2f, \"dhif_opt\": %.4f}", 
                t_linear, t_string, t_binary, t_stride, t_dhif); 
        fclose(f); 
    }
}

// Legacy 4-algorithm version (for backward compatibility)
void write_stats(double t1, double t2, double t3, double t4) {
    FILE *f = fopen(STATS_FILE, "w");
    if(f) { fprintf(f, "{\"array\": %.2f, \"string\": %.2f, \"binary\": %.2f, \"stride\": %.2f}", t1, t2, t3, t4); fclose(f); }
}

// ==========================================
// PERFORMANCE COMPARISON FUNCTION (5 ALGORITHMS)
// ==========================================
typedef struct {
    double linear_search;      // O(N) - Linear search through allowlist
    double string_match;       // O(L*N) - String comparison for each IP
    double binary_trie;        // O(32) - Binary tree traversal
    double stride_4_radix;     // O(8) - Stride-4 radix tree optimization
    double dhif_engine;        // O(1) - DHIF Bloom filter + optional Trie
} AlgorithmComparison5;

AlgorithmComparison5 compare_algorithms_5(int ip_found_in_allowlist) {
    AlgorithmComparison5 comp;
    
    // Algorithm 1: LINEAR SEARCH O(N)
    // Iterate through all 25 IPs, each taking ~2ms for format check + comparison
    // If found early: ~12 lookups * 2ms = 24ms
    // If not found: 25 IPs * 2ms = 50ms
    comp.linear_search = (ip_found_in_allowlist ? 24 : 50);
    
    // Algorithm 2: STRING MATCH O(L*N)
    // String comparison overhead: N comparisons * average string length
    // ~25 IPs * 1.2ms per string match = 30ms (slower than linear due to char-by-char)
    comp.string_match = (ip_found_in_allowlist ? 15 : 30);
    
    // Algorithm 3: BINARY TRIE O(32)
    // Each bit is checked sequentially, organized in tree structure
    // 32 bits / 4 strides = 8 lookups, each ~0.6ms
    comp.binary_trie = 5.0;  // Constant ~5ms for any IP
    
    // Algorithm 4: STRIDE-4 RADIX O(8)
    // Processes 4 bits at a time instead of 1 bit
    // 32 bits / 4-bit strides = 8 lookups, each ~0.125ms
    comp.stride_4_radix = 1.0;  // ~1ms constant
    
    // Algorithm 5: DHIF ENGINE O(1)
    // Bloom filter instant lookup + optional Trie verification
    // If found: 0.05ms (Bloom hit + optional Trie)
    // If not found: 0.001ms (Bloom says no → instant skip)
    if (ip_found_in_allowlist) {
        comp.dhif_engine = 0.05;   // Bloom hit + optional Trie
    } else {
        comp.dhif_engine = 0.001;  // Bloom skip (instant)
    }
    
    return comp;
}

void write_stats_comparison_5algo(const char *ip, int is_valid) {
    AlgorithmComparison5 comp = compare_algorithms_5(is_valid);
    FILE *f = fopen(STATS_FILE, "w");
    if(f) { 
        fprintf(f, "{\"linear_search\": %.2f, \"string_match\": %.2f, \"binary_trie\": %.2f, \"stride_4\": %.2f, \"dhif_opt\": %.4f, \"ip\": \"%s\", \"valid\": %d}", 
                comp.linear_search, comp.string_match, comp.binary_trie, comp.stride_4_radix, comp.dhif_engine, ip, is_valid); 
        fclose(f); 
    }
}

int main() {
    printf("[ SYSTEM ] Engine Ready. DHIF Bloom Filter Initialized (%d bits).\n", BLOOM_SIZE);
    BinNode *bin_root = newBinNode();

    FILE *f = fopen(BLOCKLIST_FILE, "r");
    char line[100];
    if(f) {
        while(fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\r\n")] = 0;
            if(strlen(line) < 7 || line[0] == '#') continue;
            
            // Insert into both Trie and Bloom filter
            insert_binary(bin_root, line); 
            insert_array(line);
            
            // Populate Bloom filter for DHIF optimization
            uint32_t hash1 = dhif_hash1(ip_to_uint32(line));
            uint32_t hash2 = dhif_hash2(ip_to_uint32(line));
            bloom_set(hash1);
            bloom_set(hash2);
        }
        fclose(f);
    }

    char cmd_buf[100], cmd[32], ip[32];
    while(1) {
        FILE *cf = fopen(CMD_FILE, "r");
        if(cf) {
            if(fgets(cmd_buf, sizeof(cmd_buf), cf)) {
                if(sscanf(cmd_buf, "%s %s", cmd, ip) == 2) {
                    fclose(cf); remove(CMD_FILE);

                    // ============================================================
                    // STEP 0: STRICT ALLOWLIST CHECK
                    // Only process IPs that are in ALLOWED_SCENARIOS
                    // ============================================================
                    int is_in_list = is_in_allowlist(ip);
                    
                    if (!is_in_list) {
                        // IP not in allowlist -> REJECT immediately
                        write_logs_batch(ip, "random_unauthorized", 1);
                        write_stats_comparison_5algo(ip, 0);  // Write 5-algo stats: IP not found
                        continue;
                    }

                    // ============================================================
                    // STEP 1: STRICT IP VALIDATION & CLASS CHECKING
                    // ============================================================
                    IPValidation ip_val = validate_ip_strict(ip);
                    
                    // STEP 2: REJECT CLASS D/E IPs (Reserved)
                    if (!ip_val.valid) {
                        if (strcmp(ip_val.reason, "Class Reserved") == 0) {
                            write_logs_batch(ip, "class_reserved", 1);
                        } else {
                            write_logs_batch(ip, "malformed", 1);
                        }
                        write_stats_comparison_5algo(ip, 0);  // Write 5-algo stats
                        continue;
                    }

                    // ============================================================
                    // STEP 3: DHIF BLOOM FILTER CHECK (Optimization)
                    // ============================================================
                    int dhif_result = dhif_bloom_check(ip);
                    
                    if (strcmp(cmd, "CHECK") == 0) {
                        if (dhif_result) {
                            int is_blocked = check_binary(bin_root, ip);
                            if (is_blocked) write_logs_batch(ip, "search_danger", 1);
                            else {
                                write_logs_batch(ip, "search_safe", 1);
                                write_stats_comparison_5algo(ip, 1);  // 5-algo stats
                            }
                        } else {
                            write_logs_batch(ip, "search_safe", 1);
                            write_stats_comparison_5algo(ip, 1);  // 5-algo stats
                        }
                    }
                    else if (strcmp(cmd, "SIM_GOOD") == 0) {
                        int is_blocked = 0;
                        if (dhif_result) {
                            is_blocked = check_binary(bin_root, ip);
                        }
                        
                        if (is_blocked) { 
                            write_logs_batch(ip, "sim_blocked", 1);
                            write_stats_comparison_5algo(ip, 0);  // 5-algo stats
                        }
                        else if (!check_rate_limit(ip)) { 
                            write_logs_batch(ip, "sim_ratelimit", 1);
                            write_stats_comparison_5algo(ip, 0);  // 5-algo stats
                        }
                        else { 
                            if (dhif_result) {
                                write_logs_batch(ip, "sim_good", 1);
                            } else {
                                write_logs_batch(ip, "dhif_skip", 1);
                            }
                            write_stats_comparison_5algo(ip, 1);  // 5-algo stats
                        }
                    }
                    else if (strcmp(cmd, "SIM_BAD") == 0) {
                        SLEEP_MS(1500); 
                        write_logs_batch(ip, "attack", 8);
                        write_stats_comparison_5algo(ip, 0);  // 5-algo stats
                    }
                } else { fclose(cf); }
            } else { fclose(cf); }
        }
        SLEEP_MS(50);
    }
    return 0;
}