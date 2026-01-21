#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <stdint.h> // For uint32_t types

// --- CROSS-PLATFORM COMPATIBILITY ---
#ifdef _WIN32
    #include <windows.h>
    #define SLEEP_MS(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

#define LOG_LEGACY "logs.json"
#define LOG_DEFENSE "logs_defense.json"
#define TRIE_VIEW "trie_view.json"
#define BLOCKLIST_FILE "blocked_ips.txt"

// --- RATE LIMIT CONFIGURATION ---
#define RATE_LIMIT_MAX 3      
#define RATE_LIMIT_WINDOW 10  

typedef struct { char ip[16]; int count; time_t start_time; } RateLimitEntry;
RateLimitEntry sessions[100];
int session_count = 0;

// Returns 1 if Allowed, 0 if Rate Limited
int check_rate_limit(char *ip) {
    time_t now = time(NULL);
    for (int i = 0; i < session_count; i++) {
        if (strcmp(sessions[i].ip, ip) == 0) {
            if (difftime(now, sessions[i].start_time) <= RATE_LIMIT_WINDOW) {
                sessions[i].count++;
                return (sessions[i].count <= RATE_LIMIT_MAX);
            } else {
                sessions[i].start_time = now;
                sessions[i].count = 1;
                return 1;
            }
        }
    }
    if (session_count < 100) {
        strcpy(sessions[session_count].ip, ip);
        sessions[session_count].start_time = now;
        sessions[session_count].count = 1;
        session_count++;
    }
    return 1;
}

// IP ANALYSIS HELPER
int analyze_ip(char *ip, char *result) {
    if (!ip || strlen(ip) == 0) { strcpy(result, "EMPTY"); return 0; }
    int a, b, c, d;
    if (sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d) != 4) { strcpy(result, "FORMAT ERROR"); return 0; }
    if (a<0||a>255||b<0||b>255||c<0||c>255||d<0||d>255) { strcpy(result, "RANGE ERROR"); return 0; }
    sprintf(result, "Valid IP");
    return 1;
}

// ==========================================
//    ENGINE 1: LEGACY STRING TRIE (Exact Match)
// ==========================================
typedef struct TrieNode { struct TrieNode *children[11]; int is_blocked; } TrieNode;

TrieNode* createNode() {
    TrieNode *node = (TrieNode*)malloc(sizeof(TrieNode));
    node->is_blocked = 0;
    for(int i=0; i<11; i++) node->children[i] = NULL;
    return node;
}
int getIndex(char c) { return (c == '.') ? 10 : c - '0'; }

void insert_ip_string(TrieNode *root, char *ip) {
    TrieNode *curr = root;
    for (int i = 0; ip[i]; i++) {
        if(ip[i] < '0' && ip[i] != '.') continue;
        int idx = getIndex(ip[i]);
        if (!curr->children[idx]) curr->children[idx] = createNode();
        curr = curr->children[idx];
    }
    curr->is_blocked = 1;
}

int check_ip_string(TrieNode *root, char *ip) {
    TrieNode *curr = root;
    for (int i = 0; ip[i]; i++) {
        if(ip[i] < '0' && ip[i] != '.') continue;
        int idx = getIndex(ip[i]);
        if (!curr->children[idx]) return 0;
        curr = curr->children[idx];
    }
    return curr && curr->is_blocked;
}

// ==========================================
//    ENGINE 2: NEXT-GEN BITWISE TRIE (CIDR)
// ==========================================
typedef struct BinaryNode { struct BinaryNode *left; struct BinaryNode *right; int is_blocked; } BinaryNode;

BinaryNode* createBinaryNode() {
    BinaryNode *node = (BinaryNode*)malloc(sizeof(BinaryNode));
    node->left = node->right = NULL;
    node->is_blocked = 0;
    return node;
}

