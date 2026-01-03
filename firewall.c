#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h> 

#define LOG_FILE "logs.json"

typedef struct {
    int id;
    char ip[16];
    char type; // 'G' or 'R'
    char info[50];
} Log;

Log history[1000];
int count = 0;
int global_id = 1;

void add_log(char *ip, char type, char *info) {
    history[count].id = global_id++;
    strcpy(history[count].ip, ip);
    history[count].type = type;
    strcpy(history[count].info, info);
    count++;
    if(count >= 1000) count = 0; 
}

void save_logs() {
    FILE *f = fopen(LOG_FILE, "w");
    if (!f) return;

    fprintf(f, "[");
    for (int i = 0; i < count; i++) {
        fprintf(f, "{\"id\": %d, \"ip\": \"%s\", \"type\": \"%c\", \"info\": \"%s\"}", 
            history[i].id, history[i].ip, history[i].type, history[i].info);
        if (i < count - 1) fprintf(f, ",");
    }
    fprintf(f, "]");
    fclose(f);
}

// Helper to safely check and delete command files
int check_and_delete(char *filename) {
    FILE *f = fopen(filename, "r");
    if (f) {
        fclose(f); // Close immediately so we can delete
        // CRITICAL FIX: Only return TRUE if we successfully deleted it
        if (remove(filename) == 0) {
            return 1; // Success: We claimed the command
        }
    }
    return 0; // Failed: File locked or didn't exist
}

int main() {
    printf("[ SYSTEM READY ] Waiting for manual triggers...\n");
    
    // Clear logs on start
    FILE *f = fopen(LOG_FILE, "w"); if(f){fprintf(f, "[]"); fclose(f);}

    while (1) {
        // 1. CHECK GREEN BUTTON
        if (check_and_delete("cmd_normal.txt")) {
            printf(">> Processing Normal Traffic...\n");
            add_log("192.168.1.5", 'G', "Valid User Request");
            save_logs();
        }

        // 2. CHECK RED BUTTON
        if (check_and_delete("cmd_attack.txt")) {
            printf(">> Processing Single Attack Spike...\n");
            // Generate exactly ONE burst of 15 packets
            for(int i=0; i<15; i++) {
                add_log("6.6.6.6", 'R', "DDOS FLOOD BLOCKED");
            }
            save_logs();
        }

        Sleep(50); // 50ms polling
    }
    return 0;
}