# DSA-EL: Network Defense System

A high-performance Network Defense System demonstrating the power of advanced data structures and algorithms for real-time IP filtering and threat detection. This project showcases the DHIF (Dynamic Hash-Indexed Filter) algorithm - a proprietary solution achieving sub-millisecond lookup times for network security applications.

---

## Project Overview

DSA-EL is a Flask-based Network Defense System demo focused on IP authorization, blocked-IP checks, and algorithm comparison. The UI is served from HTML templates with shared CSS/JS assets, while the backend provides lightweight routes for page rendering, stats/log retrieval, and triggering simulations.

### What It Does

- Serves the dashboard and documentation pages from `templates/`
- Loads blocked IPs from `blocked_ips.txt` for client-side validation
- Emits a command file (`cmd_trigger.txt`) when simulations are triggered
- Exposes JSON endpoints for stats and logs used by the front end

### Key Features

1. **Strict Allowlist System:** 50 carefully curated IPs across all IP classes (A, B, C, D, E)
2. **Real-Time Threat Detection:** Integration with blocked IP database
3. **Rate Limiting Protection:** Prevents algorithmic abuse (3 requests/5 seconds)
4. **Attack Simulation:** DoS and DDoS testing capabilities
5. **Visual Algorithm Comparison:** Live performance benchmarking
6. **Professional Documentation:** Comprehensive technical guides

---

## Quick Start

### Prerequisites

- Python 3.8 or higher
- Modern web browser
- GCC compiler (optional, for `backend.c`)

### Installation

```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/DSA-EL-.git
cd DSA-EL-

# Install dependencies
pip install -r requirements.txt
```

### Run Locally (Flask)

```bash
python app.py
```

Access the dashboard at: `http://localhost:5000`

### Run with Gunicorn

```bash
bash start.sh
```

Set `PORT` to change the default port (5000):

```bash
PORT=8000 bash start.sh
```

### Optional: Build the C Backend

```bash
bash build.sh
```

---

## Project Structure

```
DSA-EL-/
├── app.py                      # Flask application entry point (56 lines)
├── backend.c                   # C implementation of DHIF algorithm
├── requirements.txt            # Python dependencies
├── blocked_ips.txt             # Blacklist database (50+ malicious IPs)
├── .gitignore                  # Git ignore configuration
├── templates/
│   ├── index.html              # Main dashboard interface (310 lines)
│   ├── documentation.html      # Technical documentation & setup guide
│   └── references.html         # Academic references & algorithm sources
├── static/
│   ├── css/
│   │   └── main.css            # Consolidated stylesheet (100 lines)
│   └── js/
│       └── main.js             # Application logic & IP validation (447 lines)
├── stats.json                  # Runtime statistics
├── simulation_logs.json        # Attack simulation logs
├── start.sh                    # Quick start script
└── README.md                   # This file
```

### File Statistics
- **Total Lines of Code:** ~1000 (highly optimized)
- **HTML Templates:** 583 lines
- **Stylesheet:** 100 lines  
- **JavaScript Logic:** 447 lines
- **Backend:** 56 lines (Flask server)

---

## Features in Detail

### 1. IP Authorization System

The system enforces a strict allowlist of 50 IPs categorized by class:

- **Class A:** Large networks (10.0.0.0/8, public ranges)
- **Class B:** Medium networks (172.16.0.0/12, public ranges)  
- **Class C:** Small networks (192.168.0.0/16, public ranges)
- **Class D:** Multicast addresses (224.0.0.0/4)
- **Class E:** Experimental/reserved (240.0.0.0/4)

### 2. Rate Limiting

Protects the DHIF algorithm from abuse:
- **Limit:** 3 authorization attempts per IP within 5 seconds
- **Action:** 30-second automatic block on violation
- **Purpose:** Prevents automated attacks and algorithmic degradation

### 3. Blacklist Integration

The system loads pre-blocked IPs from `blocked_ips.txt`:
- Real-world malicious IP addresses
- Threat intelligence database integration
- Fast DoS simulation on cached IPs
- Optimized performance for known threats

### 4. Attack Simulation

**DoS (Denial of Service):**
- Single-source attack simulation
- Performance impact metrics
- Algorithm degradation testing

**DDoS (Distributed Denial of Service):**
- Multi-source coordinated attacks (3-5 random IPs)
- Botnet behavior simulation
- System resilience testing

---

## Algorithm Comparison

The dashboard provides real-time comparison of 5 search algorithms:

| Algorithm | Time Complexity | Avg. Time | Use Case |
|-----------|----------------|-----------|----------|
| Linear Search | O(n) | 50-60ms | Small datasets |
| String Matching | O(n*m) | 30-35ms | Pattern matching |
| Binary Search | O(log n) | 5-6ms | Sorted datasets |
| Stride Search | O(n/k) | 1.5-2ms | Medium datasets |
| **DHIF** | **O(1)** | **0.04-0.06ms** | **Production systems** |

---

## Development Guide

### For Backend Developers

**Required Skills:**
- Python Flask framework
- Data structures (hash tables, tries)
- File I/O and parsing
- C programming (optional, for optimization)

**Key Files:**
- `app.py`: Flask routes and endpoints (56 lines)
- `backend.c`: Core DHIF algorithm implementation

### For Frontend Developers

**Required Skills:**
- HTML5/CSS3
- JavaScript ES6+
- DOM manipulation
- Fetch API

**Key Files:**
- `templates/index.html`: Main dashboard interface (310 lines)
- `templates/documentation.html`: Technical documentation (273 lines)
- `templates/references.html`: Algorithm references (272 lines)
- `static/css/main.css`: Consolidated stylesheet (100 lines)
- `static/js/main.js`: Application logic (447 lines)

### Code Organization

