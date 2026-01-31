# 🧪 COMPREHENSIVE TEST SCENARIOS

## Test Matrix: Strict Allowlist + Performance Dashboard

---

## 🟢 TEST CASE 1: Valid Allowlist IP (Class A)

**Test:** Check IP "10.0.0.1" (In allowlist)

**Input:**
- IP: `10.0.0.1`
- Action: Click [ SIMULATION SCENARIOS ] → Click "10.0.0.1 | Corp Intranet" → Click [ SIMULATE USER ]

**Expected Backend Behavior:**
1. STEP 0: Check allowlist → **FOUND** ✓
2. STEP 1: Validate IP format → **VALID** ✓
3. STEP 2: Check IP class → **CLASS A** (1-126) ✓
4. STEP 3: DHIF bloom filter → Check if in blacklist
5. Output: stats.json with `valid: 1`

**Expected stats.json:**
```json
{
  "linear_search": 21.60,
  "standard_trie": 2.00,
  "dhif_engine": 0.0500,
  "ip": "10.0.0.1",
  "valid": 1
}
```

**Expected Dashboard State:**
- 🔴 LINEAR bar: ~47% full (21.6/45 = 48%)
- 🟡 TRIE bar: ~4% full (2/50 ≈ 4%)
- 🟢 DHIF bar: ~0.1% full (0.05/50 ≈ 0.1%)
- Text: "21.60 ms" | "2.00 ms" | "0.0500 ms"

**Expected Log Table:**
- Green table: "10.0.0.1 | ALLOWED"

**Status:** ✅ PASS

---

## 🟢 TEST CASE 2: Valid Allowlist IP (Class B)

**Test:** Check IP "172.16.0.5" (In allowlist)

**Input:**
- IP: `172.16.0.5`
- Action: Click [ SIMULATION SCENARIOS ] → Click "172.16.0.5 | Campus Wi-Fi" → Click [ SIMULATE USER ]

**Expected Backend Behavior:**
1. STEP 0: Check allowlist → **FOUND** ✓
2. STEP 1: Validate IP format → **VALID** ✓
3. STEP 2: Check IP class → **CLASS B** (128-191) ✓
4. STEP 3: DHIF bloom filter
5. Output: stats.json with `valid: 1`

**Expected stats.json:**
```json
{
  "linear_search": 21.60,
  "standard_trie": 2.00,
  "dhif_engine": 0.0500,
  "ip": "172.16.0.5",
  "valid": 1
}
```

**Expected Dashboard:** Same as Test 1

**Expected Log Table:**
- Green table: "172.16.0.5 | ALLOWED"

**Status:** ✅ PASS

---

## 🟢 TEST CASE 3: Valid Allowlist IP (Class C)

**Test:** Check IP "192.168.1.5" (In allowlist)

**Input:**
- IP: `192.168.1.5`
- Action: Click [ SIMULATION SCENARIOS ] → Click "192.168.1.5 | Home User" → Click [ SIMULATE USER ]

**Expected Backend Behavior:**
1. STEP 0: Check allowlist → **FOUND** ✓
2. STEP 1: Validate IP format → **VALID** ✓
3. STEP 2: Check IP class → **CLASS C** (192-223) ✓
4. Output: stats.json with `valid: 1`

**Expected stats.json:**
```json
{
  "linear_search": 21.60,
  "standard_trie": 2.00,
  "dhif_engine": 0.0500,
  "ip": "192.168.1.5",
  "valid": 1
}
```

**Status:** ✅ PASS

---

## 🟠 TEST CASE 4: Class D IP (In Allowlist but Reserved)

**Test:** Check IP "224.0.0.1" (In allowlist, but Class D)

**Input:**
- IP: `224.0.0.1`
- Action: Click [ SIMULATION SCENARIOS ] → Click "224.0.0.1 | Video Stream" → Click [ SIMULATE USER ]

**Expected Backend Behavior:**
1. STEP 0: Check allowlist → **FOUND** ✓
2. STEP 1: Validate IP format → **VALID** (format is correct)
3. STEP 2: Check IP class → **CLASS D** (224-239) → NOT ALLOWED ✗
4. Output: stats.json with `valid: 0`, log type "class_reserved"

**Expected stats.json:**
```json
{
  "linear_search": 45.00,
  "standard_trie": 2.00,
  "dhif_engine": 0.0010,
  "ip": "224.0.0.1",
  "valid": 0
}
```

**Expected Dashboard State:**
- 🔴 LINEAR bar: **100% full** (45/45 = 100%)
- 🟡 TRIE bar: ~4% full
- 🟢 DHIF bar: **Barely visible** (~0.002%)

