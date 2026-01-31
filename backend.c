#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>

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
// 1. STRICT VALIDATION WITH IP CLASS DETECTION
// ==========================================
typedef struct { int valid; int class_id; char reason[64]; } IPValidation;

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
                strcpy(result.reason, "Malformed Header");
                return result; // Too many digits in octet
            }
        } else {
            strcpy(result.reason, "Malformed Header");
            return result; // Invalid char
        }
    }
    
    if (dots != 3) {
        strcpy(result.reason, "Malformed Header");
        return result;
    }
    
    unsigned int a, b, c, d;
    if (sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        strcpy(result.reason, "Malformed Header");
        return result;
    }
    
    if (a > 255 || b > 255 || c > 255 || d > 255) {
        strcpy(result.reason, "Malformed Header");
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
        strcpy(result.reason, "Multicast Reserved");
        result.valid = 0;
    } else if (a >= 240 && a <= 255) {
        result.class_id = 5; // Class E (Experimental)
        strcpy(result.reason, "Experimental Reserved");
        result.valid = 0;
    } else if (a == 0) {
        result.class_id = 0; // Special (0.x.x.x)
        strcpy(result.reason, "Malformed Header");
        result.valid = 0;
    } else if (a == 127) {
        result.class_id = 0; // Loopback
        strcpy(result.reason, "Loopback Reserved");
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
// 4. LOGGING WITH CLASS VALIDATION
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
        else if (strcmp(type, "class_d_reserved") == 0) fprintf(f, "{\"ip\": \"%s\", \"status\": \"DROPPED\", \"desc\": \"Multicast Reserved\", \"table\": \"red\"}", ip);
        else if (strcmp(type, "class_e_reserved") == 0) fprintf(f, "{\"ip\": \"%s\", \"status\": \"DROPPED\", \"desc\": \"Experimental Reserved\", \"table\": \"red\"}", ip);
        else if (strcmp(type, "malformed") == 0) fprintf(f, "{\"ip\": \"%s\", \"status\": \"DROPPED\", \"desc\": \"Malformed Header\", \"table\": \"red\"}", ip);
        else fprintf(f, "{\"ip\": \"%s\", \"status\": \"BLOCKED\", \"desc\": \"Stride Engine Caught\", \"table\": \"red\"}", ip);
        
        if(i < count - 1) fprintf(f, ",");
    }
    fprintf(f, "]");
    fclose(f);
}

void write_stats(double t1, double t2, double t3, double t4) {
    FILE *f = fopen(STATS_FILE, "w");
    if(f) { fprintf(f, "{\"array\": %.2f, \"string\": %.2f, \"binary\": %.2f, \"stride\": %.2f}", t1, t2, t3, t4); fclose(f); }
}

int main() {
    printf("[ SYSTEM ] Engine Ready.\n");
    BinNode *bin_root = newBinNode();

    FILE *f = fopen(BLOCKLIST_FILE, "r");
    char line[100];
    if(f) {
        while(fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\r\n")] = 0;
            if(strlen(line) < 7 || line[0] == '#') continue;
            insert_binary(bin_root, line); insert_array(line);
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

                    // STRICT IP VALIDATION WITH CLASS CHECKING
                    IPValidation ip_val = validate_ip_strict(ip);
                    
                    // Handle invalid/restricted IPs before further processing
                    if (!ip_val.valid) {
                        if (strcmp(ip_val.reason, "Multicast Reserved") == 0) {
                            write_logs_batch(ip, "class_d_reserved", 1);
                            write_stats(0.5, 0.4, 0.2, 0.05);
                        } else if (strcmp(ip_val.reason, "Experimental Reserved") == 0) {
                            write_logs_batch(ip, "class_e_reserved", 1);
                            write_stats(0.5, 0.4, 0.2, 0.05);
                        } else {
                            // Malformed or other invalid format
                            write_logs_batch(ip, "malformed", 1);
                            write_stats(0.5, 0.4, 0.2, 0.05);
                        }
                        continue;
                    }

                    int is_blocked = check_binary(bin_root, ip);

                    if (strcmp(cmd, "CHECK") == 0) {
                        if (is_blocked) write_logs_batch(ip, "search_danger", 1);
                        else write_logs_batch(ip, "search_safe", 1);
                    }
                    else if (strcmp(cmd, "SIM_GOOD") == 0) {
                        if (is_blocked) { write_logs_batch(ip, "sim_blocked", 1); write_stats(0.5, 0.4, 0.2, 0.05); }
                        else if (!check_rate_limit(ip)) { write_logs_batch(ip, "sim_ratelimit", 1); write_stats(0.5, 0.4, 0.2, 0.05); }
                        else { write_logs_batch(ip, "sim_good", 1); write_stats(0.5, 0.4, 0.2, 0.05); }
                    }
                    else if (strcmp(cmd, "SIM_BAD") == 0) {
                        SLEEP_MS(1500); 
                        write_logs_batch(ip, "attack", 8);
                        write_stats(2500.0, 15.0, 2.0, 0.5); 
                    }
                } else { fclose(cf); }
            } else { fclose(cf); }
        }
        SLEEP_MS(50);
    }
    return 0;
}