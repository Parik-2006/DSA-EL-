# 🛡️ Next-Gen Network Defense: Legacy vs. Trie-Based Firewall

> **A High-Performance Packet Filtering Engine.**
> *Built for Wire-Speed Security, Cloud Infrastructure, and DDoS Resilience.*

---

## 📖 Project Objective

Modern network security faces a massive challenge: as traffic volume increases, traditional firewalls become the bottleneck. Standard filtering methods often struggle with:

* **Linear Latency:** Sequential rule checking ($O(N)$) causes network lag during spikes.
* **Memory Inefficiency:** Storing millions of individual IP addresses wastes hardware resources.
* **Static Rigidity:** Lack of subnet awareness makes managing large ranges difficult.

**The Solution:**
This project demonstrates a **Multi-Stage Defense Architecture**. It contrasts legacy methods with modern **Longest Prefix Matching (LPM)** and **Probabilistic Hashing**, proving that constant-time ($O(1)$) filtering is possible even under heavy DDoS loads.

---

## ⚙️ The Core Technologies

The engine validates traffic through three sophisticated layers of networking theory:

### **1. The Fail-Fast Filter (IP Classes)**

The system instantly identifies the **Class (A, B, C, D, or E)** of an incoming IP. Using bitwise shifts, it rejects Class D (Multicast) and Class E (Experimental) traffic at the entry point to save CPU cycles.

### **2. Longest Prefix Match (Stride-4 Radix Trie)**

Instead of checking one IP at a time, the firewall uses **Subnet Masking** and **CIDR** logic.

* **The Theory:** By applying a mask (e.g., `255.255.255.0`), the firewall identifies the **Network ID**.
* **The Data Structure:** A **Stride-4 Radix Trie** "walks" the bits 4 at a time. This allows the firewall to block an entire network range with a single rule, always prioritizing the most specific (longest) match.

### **3. DHIF-Cuckoo Engine (O(1) Speed)**

For individual session verification, we use the **Dual-Hash Integer-Fold (DHIF)** algorithm.


$$Index = (IP_{High16} \oplus IP_{Low16}) \pmod{TableSize}$$

* **Cuckoo Hashing:** Resolves collisions by "kicking" older entries to alternative slots, ensuring that lookups remain constant speed regardless of table fullness.

---

## 🌍 Real-World Scenarios

This project simulates firewall performance across critical networking use cases:

| Scenario | The Challenge | The Solution |
| --- | --- | --- |
| **🚨 DDoS Attack** | 100,000+ spoofed IPs hitting the gate. | **Fail-Fast & DHIF:** Malicious packets are dropped in 0.005ms. |
| **🏢 Corporate LAN** | Managing 500+ employee devices. | **Subnet Masking:** One Trie rule manages the entire department. |
| **🌐 ISP Routing** | Routing traffic across global CIDR blocks. | **Stride-4 Radix:** High-speed jumps through bit-levels. |
| **🔄 Dynamic Access** | New devices joining via DHCP. | **Cuckoo Insert:** Instant, dynamic updates to the allowed list. |

---

## 🛠️ Tech Stack

* **Backend Engine:** C (High-Performance Core)
* **Web Logic:** Python (Flask API)
* **Frontend:** HTML5, CSS (Cyberpunk Dark Mode), JavaScript
* **Visualization:** Chart.js (Real-time Latency Benchmarking)

---

## 🚀 How to Run Locally

1. **Clone the Repository**
```bash
git clone https://github.com/Parik-2006/DSA-EL-
cd DSA-EL-

```


2. **Compile the C Engine**
```bash
gcc -O3 backend.c -o firewall_engine

```


3. **Install Python Dependencies**
```bash
pip install -r requirements.txt

```


4. **Launch the System**
```bash
python app.py

```



---

## ☁️ Deployment

This project is live on **Render**.

* The C-backend is integrated with the Flask wrapper to provide a real-time interactive simulation of the packet filtering process.
* **Live Link:** [https://dsa-el-2.onrender.com](https://dsa-el-2.onrender.com)

---

### 👨‍🏫 Acknowledgments

This project was developed as part of the **Data Structures and Applications (DSA-EL)** curriculum at **R V College of Engineering**. Special thanks to:

* **Ganashree K C**, Assistant Professor, CSE, RVCE.
* **Dr. Praveena T**, Associate Professor, CSE, RVCE.

---

### 📄 License

Educational Research Purpose - RVCE CSE 2025.

---

### 🤝 Contributing & Issues

This project is open for viewing. **Direct changes are restricted.**

* **Found a bug?** Please [Open a New Issue](https://www.google.com/search?q=https://github.com/Parik-2006/DSA-EL-/issues/new) and describe the problem.
* **Want to fix it?** Please Fork the repo and submit a Pull Request (PR) for review.

Would you like me to help you create a specific "How to contribute" guide for people who want to fork your project?