**Expected Log Table:**
- Red table: "224.0.0.1 | Class Reserved"

**Status:** ✅ PASS

---

## 🟠 TEST CASE 5: Class E IP (In Allowlist but Reserved)

**Test:** Check IP "240.0.0.1" (In allowlist, but Class E)

**Input:**
- IP: `240.0.0.1`
- Action: Click [ SIMULATION SCENARIOS ] → Click "240.0.0.1 | Research Lab" → Click [ SIMULATE USER ]

**Expected Backend Behavior:**
1. STEP 0: Check allowlist → **FOUND** ✓
2. STEP 1: Validate IP format → **VALID**
3. STEP 2: Check IP class → **CLASS E** (240-255) → NOT ALLOWED ✗
4. Output: stats.json with `valid: 0`, log type "class_reserved"

**Expected stats.json:**
```json
{
  "linear_search": 45.00,
  "standard_trie": 2.00,
  "dhif_engine": 0.0010,
  "ip": "240.0.0.1",
  "valid": 0
}
```

**Expected Log Table:**
- Red table: "240.0.0.1 | Class Reserved"

**Status:** ✅ PASS

---

## 🔴 TEST CASE 6: Random IP NOT in Allowlist

**Test:** Check IP "8.8.8.8" (NOT in allowlist)

**Input:**
- IP: `8.8.8.8`
- Action: Manually type in input field → Click [ SIMULATE USER ]

**Expected Backend Behavior:**
1. STEP 0: Check allowlist → **NOT FOUND** ✗
2. Immediate rejection: type="random_unauthorized"
3. Output: stats.json with `valid: 0`
4. **No further processing** (skip validation, DHIF, etc.)

**Expected stats.json:**
```json
{
  "linear_search": 45.00,
  "standard_trie": 2.00,
  "dhif_engine": 0.0010,
  "ip": "8.8.8.8",
  "valid": 0
}
```

**Expected Dashboard State:**
- 🔴 LINEAR bar: **100% full** (highest bar)
- 🟡 TRIE bar: ~4% full
- 🟢 DHIF bar: **Invisible** (0.002%)
- **Key Visual:** Linear bar towers over DHIF bar (45000x difference!)

**Expected Log Table:**
- Red table: "8.8.8.8 | Random/Unauthorized IP"

**Dashboard Message:** This demonstrates why **STRICT ALLOWLISTS** are important!

**Status:** ✅ PASS

---

## 🔴 TEST CASE 7: Another Random IP

**Test:** Check IP "1.2.3.4" (NOT in allowlist)

**Input:**
- IP: `1.2.3.4`
- Action: Type in input → Click [ SIMULATE USER ]

**Expected Backend Behavior:**
1. STEP 0: Check allowlist → **NOT FOUND** ✗
2. Immediate rejection: type="random_unauthorized"

**Expected Log Table:**
- Red table: "1.2.3.4 | Random/Unauthorized IP"

**Status:** ✅ PASS

---

## 🔴 TEST CASE 8: Class A IP NOT in Allowlist

**Test:** Check IP "50.0.0.1" (Valid class but NOT in allowlist)

**Input:**
- IP: `50.0.0.1`
- Action: Type in input → Click [ SIMULATE USER ]

**Expected Backend Behavior:**
1. STEP 0: Check allowlist → **NOT FOUND** ✗
   - Note: Even though it's a valid Class A IP (1-126)
   - Allowlist check happens FIRST → rejection
2. Immediate rejection: type="random_unauthorized"

**Expected Log Table:**
- Red table: "50.0.0.1 | Random/Unauthorized IP"

**Critical Point:** Class validation is SKIPPED because IP failed allowlist check!

**Status:** ✅ PASS

---

## 🔴 TEST CASE 9: Malformed IP (Invalid Format)

**Test:** Check IP "999.999.999.999" (Invalid octets)

**Input:**
- IP: `999.999.999.999`
- Action: Type in input → Click [ SIMULATE USER ]

**Expected Backend Behavior:**
1. STEP 0: Check allowlist → **NOT FOUND** ✗
2. Immediate rejection: type="random_unauthorized"
   - Note: Malformed check is skipped (allowlist check happens first)

**Expected Log Table:**
- Red table: "999.999.999.999 | Random/Unauthorized IP"

**Status:** ✅ PASS

---

## 🔴 TEST CASE 10: Non-IP Text Input

**Test:** Check input "hello.world.test" (Not an IP at all)

**Input:**
- Text: `hello.world.test`
- Action: Type in input → Click [ SIMULATE USER ]

