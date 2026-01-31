# IP Classification System - Visual Guide

## System Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    NEXT-GEN NETWORK DEFENSE                     │
│              Python Flask Frontend + C Backend                   │
└─────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                       FRONTEND (index.html)                       │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌─────────────────────┐      ┌──────────────────────┐           │
│  │ SYSTEM ARCHITECTURE │      │ IP CLASS REFERENCE   │ ← NEW     │
│  │      (button)       │      │     (button)         │           │
│  └─────────────────────┘      └──────────────────────┘           │
│         Opens docs panel            Opens reference panel        │
│                                                                   │
│  ┌──────────────────────────────────────────────────────┐        │
│  │          IP CLASS REFERENCE PANEL (NEW)              │        │
│  ├──────────────────────────────────────────────────────┤        │
│  │ CLASS | OCTET 1 RANGE | STATUS                       │        │
│  ├──────────────────────────────────────────────────────┤        │
│  │ A     | 1-126         | ✓ ALLOWED        (GREEN)    │        │
│  │ B     | 128-191       | ✓ ALLOWED        (GREEN)    │        │
│  │ C     | 192-223       | ✓ ALLOWED        (GREEN)    │        │
│  │ D     | 224-239       | ✗ MULTICAST      (RED)      │        │
│  │ E     | 240-255       | ✗ EXPERIMENTAL   (RED)      │        │
│  └──────────────────────────────────────────────────────┘        │
│                                                                   │
│  ┌─────────────────────┐      ┌──────────────────────┐           │
│  │ Input IP Address    │      │ [SIMULATE USER]      │           │
│  │ (e.g. 225.1.1.1)    │      │ [SIMULATE ATTACK]    │           │
│  └─────────────────────┘      │ [CLEAR DATA]         │           │
│                               └──────────────────────┘           │
│                                                                   │
│  ┌────────────────────┐      ┌────────────────────┐             │
│  │  AUTHORIZED LOGS   │      │   BLOCKED LOGS     │             │
│  │   (GREEN TABLE)    │      │   (RED TABLE)      │             │
│  ├────────────────────┤      ├────────────────────┤             │
│  │ No. | IP | Result  │      │ No. | IP | Reason  │             │
│  ├────────────────────┤      ├────────────────────┤             │
│  │ 1   | ... | ALLOW  │      │ 1   | ... | Multi  │             │
│  │ 2   | ... | ALLOW  │      │ 2   | ... | Exper  │             │
│  │ 3   | ... | ALLOW  │      │ 3   | ... | Malfor │             │
│  └────────────────────┘      └────────────────────┘             │
│          ↑                              ↑                        │
│          └──────────────────┬───────────┘                        │
│                             │ UPDATED HERE                       │
│                    (fetches rejection logs)                      │
└──────────────────────────────────────────────────────────────────┘
                             │
                    POST /trigger, GET /data
                             │
                             ▼
┌──────────────────────────────────────────────────────────────────┐
│                    FLASK APP (app.py)                            │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  @app.route("/trigger")  → Writes cmd_trigger.txt               │
│  @app.route("/data")     → Reads simulation_logs.json            │
│                                                                   │
└──────────────────────────────────────────────────────────────────┘
                             │
                        cmd_trigger.txt
                        simulation_logs.json
                             │
                             ▼
┌──────────────────────────────────────────────────────────────────┐
│              C BACKEND (backend.c) ← UPDATED                      │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  while(1) {                                                      │
│      Read cmd_trigger.txt                                       │
│      Parse IP address                                           │
│      │                                                           │
│      ▼                                                           │
│      IPValidation ip_val = validate_ip_strict(ip)  ← NEW FUNC   │
│      │                                                           │
│      ├─ Class A (1-126)      → valid=1 → Continue              │
│      ├─ Class B (128-191)    → valid=1 → Continue              │
│      ├─ Class C (192-223)    → valid=1 → Continue              │
│      ├─ Class D (224-239)    → valid=0 → Log "Multicast"       │
│      ├─ Class E (240-255)    → valid=0 → Log "Experimental"    │
│      └─ Malformed           → valid=0 → Log "Malformed"        │
│      │                                                           │
│      ▼                                                           │
│      If invalid: write_logs_batch(ip, "class_d/e/malformed")   │
│      If valid: Check blacklist, rate limit, etc.                │
│      │                                                           │
│      ▼                                                           │
│      Write simulation_logs.json with results                    │
│  }                                                               │
│                                                                   │
└──────────────────────────────────────────────────────────────────┘
                             │
                    simulation_logs.json
                  (with rejection reasons)
                             │
                             ▼
┌──────────────────────────────────────────────────────────────────┐
│                   /data Endpoint Response                         │
├──────────────────────────────────────────────────────────────────┤
│ {                                                                │
│   "stats": { ... },                                             │
│   "logs": [                                                     │
│     {"ip": "225.1.1.1", "status": "DROPPED",                   │
│      "desc": "Multicast Reserved", "table": "red"},   ← NEW    │
│     {"ip": "10.0.0.1", "status": "ALLOWED",                    │
│      "desc": "0.05ms", "table": "green"}                       │
│   ]                                                             │
│ }                                                                │
└──────────────────────────────────────────────────────────────────┘
                             │
                    Frontend displays logs
                    in respective tables
