#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h> 

#define LOG_LEGACY "logs.json"
#define LOG_DEFENSE "logs_defense.json"
#define TRIE_VIEW "trie_view.json"
#define BLOCKLIST_FILE "blocked_ips.txt"

// ==========================================
// UTILITY: ADVANCED IP ANALYSIS
// ==========================================
// Returns 1 if valid, 0 if invalid. Writes reason to 'result'.
int analyze_ip(char *ip, char *result) {
    if (ip == NULL || strlen(ip) == 0) {
        strcpy(result, "EMPTY INPUT");
        return 0;
    }

    // Check for non-numeric characters (excluding dots)
    for (int i = 0; i < strlen(ip); i++) {
        if ((ip[i] < '0' || ip[i] > '9') && ip[i] != '.') {
            strcpy(result, "SYNTAX ERROR (Char)");
            return 0;
        }
    }

    int a = -1, b = -1, c = -1, d = -1;
    int dots = 0;
    for(int i=0; i<strlen(ip); i++) if(ip[i] == '.') dots++;
    
    if(dots != 3) {
        strcpy(result, "FORMAT ERROR (Need 3 dots)");
        return 0;
    }

    if (sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d) != 4) {
        strcpy(result, "FORMAT ERROR");
        return 0;
    }

    if (a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255 || d < 0 || d > 255) {
        strcpy(result, "RANGE ERROR (0-255)");
        return 0;
    }

    // Identify Class
    char class_type[10];
    if (a >= 1 && a <= 126) strcpy(class_type, "Class A");
    else if (a >= 128 && a <= 191) strcpy(class_type, "Class B");
    else if (a >= 192 && a <= 223) strcpy(class_type, "Class C");
    else strcpy(class_type, "Other");

    sprintf(result, "Valid (%s)", class_type);
    return 1;
}

// ==========================================
// PART 1: LEGACY SYSTEM LOGGING
// ==========================================
typedef struct { int id; char ip[16]; char type; char info[50]; } LogLegacy;
LogLegacy history_old[1000];
int count_old = 0;
int global_id_old = 1;

void add_log_old(char *ip, char type, char *info) {
    history_old[count_old].id = global_id_old++;
    strcpy(history_old[count_old].ip, ip);
    history_old[count_old].type = type;
    strcpy(history_old[count_old].info, info);
    count_old++;
    if(count_old >= 1000) count_old = 0; 
}

void save_logs_old() {
    FILE *f = fopen(LOG_LEGACY, "w");
    if (!f) return;
    fprintf(f, "[");
    for (int i = 0; i < count_old; i++) {
        fprintf(f, "{\"id\": %d, \"ip\": \"%s\", \"type\": \"%c\", \"info\": \"%s\"}", 
            history_old[i].id, history_old[i].ip, history_old[i].type, history_old[i].info);
        if (i < count_old - 1) fprintf(f, ",");
    }
    fprintf(f, "]");
    fclose(f);
}

// ==========================================
// PART 2: DEFENSE SYSTEM (Trie + Queue)
// ==========================================
#define QUEUE_SIZE 50 
typedef struct TrieNode { struct TrieNode *children[11]; int is_blocked; } TrieNode;
typedef struct { int id; char ip[16]; char type; char info[60]; } LogDefense;

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
        if(ip[i] == '\n' || ip[i] == '\r') continue;
        int idx = getIndex(ip[i]);
        if (idx < 0 || idx > 10) continue; 
        if (!curr->children[idx]) curr->children[idx] = createNode();
        curr = curr->children[idx];
    }
    curr->is_blocked = 1;
    if (display_count < 100) {
        strncpy(display_list[display_count], ip, 15);
        display_list[display_count][15] = '\0';
        char *pos; if ((pos=strchr(display_list[display_count], '\n'))) *pos = '\0';
        display_count++;
    }
}