**Separation of Concerns:**
- HTML structure in `/templates/` (clean, semantic markup)
- Styling in `/static/css/main.css` (centralized theming)
- Logic in `/static/js/main.js` (IP validation, attack simulation, metrics)
- Backend in `app.py` (server routes and data handling)

### For Network Security Students

**Concepts Covered:**
- IP address classification
- CIDR notation
- DoS/DDoS attack patterns
- Rate limiting strategies
- Hash functions and collision handling

---

## API Endpoints

### GET `/`
Home dashboard - main command center with 50 IP scenarios

### GET `/documentation`
Complete technical documentation with setup guides

### GET `/references`
Academic references and algorithm sources

### POST `/trigger`
Trigger backend operations
```json
{
  "mode": "authorize|dos|ddos",
  "ip": "192.168.1.1"
}
```

### GET `/blocked_ips`
Retrieve blacklist database (plain text)

### GET `/data`
Get runtime statistics and logs
```json
{
  "stats": {...},
  "logs": [...]
}
```

---

## Configuration

### Rate Limiting Settings

Edit in `static/js/main.js` (function `checkRateLimit`):
```javascript
const RATE_LIMIT = {
    maxAttempts: 3,        // Max attempts
    timeWindowMs: 5000,    // 5 seconds
    blockDurationMs: 30000 // 30 seconds
};
```

### IP Allowlist

Modify `validIPs` object in `static/js/main.js` to customize the 50 IP allowlist across 5 classes (A, B, C, D, E).

**Current Structure:**
```javascript
const validIPs = {
    'A': [...10 IPs],      // Class A - Large networks
    'B': [...10 IPs],      // Class B - Medium networks
    'C': [...10 IPs],      // Class C - Small networks
    'D': [...10 IPs],      // Class D - Multicast
    'E': [...10 IPs]       // Class E - Experimental
};
```

### Blacklist Database

Edit `blocked_ips.txt` to add/remove blocked IPs (one per line).

### Styling & Themes

Customize colors and animations in `static/css/main.css`:
- Primary color (Cyan): `#00d4ff`
- Success color (Green): `#00ff88`
- Danger color (Red): `#ff4444`
- Secondary color (Purple): `#8a2be2`

---

## Testing

### Test Scenarios

1. **Normal Authorization:** Select valid IPs and authorize
2. **Rate Limiting:** Click same IP 4+ times rapidly
3. **Blacklist Check:** Try authorizing pre-blocked IPs
4. **DoS Simulation:** Attack a single target IP
5. **DDoS Simulation:** Simulate distributed attack
6. **Invalid IP:** Try IPs not in allowlist

### Performance Testing

The dashboard automatically benchmarks all 5 algorithms in real-time, displaying:
- Execution time comparisons
- Visual bar charts
- Performance degradation under attack

---

## What's New

### Recent Updates

- ✨ **Code Refactoring:** Extracted 700+ lines of inline CSS/JS into external files
  - Reduced `index.html` from 922 to 310 lines (70% reduction)
  - Created `static/css/main.css` (100 lines consolidation)
  - Created `static/js/main.js` (447 lines organization)
  
- 🎨 **UI/UX Enhancements:**
  - Dark theme with cyan accents (#00d4ff)
  - Smooth hover effects with shadows and transforms
  - Enhanced footer with improved styling
  - Responsive design across all pages
  
- 📚 **Documentation:** Added comprehensive references page (`/references`)
  
- 🔐 **Security:** Cleaned codebase, optimized performance, proper `.gitignore`

---

## Git Configuration

### .gitignore Rules

The `.gitignore` file excludes:
- All markdown documentation files (except `README.md`)
- Compiled executables (`*.exe`, `backend`)
- Python cache files (`__pycache__/`, `*.pyc`)
- Log and temporary files
- IDE configuration files (`.vscode/`, `.idea/`)

**Uploaded to repository:**
- All source code files (app.py, templates/, static/)
- Configuration files (requirements.txt, .gitignore)
- Data files (blocked_ips.txt)
- README.md (main documentation)

---

## Troubleshooting

### Port Already in Use

```bash
pkill -f "python app.py"
python app.py
```

### Backend Compilation Issues

```bash
gcc backend.c -o backend.exe
```

If errors occur, ensure GCC is installed:
```bash
gcc --version
```

### Browser Cache Issues

Hard refresh: `Ctrl+Shift+R` (Windows/Linux) or `Cmd+Shift+R` (Mac)

---

## Contributing

We welcome contributions! Here's how:

1. Fork the repository
2. Create feature branch: `git checkout -b feature-name`
3. Implement changes with clear comments
4. Test thoroughly
5. Commit: `git commit -m "Description"`
6. Push: `git push origin feature-name`
7. Submit Pull Request

### Areas for Improvement

- IPv6 support
- Database integration (PostgreSQL/MongoDB)
- Additional algorithm implementations
- Enhanced visualizations
- Mobile responsive design
- API authentication

---

## License

This project is open source and available under the MIT License.

---

## Acknowledgments

- Flask framework for web application
- Modern web standards (HTML5, CSS3, ES6+)
- Network security research community
- Data structures and algorithms best practices

---

## Documentation

**Complete technical documentation is available in three pages:**

1. **Dashboard** (`http://localhost:5000/`) - Main command center with 50 IP scenarios
2. **Documentation** (`http://localhost:5000/documentation`) - Technical guides and setup instructions
3. **References** (`http://localhost:5000/references`) - Academic sources and algorithm details

Or view the source files:
- `templates/index.html` - Dashboard
- `templates/documentation.html` - Technical documentation  
- `templates/references.html` - References

---

## Support

For issues, questions, or suggestions:
- Open an issue on GitHub
- Check the documentation page
- Review the code comments

---

**Last Updated:** February 2026
