# 🛡️ DSA-EL — DHIF-Cuckoo IP Filtering & Network Defense System

> A C-based network-defense simulation that uses a **Dual-Hash Integer-Fold (DHIF) Cuckoo Hashing engine** to classify IPv4 addresses as **Authorized, Blocked, or DoS Attack** traffic, with a Flask dashboard for interactive testing and visualization.

---

## 🎯 Project Objective

DSA-EL demonstrates how data structures and algorithms can support fast IP access-control decisions.

Instead of scanning an IP list sequentially for every lookup, the system converts an IPv4 address into a 32-bit integer and checks two deterministic hash-table positions. The C backend maintains authorization states using a two-table Cuckoo Hashing design, while the Flask web application provides a dashboard to submit scenarios, inspect results and view algorithm comparisons.

The project is an **academic simulation** of an IP filtering/network-defense workflow; it is not a production firewall and does not inspect or filter live network packets.

---

## ⚙️ How It Works

```text
User selects or enters an IPv4 address
                 ↓
        Flask Web Dashboard
                 ↓
     Input validation & scenario checks
                 ↓
        Command written to file
                 ↓
        C DHIF-Cuckoo Engine
                 ↓
     IPv4 string → 32-bit integer
                 ↓
       Two deterministic hash indices
                 ↓
       Cuckoo Hash Table Lookup
           ↙             ↘
      table1            table2
           ↘             ↙
        Status classification
                 ↓
    Backend response + lookup latency
                 ↓
       Flask dashboard / activity log
```

The Flask application and C engine communicate through small local files:
- `cmd_trigger.txt` carries a requested action and IP address to the C process.
- `simulation_logs.json` carries the latest backend result back to the web application.
- `blocked_ips.txt` supplies the blocked-IP entries loaded by the C engine.

---

## 🧠 Core Data Structure & Algorithm

### DHIF-Cuckoo Hashing

The backend implements a two-table Cuckoo Hashing structure using fixed-size arrays.

| Component | Implementation |
|---|---|
| Key | IPv4 address converted to a 32-bit unsigned integer |
| Tables | `table1` and `table2` |
| Table capacity | 197 slots per table |
| Hash functions | `dhif_hash1()` and `dhif_hash2()` |
| Collision handling | Cuckoo displacement / eviction |
| Maximum displacement attempts | 500 kicks |
| Entry fields | IP integer, status code and occupied flag |

### 1. IPv4 Integer Conversion

The C function `ip_to_uint32()` parses the four IPv4 octets, validates that each is in the range 0–255, and packs them into a 32-bit integer:

```text
a.b.c.d  →  (a << 24) | (b << 16) | (c << 8) | d
```

This allows the hash functions to operate on integer keys rather than repeatedly comparing address strings.

### 2. Dual Hash Functions

**Hash 1 — Integer folding**

```text
upper = (IP >> 16) & 0xFFFF
lower = IP & 0xFFFF
index1 = (upper XOR lower) % 197
```

**Hash 2 — Rotation and folding**

The second function first checks for equal upper and lower 16-bit halves. When they are equal, it applies the symmetric salt `0x9e37`. It then rotates the 32-bit value left by 8 bits, folds the resulting upper and lower halves with XOR, and takes the result modulo 197.

The two functions provide two candidate positions for an IP entry.

### 3. Cuckoo Insertion and Lookup

- An insertion first checks whether the key already exists at either candidate position.
- If a candidate slot is empty, the entry is stored there.
- If both candidate slots are occupied, the insertion starts a displacement sequence: the new entry replaces a slot and the displaced entry is moved to its alternate hash position.
- The process is limited to 500 kicks. The insertion function returns failure if it cannot complete within that limit.
- Lookup checks the corresponding position in table 1 and then table 2.

With the two candidate positions available, lookup performs at most two table-slot checks, giving **O(1) lookup time** for this two-choice design. Insertions can require multiple displacement steps.

---

## 🚦 IP Classification

The C backend uses these status codes:

| Status code | Classification | Meaning in this project |
|---:|---|---|
| `1` | 🟢 **AUTHORIZED** | IP is stored with authorized status |
| `0` | 🔴 **BLOCKED** | IP is blocked or loaded from the blocklist |
| `2` | 🟠 **DOS_ATTACK** | IP belongs to the predefined DoS simulation set |

The C engine initializes its tables with:
- A predefined set of 50 authorized IPv4 examples.
- Up to 25 valid entries read from `blocked_ips.txt`.
- 25 predefined DoS simulation IPs.

The dashboard also presents Class A–E example groups and separate DoS simulation scenarios. The class labels are used for the project's educational scenarios; they should not be treated as a complete modern IP allocation or routing policy.

---

## 🖥️ Dashboard Features

The Flask application provides the following routes and interactions:

