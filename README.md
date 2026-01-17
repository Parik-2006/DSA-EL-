# 🛡️ Next-Gen Firewall Simulation (DSA Project)

A high-performance Network Security demonstration comparing a **Legacy Firewall** (Array-based) vs. a **Modern Defense System** (Trie-based).

This project demonstrates how advanced Data Structures (Prefix Trees, Circular Queues) are used in real-world cybersecurity to handle DDoS attacks and high-speed packet filtering.

---

## 🚀 Project Overview

### **1. Legacy System (The "Before")**
* **Logic:** Uses simple Arrays and Linear Search ($O(N)$).
* **Weakness:** As the blocklist grows, the system slows down. It cannot handle IP spoofing or rapid flood attacks effectively.
* **Behavior:** Simulates a basic packet inspector that flags traffic but fails to block floods instantly.

### **2. Defense System (The "After")**
* **Logic:** Uses a **Prefix Tree (Trie)** for routing and a **Circular Queue** for memory management.
* **Strength:** Lookup speed is Constant Time ($O(1)$), regardless of how many IPs are blocked.
* **Features:**
    * **Real-time Packet Filtering:** Instantly drops packets from the blocklist.
    * **Rate Limiting:** Drops connections if an IP sends >3 requests in 10 seconds.
    * **Input Validation:** Strictly validates IPv4 format (Class A/B/C).
    * **Live Threat Feed:** Loads blocked IPs from `blocked_ips.txt`.

---

## 📂 Project Structure

```text
/Project_Root
│── backend.c           # The "Brain" (C Code). Runs Trie, Queue, and Logic.
│── app.py              # The "Bridge" (Python/Flask). Connects C backend to Web UI.
│── blocked_ips.txt     # Data Source. List of real malicious IPs.
│── requirements.txt    # Python dependencies.
│
├── /templates
│   ├── index.html      # Home Page (Choose System).
│   ├── dashboard.html  # Legacy System Interface.
│   └── defense.html    # Defense System Interface.