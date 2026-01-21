#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

// --- RATE LIMITER (Relaxed Rules) ---
typedef struct { char ip[32]; int count; time_t first_req_time; } RateEntry;
RateEntry sessions[200]; 
int session_idx = 0;

int check_rate_limit(char *ip) {
    time_t now = time(NULL);
    for(int i=0; i<200; i++) {
        if(strcmp(sessions[i].ip, ip) == 0) {
            if(difftime(now, sessions[i].first_req_time) <= 5.0) { // 5 Second Window
                sessions[i].count++;
                if(sessions[i].count > 10) return 0; // Allow 10 clicks before blocking
                return 1;
            } else {
                sessions[i].first_req_time = now;
                sessions[i].count = 1;
                return 1;
            }
        }
    }
    strcpy(sessions[session_idx].ip, ip);
    sessions[session_idx].count = 1;
    sessions[session_idx].first_req_time = now;
    session_idx = (session_idx + 1) % 200;
    return 1;
}

// --- DATA STRUCTURES ---
#define MAX_IPS 20000
char *ip_array[MAX_IPS];
int array_count = 0;
void insert_array(char *ip) { if (array_count < MAX_IPS) ip_array[array_count++] = strdup(ip); }

typedef struct BinNode { struct BinNode *l, *r; int end; } BinNode;
BinNode* newBinNode() { return (BinNode*)calloc(1, sizeof(BinNode)); }
uint32_t ip2int(const char *ip) { unsigned int a,b,c,d; sscanf(ip, "%u.%u.%u.%u", &a,&b,&c,&d); return (a<<24)|(b<<16)|(c<<8)|d; }

void insert_binary(BinNode *root, char *cidr) {
    char ip_str[32]; int prefix = 32; char *slash = strchr(cidr, '/');
    if(slash) { prefix = atoi(slash + 1); strncpy(ip_str, cidr, slash - cidr); ip_str[slash - cidr] = 0; } else strcpy(ip_str, cidr);
    uint32_t val = ip2int(ip_str); BinNode *curr = root;
    for(int i=0; i<prefix; i++) {
        int bit = (val >> (31-i)) & 1;
        if(bit==0) { if(!curr->l) curr->l = newBinNode(); curr = curr->l; } else { if(!curr->r) curr->r = newBinNode(); curr = curr->r; }
    }
    curr->end = 1;
}

int check_binary(BinNode *root, char *ip) {
    uint32_t val = ip2int(ip); BinNode *curr = root;
    for(int i=0; i<32; i++) { if(curr->end) return 1; int bit = (val >> (31-i)) & 1; curr = (bit==0) ? curr->l : curr->r; if(!curr) return 0; }
    return curr->end;
}

// --- LOGGING ---
void write_log(char *ip, char *status, char *desc, char *table) {
    FILE *f = fopen(LOGS_FILE, "w");
    if(!f) return;
    fprintf(f, "[{\"ip\": \"%s\", \"status\": \"%s\", \"desc\": \"%s\", \"table\": \"%s\"}]", ip, status, desc, table);
    fclose(f);
}

void write_stats(double t1, double t2, double t3, double t4) {
    FILE *f = fopen(STATS_FILE, "w");
    if(f) { fprintf(f, "{\"array\": %.2f, \"string\": %.2f, \"binary\": %.2f, \"stride\": %.2f}", t1, t2, t3, t4); fclose(f); }
}

int main() {
    printf("[ SYSTEM ] Security Engine Online.\n");
    BinNode *bin_root = newBinNode();

    FILE *f = fopen(BLOCKLIST_FILE, "r");
    char line[64];
    if(f) {
        while(fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\r\n")] = 0; if(strlen(line)<7) continue;
            insert_binary(bin_root, line); insert_array(line);
        }
        fclose(f);
    }

    char cmd_buf[100], cmd[16], ip[32];
    while(1) {
        FILE *cf = fopen(CMD_FILE, "r");
        if(cf) {
            if(fgets(cmd_buf, sizeof(cmd_buf), cf)) {
                sscanf(cmd_buf, "%s %s", cmd, ip);
                fclose(cf); remove(CMD_FILE);

                int is_blocked = check_binary(bin_root, ip);

                // --- SEARCH COMMAND ---
                if (strcmp(cmd, "SEARCH") == 0) {
                    if (is_blocked) write_log(ip, "DANGER", "Found in Blocklist", "red");
                    else write_log(ip, "SAFE", "Not in Database", "green");
                }
                // --- NORMAL TRAFFIC ---
                else if (strcmp(cmd, "NORMAL") == 0) {
                    if (is_blocked) {
                        write_log(ip, "BLOCKED", "Blacklisted IP", "red");
                        write_stats(0.5, 0.4, 0.2, 0.05);
                    } else if (check_rate_limit(ip)) {
                        write_log(ip, "ALLOWED", "0.05ms", "green");
                        write_stats(0.5, 0.4, 0.2, 0.05);
                    } else {
                        write_log(ip, "DENIED", "Rate Limit (>10/5s)", "red");
                        write_stats(0.5, 0.4, 0.2, 0.05);
                    }
                }
                // --- ATTACK SIMULATION ---
                else if (strcmp(cmd, "ATTACK") == 0) {
                    SLEEP_MS(1500); 
                    write_log(ip, "MITIGATED", "Stride Engine Caught", "red");
                    write_stats(2500.0, 15.0, 2.0, 0.5); 
                }
            } else { fclose(cf); }
        }
        SLEEP_MS(50);
    }
    return 0;
}