- **Dashboard (`/`)** — main IP filtering and network-defense simulation interface.
- **Documentation (`/documentation`)** — project setup and algorithm explanation.
- **Algorithm comparison (`/comparision`)** — comparison of linear search, string matching, binary trie, stride-4 trie and DHIF-Cuckoo.
- **Hash table view (`/hashtable`)** — explanatory view of the two Cuckoo tables and example IP entries.
- **IP validation** — checks IPv4 dotted-decimal format and octet ranges.
- **Authorize / block actions** — submits an IP action to the C backend through the command file.
- **DoS / DDoS scenarios** — dashboard simulations using predefined test addresses and illustrative impact metrics.
- **Activity tables and metrics** — displays authorized and unauthorized activity, backend results and measured backend lookup latency.
- **Blocked IP list** — reads entries from `blocked_ips.txt`.

The dashboard includes illustrative comparison timings and simulated attack-impact metrics. These are demonstration values generated by the application, not results from a controlled benchmark or measurements of a live network.

---

## ⚖️ Algorithm Comparison

The documentation and comparison page present these approaches:

| Approach | Lookup idea | Complexity shown in project |
|---|---|---:|
| Linear Search | Scan entries sequentially | O(n) |
| String Matching | Compare address strings | O(n × m) |
| Binary Trie | Traverse address bits | O(32) for IPv4 |
| Stride-4 Trie | Traverse four bits per step | O(8) for IPv4 |
| **DHIF-Cuckoo** | Check two candidate hash positions | **O(1)** |

The comparison page's timing figures are illustrative dashboard values. Actual timings depend on hardware, compiler settings, implementation details and measurement conditions.

---

## 🧰 Tech Stack

### Backend & Algorithm
- **C** — IPv4 conversion, DHIF hash functions, Cuckoo insertion and lookup, backend command processing
- **GCC** — compiles the C backend

### Web Application
- **Python**
- **Flask**
- **Gunicorn** — WSGI server configuration

### Frontend
- HTML5
- CSS3
- JavaScript

### Data & Communication
- Text-based IP blocklist (`blocked_ips.txt`)
- JSON backend response / simulation log files
- File-based communication between Flask and the C process

---

## 📁 Project Structure

```text
DSA-EL-/
├── app.py                    # Flask application and HTTP routes
├── backend.c                 # DHIF-Cuckoo hashing engine in C
├── blocked_ips.txt           # Blocked IP entries loaded by the backend
├── stats.json                # Stored algorithm comparison values
├── requirements.txt          # Python dependencies
├── Procfile                  # Gunicorn process declaration
├── build.sh                  # C backend compilation script
├── start.sh                  # Gunicorn startup script
├── templates/
│   ├── index.html             # Main network-defense dashboard
│   ├── documentation.html     # Technical documentation page
│   ├── comparision.html       # Algorithm comparison page
│   └── hashtable.html         # Hash-table view
└── static/
    ├── css/
    │   └── main.css           # Dashboard styling
    └── js/
        └── main.js            # Dashboard interactions and simulations
```

Runtime files such as `cmd_trigger.txt` and `simulation_logs.json` are created/used during backend communication.

---

## 🚀 Run Locally

### Prerequisites

- Python 3.8 or later
- pip
- GCC compiler
- A Unix-like environment for the C backend as currently written (it uses POSIX `unistd.h` / `usleep()`)
- A modern web browser

### 1. Clone the repository

```bash
git clone https://github.com/Parik-2006/DSA-EL-.git
cd DSA-EL-
```

### 2. Install Python dependencies

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

On Windows PowerShell, activate the environment with:

```powershell
.venv\Scripts\Activate.ps1
```

### 3. Compile the C backend

```bash
gcc backend.c -o backend
```

Or use the included build script:

```bash
bash build.sh
```

### 4. Start the C engine

Open a terminal in the project directory and run:

```bash
./backend
```

Keep this process running. The engine polls for commands in `cmd_trigger.txt`.

### 5. Start the Flask dashboard

Open a second terminal in the same project directory, activate the virtual environment if needed, and run:

```bash
python app.py
```

### 6. Open the dashboard

Visit:

```text
http://127.0.0.1:5000
```

The dashboard can be opened without the C process, but backend-driven actions require the compiled C engine to be running alongside Flask.

---

## ⚠️ Scope & Limitations

- This project is an educational algorithm and network-defense simulation.
- It does not attach to a network interface, inspect real packets, or enforce firewall rules on an operating system.
- The dashboard's DoS/DDoS scenarios and impact metrics are simulations; they do not generate real attack traffic.
- Backend communication currently relies on local files, so the Flask application and C process must run with a shared working directory and suitable file permissions.
- Cuckoo insertion is bounded by 500 displacement attempts. The current implementation returns failure if insertion cannot be completed within that bound.
- The sample IP lists and blocklist are demonstration data, not a continuously updated threat-intelligence feed.

---

## 👥 Team

- Add project team members here.

---

## 📄 License

No explicit open-source license is currently specified in this repository. Unless a license is added, the project should be treated as an academic repository with all rights reserved by its contributors.

---

## 🔗 Repository

[DSA-EL — DHIF-Cuckoo IP Filtering & Network Defense System](https://github.com/Parik-2006/DSA-EL-)
