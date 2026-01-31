# Quick Reference - Code Snippets for IP Classification System

## HTML Reference Panel Snippet
```html
<button class="doc-btn" style="right: 280px;" onclick="toggleReference()">[ IP CLASS REFERENCE ]</button>

<div id="referencePanel" class="panel" style="display: none; position: absolute; top: 70px; right: 280px; width: 450px;">
    <h2 style="color: #fff; margin-top: 0; border-bottom: 1px solid #333; padding-bottom: 10px;">IP CLASS REFERENCE</h2>
    <table style="width: 100%; border-collapse: collapse; font-size: 12px; background: #050505;">
        <tr style="border-bottom: 1px solid #333;">
            <td style="padding: 8px; color: #888; font-weight: bold;">CLASS</td>
            <td style="padding: 8px; color: #888; font-weight: bold;">OCTET 1 RANGE</td>
            <td style="padding: 8px; color: #888; font-weight: bold;">STATUS</td>
        </tr>
        <tr style="border-bottom: 1px solid #333; background: #0a2a0a;">
            <td style="padding: 8px; color: #00ff00; font-weight: bold;">Class A</td>
            <td style="padding: 8px; color: #00ff00;">1 - 126</td>
            <td style="padding: 8px; color: #00ff00;">✓ ALLOWED</td>
        </tr>
        <tr style="border-bottom: 1px solid #333; background: #0a2a0a;">
            <td style="padding: 8px; color: #00ff00; font-weight: bold;">Class B</td>
            <td style="padding: 8px; color: #00ff00;">128 - 191</td>
            <td style="padding: 8px; color: #00ff00;">✓ ALLOWED</td>
        </tr>
        <tr style="border-bottom: 1px solid #333; background: #0a2a0a;">
            <td style="padding: 8px; color: #00ff00; font-weight: bold;">Class C</td>
            <td style="padding: 8px; color: #00ff00;">192 - 223</td>
            <td style="padding: 8px; color: #00ff00;">✓ ALLOWED</td>
        </tr>
        <tr style="border-bottom: 1px solid #333; background: #2a0a0a;">
            <td style="padding: 8px; color: #ff3333; font-weight: bold;">Class D</td>
            <td style="padding: 8px; color: #ff3333;">224 - 239</td>
            <td style="padding: 8px; color: #ff3333;">✗ MULTICAST</td>
        </tr>
        <tr style="background: #2a0a0a;">
            <td style="padding: 8px; color: #ff3333; font-weight: bold;">Class E</td>
            <td style="padding: 8px; color: #ff3333;">240 - 255</td>
            <td style="padding: 8px; color: #ff3333;">✗ EXPERIMENTAL</td>
        </tr>
    </table>
    <p style="font-size: 11px; color: #999; margin-top: 15px; padding-top: 10px; border-top: 1px solid #333;">
        <span style="color: #00ff00;">✓ Green Classes (A, B, C):</span> Standard unicast addresses. ALLOWED through firewall.<br>
        <span style="color: #ff3333;">✗ Red Classes (D, E):</span> Reserved for special use. BLOCKED and logged as policy violation.
    </p>
</div>
```

---

## C Backend Validation Structure
```c
typedef struct { 
    int valid;           // 1 if allowed, 0 if rejected
    int class_id;        // 1=A, 2=B, 3=C, 4=D, 5=E
    char reason[64];     // Rejection reason
} IPValidation;
```

---

## C Backend Validation Function
```c
IPValidation validate_ip_strict(const char *ip) {
    IPValidation result = {0, 0, ""};
    
    // [Format validation: dots, digits, etc.]
    // [Parse as 4 octets: a.b.c.d]
    
    // Determine class based on first octet
    if (a >= 1 && a <= 126) {
        result.class_id = 1;  // Class A
        result.valid = 1;
    } else if (a >= 128 && a <= 191) {
        result.class_id = 2;  // Class B
        result.valid = 1;
    } else if (a >= 192 && a <= 223) {
        result.class_id = 3;  // Class C
        result.valid = 1;
    } else if (a >= 224 && a <= 239) {
        result.class_id = 4;  // Class D (Multicast)
        strcpy(result.reason, "Multicast Reserved");
        result.valid = 0;
    } else if (a >= 240 && a <= 255) {
        result.class_id = 5;  // Class E (Experimental)
        strcpy(result.reason, "Experimental Reserved");
        result.valid = 0;
    }
    
    return result;
}
```

---

## C Backend Main Loop
```c
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

                // Valid IP - proceed with blacklist check, rate limiting, etc.
                int is_blocked = check_binary(bin_root, ip);
                // ... rest of simulation logic
            }
        }
    }
    SLEEP_MS(50);
}
```

---

## JSON Log Entry Examples

### Class D Rejection (Multicast)
```json
{
  "ip": "225.1.1.1",
  "status": "DROPPED",
  "desc": "Multicast Reserved",
  "table": "red"
}
```

### Class E Rejection (Experimental)
```json
{
  "ip": "245.100.50.200",
  "status": "DROPPED",
  "desc": "Experimental Reserved",
  "table": "red"
}
```

### Malformed IP
```json
{
  "ip": "999.999.999.999",
  "status": "DROPPED",
  "desc": "Malformed Header",
  "table": "red"
}
```

### Valid Class A IP
```json
{
  "ip": "10.0.0.1",
  "status": "ALLOWED",
  "desc": "0.05ms",
  "table": "green"
}
```

---

## Testing Commands

### Test Class D Rejection
```bash
# In simulator or via API
enter IP: 224.0.0.1 (Class D - Multicast)
click: SIMULATE USER
expected: RED table shows "Multicast Reserved"
```

### Test Class E Rejection
```bash
# In simulator or via API
enter IP: 240.0.0.1 (Class E - Experimental)
click: SIMULATE USER
expected: RED table shows "Experimental Reserved"
```

### Test Malformed Input
```bash
# In simulator
enter IP: 999.999.999.999 (Invalid)
click: SIMULATE USER
expected: RED table shows "Malformed Header"
```

### Test Valid Class A
```bash
# In simulator
enter IP: 10.20.30.40 (Class A)
click: SIMULATE USER
expected: GREEN table shows "ALLOWED" (if not on blacklist)
```

### Test Valid Class B
```bash
# In simulator
enter IP: 172.16.0.1 (Class B)
click: SIMULATE USER
expected: GREEN table shows "ALLOWED" (if not on blacklist)
```

### Test Valid Class C
```bash
# In simulator
enter IP: 192.168.1.1 (Class C)
click: SIMULATE USER
expected: GREEN table shows "ALLOWED" (if not on blacklist)
```

---

## Important Notes

1. **Non-Crashing Design**: Backend logs rejections without crashing
2. **Automatic Display**: Rejection reasons auto-populate in RED table after simulation
3. **Color Coding**: Green for allowed, Red for rejected
4. **Backward Compatible**: All existing functions work unchanged
5. **Production Ready**: Code compiled and tested successfully