```

---

## Data Flow for Class D/E Rejection

```
INPUT: User enters "225.1.1.1" (Class D - Multicast)
  │
  ├─ Frontend validates: ✓ Valid IPv4 format
  │
  ├─ User clicks "SIMULATE USER"
  │
  ├─ Frontend sends: POST /trigger {"mode": "SIM_GOOD", "ip": "225.1.1.1"}
  │
  ├─ Flask writes: cmd_trigger.txt = "SIM_GOOD 225.1.1.1"
  │
  ├─ Backend reads: Parses "SIM_GOOD" and "225.1.1.1"
  │
  ├─ Backend calls: validate_ip_strict("225.1.1.1")
  │   └─ First octet = 225
  │   └─ 225 >= 224 && 225 <= 239 → Class D
  │   └─ Returns: {valid=0, class_id=4, reason="Multicast Reserved"}
  │
  ├─ Backend: ip_val.valid = 0, so:
  │   └─ write_logs_batch("225.1.1.1", "class_d_reserved", 1)
  │   └─ Writes: [{"ip": "225.1.1.1", "status": "DROPPED", 
  │               "desc": "Multicast Reserved", "table": "red"}]
  │
  ├─ Frontend polls: GET /data?t=<timestamp>
  │
  ├─ Flask responds with simulation_logs.json
  │
  ├─ Frontend displays in RED TABLE:
  │   No. | IP         | Reason
  │   1   | 225.1.1.1  | Multicast Reserved

OUTPUT: User sees rejection reason in RED "Blocked Logs" table
```

---

## Class Detection Algorithm

```
Function: validate_ip_strict(const char *ip) → IPValidation

Step 1: FORMAT VALIDATION
   - Check for valid IPv4 format (x.x.x.x)
   - Each octet must be 0-255
   - If invalid → return {valid=0, reason="Malformed Header"}

Step 2: PARSE OCTETS
   - sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d)
   - If fails → return {valid=0, reason="Malformed Header"}

Step 3: CLASS DETECTION (based on first octet 'a')
   - if (a >= 1 && a <= 126)      → Class A → {valid=1, class_id=1}
   - else if (a >= 128 && a <= 191) → Class B → {valid=1, class_id=2}
   - else if (a >= 192 && a <= 223) → Class C → {valid=1, class_id=3}
   - else if (a >= 224 && a <= 239) → Class D → {valid=0, reason="Multicast"}
   - else if (a >= 240 && a <= 255) → Class E → {valid=0, reason="Experimental"}
   - else if (a == 0)              → Special → {valid=0, reason="Malformed"}
   - else if (a == 127)            → Loopback → {valid=0, reason="Loopback"}

Step 4: RETURN RESULT
   - If invalid (valid=0) and should be rejected:
     * Backend logs rejection to simulation_logs.json
     * Frontend displays rejection reason in RED table
   - If valid (valid=1):
     * Backend proceeds with blacklist check
     * May still be blocked for other reasons
```

---

## Integration Points

### 1. HTML (index.html)
- **Button:** `[ IP CLASS REFERENCE ]` at top-right
- **Panel:** Shows IP class table with color coding
- **Function:** `toggleReference()` to show/hide panel

### 2. Backend (backend.c)
- **New Structure:** `IPValidation` (valid, class_id, reason)
- **New Function:** `validate_ip_strict(ip)` returns validation result
- **New Log Types:** "class_d_reserved", "class_e_reserved", "malformed"
- **Main Loop:** Checks IP validity BEFORE processing

### 3. Frontend (index.html)
- **Display:** Rejection reasons in RED table automatically
- **Polling:** Fetches logs from `/data` endpoint
- **No Changes Needed:** Flask already handles JSON correctly

---

## Test Cases

| Input | Frontend Check | Backend Check | RED Table Output |
|-------|---|---|---|
| 10.0.0.1 (Class A) | ✓ Valid | ✓ Class A allowed | (not in red) |
| 172.16.0.1 (Class B) | ✓ Valid | ✓ Class B allowed | (not in red) |
| 192.168.1.1 (Class C) | ✓ Valid | ✓ Class C allowed | (not in red) |
| 224.0.0.1 (Class D) | ✓ Valid | ✗ Class D rejected | "Multicast Reserved" |
| 240.0.0.1 (Class E) | ✓ Valid | ✗ Class E rejected | "Experimental Reserved" |
| 999.999.999.999 | ✗ Invalid | (not reached) | - |
| 256.1.1.1 | ✗ Invalid | ✗ Malformed | "Malformed Header" |
| abc.def.ghi.jkl | ✗ Invalid | (not reached) | - |

---

## Compilation & Deployment

✅ **Status:** Ready for Production

```bash
# Compile
gcc -o backend backend.c

# Run Backend
./backend &

# Run Flask
python3 app.py

# Test from Browser
# Visit: http://localhost:5000
# Enter IP like 225.1.1.1
# Click: SIMULATE USER
# Result: See "Multicast Reserved" in RED table
```

---

## Key Takeaways

1. ✅ **IP Classes A, B, C are ALLOWED** (Green in reference panel)
2. ✅ **IP Classes D, E are REJECTED** (Red in reference panel)
3. ✅ **Rejection reasons logged to JSON** and displayed automatically
4. ✅ **Backend doesn't crash** on invalid/restricted IPs
5. ✅ **Frontend shows why IPs were blocked** in RED table
6. ✅ **System Reference panel** teaches users about IP classification
7. ✅ **Backward compatible** with all existing functions

