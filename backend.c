#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#define BLOCKLIST_FILE "blocked_ips.txt"
#define LOGS_FILE "simulation_logs.json"
#define CMD_FILE "cmd_trigger.txt"
#define SLEEP_MS(ms) usleep((ms) * 1000)

// Table size remains 197 (Prime) to handle ~100 IPs with low collision
#define TABLE_SIZE 197
#define MAX_KICKS 500
#define DHIF_SYMMETRIC_SALT 0x9e37u

#define AUTHORIZED 1
#define BLOCKED 0
#define DOS_ATTACK 2

typedef struct {
    uint32_t ip;
    int status_code;
    int occupied;
} HashEntry;

static HashEntry table1[TABLE_SIZE];
static HashEntry table2[TABLE_SIZE];

// [Hashing and Cuckoo Logic remains exactly as per your implementation]
static int ip_to_uint32(const char *ip, uint32_t *out) {
    unsigned int a, b, c, d;
    if (sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return 0;
    if (a > 255 || b > 255 || c > 255 || d > 255) return 0;
    *out = ((a << 24) | (b << 16) | (c << 8) | d);
    return 1;
}

static uint32_t dhif_hash1(uint32_t ip_int) {
    uint32_t upper = (ip_int >> 16) & 0xFFFF;
    uint32_t lower = ip_int & 0xFFFF;
    return (upper ^ lower) % TABLE_SIZE;
}

static uint32_t dhif_hash2(uint32_t ip_int) {
    uint32_t upper_fold = (ip_int >> 16) & 0xFFFF;
    uint32_t lower_fold = ip_int & 0xFFFF;

    if (upper_fold == lower_fold) {
        ip_int = (ip_int + DHIF_SYMMETRIC_SALT) & 0xFFFFFFFFu;
    }

    uint32_t rotated = (ip_int << 8) | (ip_int >> 24);
    uint32_t upper = (rotated >> 16) & 0xFFFF;
    uint32_t lower = rotated & 0xFFFF;
    return (upper ^ lower) % TABLE_SIZE;
}

static int cuckoo_lookup(uint32_t ip, int *status_out) {
    uint32_t idx1 = dhif_hash1(ip);
    if (table1[idx1].occupied && table1[idx1].ip == ip) {
        *status_out = table1[idx1].status_code;
        return 1;
    }
    uint32_t idx2 = dhif_hash2(ip);
    if (table2[idx2].occupied && table2[idx2].ip == ip) {
        *status_out = table2[idx2].status_code;
        return 1;
    }
    return 0;
}

static int cuckoo_insert(uint32_t ip, int status_code) {
    uint32_t idx1 = dhif_hash1(ip);
    if (table1[idx1].occupied && table1[idx1].ip == ip) {
        table1[idx1].status_code = status_code;
        return 1;
    }
    if (!table1[idx1].occupied) {
        table1[idx1].ip = ip;
        table1[idx1].status_code = status_code;
        table1[idx1].occupied = 1;
        return 1;
    }
    uint32_t idx2 = dhif_hash2(ip);
    if (table2[idx2].occupied && table2[idx2].ip == ip) {
        table2[idx2].status_code = status_code;
        return 1;
    }
    if (!table2[idx2].occupied) {
        table2[idx2].ip = ip;
        table2[idx2].status_code = status_code;
        table2[idx2].occupied = 1;
        return 1;
    }
    HashEntry entry = {ip, status_code, 1};
    uint32_t idx = idx1;
    int in_table1 = 1;
    for (int kick = 0; kick < MAX_KICKS; kick++) {
        HashEntry *slot = in_table1 ? &table1[idx] : &table2[idx];
        HashEntry displaced = *slot;
        *slot = entry;
        if (!displaced.occupied) return 1;
        entry = displaced;
        if (in_table1) {
            idx = dhif_hash2(entry.ip);
            in_table1 = 0;
        } else {
            idx = dhif_hash1(entry.ip);
            in_table1 = 1;
        }
    }
    return 0;
}

static void write_json_response(const char *status, int status_code, const char *ip, double latency_ms) {
    FILE *f = fopen(LOGS_FILE, "w");
    if (f) {
        fprintf(f, "{\"status\":\"%s\",\"status_code\":%d,\"ip\":\"%s\",\"latency\":\"%.3fms\"}",
                status, status_code, ip, latency_ms);
        fclose(f);
    }
    printf("{\"status\":\"%s\",\"status_code\":%d,\"ip\":\"%s\",\"latency\":\"%.3fms\"}\n",
           status, status_code, ip, latency_ms);
    fflush(stdout);
}

static double elapsed_ms(struct timespec start, struct timespec end) {
    double seconds = (double)(end.tv_sec - start.tv_sec);
    double nanos = (double)(end.tv_nsec - start.tv_nsec) / 1000000.0;
    return (seconds * 1000.0) + nanos;
}

static void insert_ip_literal(const char *ip_str, int status_code) {
    uint32_t ip;
    if (ip_to_uint32(ip_str, &ip)) {
        cuckoo_insert(ip, status_code);
    }
}

static void load_authorized_ips(void) {
    // Insertion order enforces expected Cuckoo placements.
    insert_ip_literal("10.10.10.10", AUTHORIZED);
    insert_ip_literal("10.0.0.1", AUTHORIZED);
    insert_ip_literal("222.10.10.10", AUTHORIZED);
    insert_ip_literal("10.5.5.5", AUTHORIZED);
    insert_ip_literal("10.1.1.100", AUTHORIZED);
    insert_ip_literal("10.255.0.1", AUTHORIZED);
    insert_ip_literal("8.8.8.8", AUTHORIZED);
    insert_ip_literal("1.1.1.1", AUTHORIZED);
    insert_ip_literal("55.10.10.1", AUTHORIZED);
    insert_ip_literal("100.20.30.40", AUTHORIZED);
    insert_ip_literal("120.5.5.5", AUTHORIZED);

    insert_ip_literal("172.16.0.10", AUTHORIZED);
    insert_ip_literal("172.17.5.5", AUTHORIZED);
    insert_ip_literal("172.31.255.254", AUTHORIZED);
    insert_ip_literal("172.20.10.1", AUTHORIZED);
    insert_ip_literal("172.16.100.5", AUTHORIZED);
    insert_ip_literal("128.50.1.1", AUTHORIZED);
    insert_ip_literal("150.10.10.10", AUTHORIZED);
    insert_ip_literal("160.1.1.1", AUTHORIZED);
    insert_ip_literal("180.50.50.50", AUTHORIZED);
    insert_ip_literal("190.10.20.30", AUTHORIZED);

    insert_ip_literal("192.168.1.1", AUTHORIZED);
    insert_ip_literal("192.168.0.105", AUTHORIZED);
    insert_ip_literal("192.168.1.50", AUTHORIZED);
    insert_ip_literal("192.168.10.10", AUTHORIZED);
    insert_ip_literal("192.168.100.1", AUTHORIZED);
    insert_ip_literal("200.100.50.1", AUTHORIZED);
    insert_ip_literal("208.67.222.222", AUTHORIZED);
    insert_ip_literal("210.1.1.1", AUTHORIZED);
    insert_ip_literal("220.50.10.1", AUTHORIZED);

    insert_ip_literal("224.0.0.1", AUTHORIZED);
    insert_ip_literal("224.0.0.5", AUTHORIZED);
    insert_ip_literal("224.0.0.251", AUTHORIZED);
    insert_ip_literal("224.0.0.18", AUTHORIZED);
    insert_ip_literal("224.0.0.22", AUTHORIZED);
    insert_ip_literal("239.1.1.1", AUTHORIZED);
    insert_ip_literal("239.255.0.1", AUTHORIZED);
    insert_ip_literal("232.5.5.5", AUTHORIZED);
    insert_ip_literal("233.1.1.1", AUTHORIZED);
    insert_ip_literal("225.10.10.10", AUTHORIZED);

    insert_ip_literal("240.0.0.1", AUTHORIZED);
    insert_ip_literal("255.255.255.255", AUTHORIZED);
    insert_ip_literal("250.1.1.1", AUTHORIZED);
    insert_ip_literal("245.0.0.5", AUTHORIZED);
    insert_ip_literal("252.10.10.10", AUTHORIZED);
    insert_ip_literal("254.100.100.1", AUTHORIZED);
    insert_ip_literal("248.5.5.5", AUTHORIZED);
    insert_ip_literal("241.1.1.1", AUTHORIZED);
    insert_ip_literal("242.2.2.2", AUTHORIZED);
    insert_ip_literal("253.3.3.3", AUTHORIZED);
}

// ================================================================
// UPDATED: 25 DOS ATTACK IPS (5 per Class)
// ================================================================
static void load_dos_simulation_ips(void) {
    insert_ip_literal("10.99.1.1", DOS_ATTACK);
    insert_ip_literal("10.99.1.2", DOS_ATTACK);
    insert_ip_literal("10.99.1.3", DOS_ATTACK);
    insert_ip_literal("10.99.1.4", DOS_ATTACK);
    insert_ip_literal("10.99.1.5", DOS_ATTACK);

    insert_ip_literal("172.16.88.1", DOS_ATTACK);
    insert_ip_literal("172.16.88.2", DOS_ATTACK);
    insert_ip_literal("172.16.88.3", DOS_ATTACK);
    insert_ip_literal("172.16.88.4", DOS_ATTACK);
    insert_ip_literal("172.16.88.5", DOS_ATTACK);

    insert_ip_literal("192.168.77.1", DOS_ATTACK);
    insert_ip_literal("192.168.77.2", DOS_ATTACK);
    insert_ip_literal("192.168.77.3", DOS_ATTACK);
    insert_ip_literal("192.168.77.4", DOS_ATTACK);
    insert_ip_literal("192.168.77.5", DOS_ATTACK);

    insert_ip_literal("224.0.0.50", DOS_ATTACK);
    insert_ip_literal("224.0.0.51", DOS_ATTACK);
    insert_ip_literal("224.0.0.52", DOS_ATTACK);
    insert_ip_literal("224.0.0.53", DOS_ATTACK);
    insert_ip_literal("224.0.0.54", DOS_ATTACK);

    insert_ip_literal("240.0.0.70", DOS_ATTACK);
    insert_ip_literal("240.0.0.71", DOS_ATTACK);
    insert_ip_literal("240.0.0.72", DOS_ATTACK);
    insert_ip_literal("240.0.0.73", DOS_ATTACK);
    insert_ip_literal("240.0.0.74", DOS_ATTACK);
}

static int search_ip(uint32_t ip_int, int *status_out) {
    return cuckoo_lookup(ip_int, status_out);
}

static void load_from_file(const char *path, int limit) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[128];
    int inserted = 0;
    while (fgets(line, sizeof(line), f) && inserted < limit) {
        char ip_str[32];
        uint32_t ip;
        if (sscanf(line, "%31s", ip_str) != 1) continue;
        if (!ip_to_uint32(ip_str, &ip)) continue;
        if (cuckoo_insert(ip, BLOCKED)) inserted++;
    }
    fclose(f);
}