int check_ip(TrieNode *root, char *ip) {
    TrieNode *curr = root;
    for (int i = 0; ip[i]; i++) {
        if(ip[i] == '\n' || ip[i] == '\r') continue;
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
    while (fgets(line, sizeof(line), f)) insert_ip(root, line);
    fclose(f);
}

void save_trie_rules() {
    FILE *f = fopen(TRIE_VIEW, "w");
    if (!f) return;
    fprintf(f, "[");
    for(int i=0; i<display_count; i++) {
        fprintf(f, "{\"ip\": \"%s\", \"status\": \"BLOCKED\"}", display_list[i]);
        if(i < display_count - 1) fprintf(f, ",");
    }
    fprintf(f, "]");
    fclose(f);
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
    FILE *f = fopen(LOG_DEFENSE, "w");
    if (!f) return;
    fprintf(f, "[");
    int idx = front;
    for(int k=0; k<count_new; k++) {
        fprintf(f, "{\"id\": %d, \"ip\": \"%s\", \"type\": \"%c\", \"info\": \"%s\"}", 
            history_new[idx].id, history_new[idx].ip, history_new[idx].type, history_new[idx].info);
        if (k < count_new - 1) fprintf(f, ",");
        idx = (idx + 1) % QUEUE_SIZE;
    }
    fprintf(f, "]");
    fclose(f);
}

// ==========================================
// PART 3: MAIN LOOP
// ==========================================
int check_and_read(char *filename, char *buffer) {
    FILE *f = fopen(filename, "r");
    if (f) {
        if(fgets(buffer, 32, f) == NULL) strcpy(buffer, ""); 
        fclose(f);
        remove(filename);
        return 1;
    }
    return 0;
}

int main() {
    srand(time(NULL));
    printf("[ SYSTEM READY ] Running Packet Inspector & Trie Firewall...\n");
    
    // Init Logs
    FILE *f1 = fopen(LOG_LEGACY, "w"); if(f1){fprintf(f1, "[]"); fclose(f1);}
    FILE *f2 = fopen(LOG_DEFENSE, "w"); if(f2){fprintf(f2, "[]"); fclose(f2);}
    
    // Init Defense
    TrieNode *root = createNode();
    load_blocklist(root);
    save_trie_rules();

    char input_ip[32];
    char analysis[64];

    while (1) {
        // --- 1. LEGACY (PACKET INSPECTOR) ---
        if (check_and_read("cmd_legacy_normal.txt", input_ip)) {
            // GREEN BUTTON
            int is_valid = analyze_ip(input_ip, analysis);
            if (is_valid) {
                // Only log if it's a real IP
                add_log_old(input_ip, 'G', "Good Traffic (Authorized)");
                save_logs_old();
            } 
            // If invalid, DO NOTHING (No log update)
        }
        
        if (check_and_read("cmd_legacy_attack.txt", input_ip)) {
            // RED BUTTON
            int is_valid = analyze_ip(input_ip, analysis);
            
            if (is_valid) {
                // Only log if it's a real IP (e.g. 192.168.1.1)
                // This simulates a "Valid Packet" being used for a "DoS Attack"
                char flood_msg[64];
                sprintf(flood_msg, "DoS FLOOD (%s)", analysis); // e.g. DoS FLOOD (Valid Class C)
                
                for(int i=0; i<15; i++) add_log_old(input_ip, 'R', flood_msg);
                save_logs_old();
            }
            // If invalid (e.g. 999.999), DO NOTHING.
        }

        // --- 2. DEFENSE (TRIE FIREWALL) ---
        if (check_and_read("cmd_defense_normal.txt", input_ip)) {
            // Defense also only accepts valid IPs
            int is_valid = analyze_ip(input_ip, analysis);
            if (is_valid) {
                int blocked = check_ip(root, input_ip);
                add_log_new(input_ip, blocked ? 'R' : 'G', blocked ? "BLOCKED BY TRIE" : "Allowed");
                save_logs_new();
            }
        }
        if (check_and_read("cmd_defense_attack.txt", input_ip)) {
            int is_valid = analyze_ip(input_ip, analysis);
            if (is_valid) {
                int blocked = check_ip(root, input_ip);
                for(int i=0; i<5; i++) {
                    add_log_new(input_ip, blocked ? 'R' : 'G', blocked ? "INSTANT BLOCK (Trie)" : "Allowed (No Rule)");
                }
                save_logs_new();
            }
        }

        Sleep(50);
    }
    return 0;
}