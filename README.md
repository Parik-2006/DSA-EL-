# 🛡️ DSA-EL — DHIF-Cuckoo IP Filtering System

> A C and Flask-based network-defense simulation that classifies IPv4 addresses as **Authorized, Blocked, or DoS Attack** using a custom Dual-Hash Integer-Fold (DHIF) Cuckoo Hashing engine.

## 🎯 Project Overview

DSA-EL demonstrates how data structures and algorithms can enable fast IP access-control lookups. Instead of scanning an IP list sequentially, the C backend converts each IPv4 address into a 32-bit integer and checks two candidate positions across two hash tables. A Flask dashboard lets users test IP scenarios, view classifications and compare lookup approaches.

## ⚙️ How It Works

```text
Enter / select IPv4 address
          ↓
     Flask Dashboard
          ↓
    Validate IP address
          ↓
     C DHIF Engine
          ↓
 IPv4 → 32-bit integer
          ↓
 Two hash positions (table1 / table2)
          ↓
   Cuckoo Hash Lookup
          ↓
 Authorized / Blocked / DoS Attack
          ↓
 Dashboard result and latency
```

**Core algorithm**
- Two hash tables, each with 197 slots.
- Two integer-fold hash functions; the second uses an 8-bit rotation and symmetric salt when required.
- Cuckoo displacement handles collisions, with a maximum of 500 kicks per insertion.
- Lookup checks at most two candidate slots: **O(1)**.

**Status codes:** `1 = AUTHORIZED`, `0 = BLOCKED`, `2 = DOS_ATTACK`.

## 🖥️ Features & Tech Stack

**Features**
- Interactive dashboard with IPv4 validation and authorize/block actions.
- Predefined authorized, blocked and DoS simulation IP scenarios.
- Activity tables and backend lookup-latency display.
- Algorithm comparison: Linear Search, String Matching, Binary Trie, Stride-4 Trie and DHIF-Cuckoo.
- Hash-table explanation and blocked-IP list.

**Tech stack**
- **Algorithm/backend:** C, GCC
- **Web application:** Python, Flask, Gunicorn
- **Frontend:** HTML, CSS, JavaScript
- **Communication:** local text files and JSON logs between Flask and the C process

## 🚀 Run Locally

**Requirements:** Python 3.8+, pip, GCC and a Unix-like environment for the C backend.

```bash
git clone https://github.com/Parik-2006/DSA-EL-.git
cd DSA-EL-

python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

gcc backend.c -o backend
```

Open two terminals in the project directory:

**Terminal 1 — start the C engine**
```bash
./backend
```

**Terminal 2 — start Flask**
```bash
python app.py
```

Open **http://127.0.0.1:5000** in your browser. Keep both processes running for backend-driven actions.

## ⚠️ Scope & Limitations

This is an academic simulation, not a production firewall. It does not inspect live packets or enforce operating-system firewall rules. DoS/DDoS scenarios and impact metrics are simulated; comparison timings are illustrative, not controlled benchmark results. Flask and C communicate through local files and must share the same working directory.