int main(void) {
    printf("[SYSTEM] Cuckoo DHIF Engine Ready - Table Size: %d\n", TABLE_SIZE);
    
    // Initialization sequence
    load_authorized_ips();
    load_from_file(BLOCKLIST_FILE, 25);
    load_dos_simulation_ips();

    char cmd_buf[128];
    char cmd[32];
    char ip_str[32];

    while (1) {
        FILE *cf = fopen(CMD_FILE, "r");
        if (cf) {
            if (fgets(cmd_buf, sizeof(cmd_buf), cf)) {
                if (sscanf(cmd_buf, "%31s %31s", cmd, ip_str) == 2) {
                    fclose(cf);
                    remove(CMD_FILE);

                    struct timespec start, end;
                    uint32_t ip_int;

                    clock_gettime(CLOCK_MONOTONIC, &start);

                    if (!ip_to_uint32(ip_str, &ip_int)) {
                        clock_gettime(CLOCK_MONOTONIC, &end);
                        write_json_response("BLOCKED", BLOCKED, ip_str, elapsed_ms(start, end));
                        continue;
                    }

                    if (strcmp(cmd, "authorize") == 0) {
                        cuckoo_insert(ip_int, AUTHORIZED);
                    } else if (strcmp(cmd, "block") == 0) {
                        cuckoo_insert(ip_int, BLOCKED);
                    }

                    int status = BLOCKED;
                    int found = search_ip(ip_int, &status);
                    clock_gettime(CLOCK_MONOTONIC, &end);

                    if (found && status == AUTHORIZED) {
                        write_json_response("AUTHORIZED", AUTHORIZED, ip_str, elapsed_ms(start, end));
                    } else if (found && status == DOS_ATTACK) {
                        write_json_response("DOS_ATTACK", DOS_ATTACK, ip_str, elapsed_ms(start, end));
                    } else {
                        write_json_response("BLOCKED", BLOCKED, ip_str, elapsed_ms(start, end));
                    }
                } else { fclose(cf); }
            } else { fclose(cf); }
        }
        SLEEP_MS(100);
    }
    return 0;
}