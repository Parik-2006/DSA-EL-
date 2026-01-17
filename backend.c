#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 

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

// IP ANALYSIS
int analyze_ip(char *ip, char *result) {
    if (!ip || strlen(ip) == 0) { strcpy(result, "EMPTY"); return 0; }
    int a, b, c, d;
    if (sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d) != 4) { strcpy(result, "FORMAT ERROR"); return 0; }
    if (a<0||a>255||b<0||b>255||c<0||c>255||d<0||d>255) { strcpy(result, "RANGE ERROR"); return 0; }
    char cls[10] = "Other";
    if (a<=126) strcpy(cls, "Class A"); else if (a<=191) strcpy(cls, "Class B"); else if (a<=223) strcpy(cls, "Class C");
    sprintf(result, "Valid (%s)", cls);
    return 1;
}

// LOGGING STRUCTURES
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

// DEFENSE STRUCTURES
#define QUEUE_SIZE 50 
typedef struct TrieNode { struct TrieNode *children[11]; int is_blocked; } TrieNode;
typedef struct { int id; char ip[16]; char type; char info[80]; } LogDefense;

LogDefense history_new[QUEUE_SIZE];
int front = 0, rear = -1, count_new = 0, global_id_new = 1;
char display_list[100][16];
int display_count = 0;

TrieNode* createNode() {
    TrieNode *node = (TrieNode*)malloc(sizeof(TrieNode));
    node->is_blocked = 0;
    for(int i=0; i<11; i++) node->children[i] = NULL;
    return node;
}
int getIndex(char c) { return (c == '.') ? 10 : c - '0'; }

void insert_ip(TrieNode *root, char *ip) {
    TrieNode *curr = root;
    for (int i = 0; ip[i]; i++) {
        if(ip[i] < '0' && ip[i] != '.') continue;
        int idx = getIndex(ip[i]);
        if (!curr->children[idx]) curr->children[idx] = createNode();
        curr = curr->children[idx];
    }
    curr->is_blocked = 1;
    if (display_count < 100) { strcpy(display_list[display_count++], ip); }
}

int check_ip(TrieNode *root, char *ip) {
    TrieNode *curr = root;
    for (int i = 0; ip[i]; i++) {
        if(ip[i] < '0' && ip[i] != '.') continue;
        int idx = getIndex(ip[i]);
        if (!curr->children[idx]) return 0;
        curr = curr->children[idx];
    }
    return curr && curr->is_blocked;
}

void load_blocklist(TrieNode *root) {
    FILE *f = fopen(BLOCKLIST_FILE, "r");
    if (!f) { insert_ip(root, "6.6.6.6"); return; }
    char line[32];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if(strlen(line) > 6) insert_ip(root, line);
    }
    fclose(f);
}

void save_trie_rules() {
    FILE *f = fopen(TRIE_VIEW, "w"); if (!f) return;
    fprintf(f, "[");
    for(int i=0; i<display_count; i++) {
        fprintf(f, "{\"ip\": \"%s\", \"status\": \"BLOCKED\"}", display_list[i]);
        if(i < display_count - 1) fprintf(f, ",");
    }
    fprintf(f, "]"); fclose(f);
}

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

int main() {
    printf("[ SYSTEM ACTIVE ] Rate Limit: 3 req / 10 sec\n");
    FILE *f1 = fopen(LOG_LEGACY, "w"); if(f1){fprintf(f1, "[]"); fclose(f1);}
    FILE *f2 = fopen(LOG_DEFENSE, "w"); if(f2){fprintf(f2, "[]"); fclose(f2);}
    
    TrieNode *root = createNode();
    load_blocklist(root);
    save_trie_rules();

    char ip[32], info[64];

    while (1) {
        // --- LEGACY ---
        if (check_and_read("cmd_legacy_normal.txt", ip)) {
            if (analyze_ip(ip, info)) { add_log_old(ip, 'G', "[ALLOW] Traffic Authorized"); save_logs_old(); }
        }
        if (check_and_read("cmd_legacy_attack.txt", ip)) {
            if (analyze_ip(ip, info)) { 
                for(int i=0; i<10; i++) add_log_old(ip, 'R', "[CRITICAL] DoS Flood Detected"); 
                save_logs_old(); 
            }
        }

        // --- DEFENSE SYSTEM ---
        if (check_and_read("cmd_defense_normal.txt", ip)) {
            if (analyze_ip(ip, info)) {
                if (check_ip(root, ip)) {
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
                int blocked = check_ip(root, ip);
                for(int i=0; i<5; i++) {
                    if (blocked) add_log_new(ip, 'R', "[TRIE] Pattern Match: BLOCKED");
                    else add_log_new(ip, 'G', "[WARN] High Traffic - NO RULE");
                }
                save_logs_new();
            }
        }

        SLEEP_MS(50); // Replaced Sleep() with Cross-Platform Macro
    }
    return 0;
}