// Helper: Convert String IP to 32-bit Integer
uint32_t ip_to_int(const char *ip) {
    unsigned int a, b, c, d;
    if (sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return 0;
    return (a << 24) | (b << 16) | (c << 8) | d;
}

// Insert CIDR Rule (e.g., 10.0.0.0/8)
void insert_cidr(BinaryNode *root, char *cidr) {
    char ip_str[32];
    int prefix_len = 32; // Default to /32 (Exact IP) if no slash found
    
    // Parse "IP/Prefix" string
    char *slash = strchr(cidr, '/');
    if (slash) {
        int len = slash - cidr;
        strncpy(ip_str, cidr, len);
        ip_str[len] = '\0';
        prefix_len = atoi(slash + 1);
    } else {
        strcpy(ip_str, cidr);
    }

    uint32_t ip_val = ip_to_int(ip_str);
    BinaryNode *curr = root;

    // Traverse bits only up to prefix length
    for (int i = 0; i < prefix_len; i++) {
        // Get the bit at position (31 - i)
        int bit = (ip_val >> (31 - i)) & 1;
        
        if (bit == 0) {
            if (!curr->left) curr->left = createBinaryNode();
            curr = curr->left;
        } else {
            if (!curr->right) curr->right = createBinaryNode();
            curr = curr->right;
        }
    }
    curr->is_blocked = 1; // Mark this Subnet (or IP) as Blocked
}

// Check IP against Bitwise Trie
int check_binary_trie(BinaryNode *root, char *ip) {
    uint32_t ip_val = ip_to_int(ip);
    BinaryNode *curr = root;
    
    // Walk down the 32 bits of the IP
    for (int i = 0; i < 32; i++) {
        // OPTIMIZATION: If we hit a blocked node early, it means
        // a larger subnet block is active (e.g. 10.0.0.0/8).
        if (curr->is_blocked) return 1; 
        
        int bit = (ip_val >> (31 - i)) & 1;
        if (bit == 0) curr = curr->left;
        else curr = curr->right;
        
        if (!curr) return 0; // No rule found on this path
    }
    return curr->is_blocked; // Check exact match at the end
}


// ==========================================
//    LOGGING & FILE UTILS
// ==========================================

// Legacy Logging
typedef struct { int id; char ip[16]; char type; char info[60]; } LogLegacy;
LogLegacy history_old[1000];
int count_old = 0, global_id_old = 1;

void add_log_old(char *ip, char type, char *info) {
    history_old[count_old].id = global_id_old++;
    strcpy(history_old[count_old].ip, ip);
    history_old[count_old].type = type;
    strcpy(history_old[count_old].info, info);
    count_old++; if(count_old >= 1000) count_old = 0; 
}

void save_logs_old() {
    FILE *f = fopen(LOG_LEGACY, "w"); if (!f) return;
    fprintf(f, "[");
    for (int i = 0; i < count_old; i++) {
        fprintf(f, "{\"id\": %d, \"ip\": \"%s\", \"type\": \"%c\", \"info\": \"%s\"}", 
            history_old[i].id, history_old[i].ip, history_old[i].type, history_old[i].info);
        if (i < count_old - 1) fprintf(f, ",");
    }
    fprintf(f, "]"); fclose(f);
}

// Defense Logging
#define QUEUE_SIZE 50 
typedef struct { int id; char ip[16]; char type; char info[80]; } LogDefense;
LogDefense history_new[QUEUE_SIZE];
int front = 0, rear = -1, count_new = 0, global_id_new = 1;

void add_log_new(char *ip, char type, char *info) {
    if (count_new == QUEUE_SIZE) { front = (front + 1) % QUEUE_SIZE; count_new--; }
    rear = (rear + 1) % QUEUE_SIZE;
    history_new[rear].id = global_id_new++;
    strcpy(history_new[rear].ip, ip);
    history_new[rear].type = type;
    strcpy(history_new[rear].info, info);
    count_new++;
}

void save_logs_new() {
    FILE *f = fopen(LOG_DEFENSE, "w"); if (!f) return;
    fprintf(f, "[");
    int idx = front;
    for(int k=0; k<count_new; k++) {
        fprintf(f, "{\"id\": %d, \"ip\": \"%s\", \"type\": \"%c\", \"info\": \"%s\"}", 
            history_new[idx].id, history_new[idx].ip, history_new[idx].type, history_new[idx].info);
        if (k < count_new - 1) fprintf(f, ",");
        idx = (idx + 1) % QUEUE_SIZE;
    }
    fprintf(f, "]"); fclose(f);
}

int check_and_read(char *filename, char *buffer) {
    FILE *f = fopen(filename, "r");
    if (f) {
        if(fgets(buffer, 32, f) == NULL) strcpy(buffer, "");
        buffer[strcspn(buffer, "\r\n")] = 0;
        fclose(f); remove(filename); return 1;
    }
    return 0;
}

// ==========================================
//    MAIN LOOP
// ==========================================
int main() {
    printf("[ SYSTEM ACTIVE ] Dual Engine Mode (String + Bitwise)\n");
    FILE *f1 = fopen(LOG_LEGACY, "w"); if(f1){fprintf(f1, "[]"); fclose(f1);}
    FILE *f2 = fopen(LOG_DEFENSE, "w"); if(f2){fprintf(f2, "[]"); fclose(f2);}
    
    // --- INIT DUAL ENGINES ---
    TrieNode *string_root = createNode();       // Engine A
    BinaryNode *binary_root = createBinaryNode(); // Engine B

    // --- LOAD RULES INTO BOTH ENGINES ---
    FILE *f = fopen(BLOCKLIST_FILE, "r");
    char line[64];
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\r\n")] = 0; // Strip newline
            if(strlen(line) > 3) {
                // If it's a CIDR rule (has '/'), it ONLY goes to Binary Engine
                // If it's an exact IP, it goes to BOTH
                if(strchr(line, '/')) {
                    insert_cidr(binary_root, line); // CIDR Rule
                } else {
                    insert_ip_string(string_root, line); // Exact IP (Old way)
                    insert_cidr(binary_root, line);      // Exact IP (New way)
                }
            }
        }
        fclose(f);
    } else {
        printf("[WARN] blocked_ips.txt not found!\n");
    }

    char ip[32], info[64];

    while (1) {
        
        // --- RESET TRIGGER ---
        if (check_and_read("cmd_reset.txt", ip)) {
            count_old = 0; global_id_old = 1;
            count_new = 0; front = 0; rear = -1; global_id_new = 1;
            session_count = 0; 
            
            f1 = fopen(LOG_LEGACY, "w"); fprintf(f1, "[]"); fclose(f1);
            f2 = fopen(LOG_DEFENSE, "w"); fprintf(f2, "[]"); fclose(f2);
            printf("[ SYSTEM RESET ] All logs cleared.\n");
        }

        // --- LEGACY MODE ---
        if (check_and_read("cmd_legacy_normal.txt", ip)) {
            if (analyze_ip(ip, info)) { add_log_old(ip, 'G', "[ALLOW] Traffic Authorized"); save_logs_old(); }
        }
        if (check_and_read("cmd_legacy_attack.txt", ip)) {
            if (analyze_ip(ip, info)) { 
                for(int i=0; i<10; i++) add_log_old(ip, 'R', "[CRITICAL] DoS Flood Detected"); 
                save_logs_old(); 
            }
        }

        // --- NEW FEATURE: ADVANCED CIDR CHECK (Comparison Mode) ---
        if (check_and_read("cmd_binary_check.txt", ip)) {
            if (analyze_ip(ip, info)) {
                int string_hit = check_ip_string(string_root, ip);
                int binary_hit = check_binary_trie(binary_root, ip);

                if (string_hit) {
                     add_log_new(ip, 'R', "[BOTH] Blocked by Exact Match");
                } else if (binary_hit) {
                     // This is the money shot: String missed, but Binary caught it!
                     add_log_new(ip, 'R', "[CIDR] Blocked by Subnet Rule (Bitwise)");
                } else {
                     add_log_new(ip, 'G', "[PASS] Clean Traffic");
                }
                save_logs_new();
            }
        }

        // --- DEFENSE SYSTEM (Standard Button) ---
        if (check_and_read("cmd_defense_normal.txt", ip)) {
            if (analyze_ip(ip, info)) {
                // We use the Bitwise engine here because it is superior (handles both CIDR and Exact)
                if (check_binary_trie(binary_root, ip)) {
                    add_log_new(ip, 'R', "[BLOCK] Blacklisted IP");
                } 
                else if (!check_rate_limit(ip)) {
                    add_log_new(ip, 'R', "[DROP] Rate Limit Exceeded");
                } 
                else {
                    add_log_new(ip, 'G', "[PASS] Access Granted");
                }
                save_logs_new();
            }
        }

        if (check_and_read("cmd_defense_attack.txt", ip)) {
            if (analyze_ip(ip, info)) {
                int blocked = check_binary_trie(binary_root, ip);
                for(int i=0; i<5; i++) {
                    if (blocked) add_log_new(ip, 'R', "[TRIE] Pattern Match: BLOCKED");
                    else add_log_new(ip, 'G', "[WARN] High Traffic - NO RULE");
                }
                save_logs_new();
            }
        }

        SLEEP_MS(50); 
    }
    return 0;
}