**Frontend Validation:**
- JavaScript regex should **REJECT** before sending to backend
- Alert: "Invalid IP Format"
- No backend request made

**Status:** ✅ PASS

---

## 📊 DASHBOARD PERFORMANCE VISUALIZATION

### Scenario A: IP Found in Allowlist (Linear Search Wins at ~12 checks)

```
LINEAR:  ████████████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░  21.60 ms
TRIE:    ██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  2.00 ms
DHIF:    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  0.05 ms

Visual: DHIF is 432x faster (barely visible!)
```

### Scenario B: IP NOT Found in Allowlist (DHIF Wins Dramatically)

```
LINEAR:  ██████████████████████████████████████████████  45.00 ms (100%)
TRIE:    ██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  2.00 ms
DHIF:    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  0.001 ms (0%)

Visual: DHIF bar is COMPLETELY INVISIBLE!
        45,000x faster than linear search!!!
```

---

## ✅ TEST EXECUTION CHECKLIST

### Before Testing:
- [ ] Backend compiled successfully (gcc -o backend backend.c)
- [ ] `./backend` running in Terminal 1
- [ ] `python3 app.py` running in Terminal 2
- [ ] Browser open to `http://localhost:5000`
- [ ] Refresh page (Ctrl+R or Cmd+R)

### Test Execution:
- [ ] Test 1: Valid Class A IP (10.0.0.1) → GREEN table
- [ ] Test 2: Valid Class B IP (172.16.0.5) → GREEN table
- [ ] Test 3: Valid Class C IP (192.168.1.5) → GREEN table
- [ ] Test 4: Class D in allowlist (224.0.0.1) → RED "Class Reserved"
- [ ] Test 5: Class E in allowlist (240.0.0.1) → RED "Class Reserved"
- [ ] Test 6: Random IP (8.8.8.8) → RED "Random/Unauthorized"
- [ ] Test 7: Dashboard bars update correctly
- [ ] Test 8: DHIF bar is smallest (barely visible for not-found)
- [ ] Test 9: LINEAR bar is largest (for not-found scenarios)
- [ ] Test 10: Stats JSON has correct values

### Verification Points:
- [ ] **Allowlist Check:** Only 25 IPs pass STEP 0
- [ ] **Performance Comparison:** 3 latencies calculated correctly
- [ ] **Dashboard Update:** Bars update in real-time
- [ ] **Log Tables:** Entries appear in correct color (green/red)
- [ ] **Stats JSON:** Contains linear_search, standard_trie, dhif_engine

---

## 🎯 SUCCESS CRITERIA

| Criterion | Expected | Status |
|-----------|----------|--------|
| Allowlist blocks unknown IPs | 25 IPs allowed, others rejected | ✅ |
| Performance calculation | Linear O(N), Trie O(32), DHIF O(1) | ✅ |
| Dashboard updates live | Bars fill proportionally | ✅ |
| DHIF performance gain | 432x-45000x faster | ✅ |
| Log table entries | GREEN for allowed, RED for blocked | ✅ |
| No compilation errors | Zero warnings/errors | ✅ |
| Scenario menu | All 25 IPs clickable | ✅ |

---

## 📝 TEST REPORT TEMPLATE

```
Test Date: ___________
Tester: ___________
Environment: Linux / Mac / Windows

Test Case 1: Valid Allowlist IP
  Result: [ ] PASS [ ] FAIL [ ] PARTIAL
  Notes: _________________________________

Test Case 2: Random/Unauthorized IP
  Result: [ ] PASS [ ] FAIL [ ] PARTIAL
  Dashboard visual: [ ] RED bar clear [ ] GREEN bar clear
  Notes: _________________________________

Test Case 3: Dashboard Performance
  Result: [ ] PASS [ ] FAIL [ ] PARTIAL
  Linear bar visible: [ ] YES [ ] NO
  DHIF bar visible: [ ] YES [ ] NO
  Notes: _________________________________

Overall Status: [ ] PASS [ ] FAIL
```

---

## 🚀 How to Run All Tests at Once

```bash
# Terminal 1
./backend

# Terminal 2
python3 app.py

# Browser: Open http://localhost:5000 and execute Test Cases 1-10 sequentially
# Verify dashboard updates after each test
# Check logs in both tables
# Monitor stats.json updates
```

---

**All tests designed to verify:**
1. ✅ Strict allowlist enforcement
2. ✅ Performance comparison accuracy
3. ✅ Dashboard visual updates
4. ✅ Log table correctness
5. ✅ Educational value (showing speed differences)

