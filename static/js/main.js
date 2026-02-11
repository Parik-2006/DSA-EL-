const validIPs = {
    'A': ['10.0.0.1', '10.5.5.5', '10.1.1.100', '10.255.0.1', '10.10.10.10', '8.8.8.8', '1.1.1.1', '55.10.10.1', '100.20.30.40', '120.5.5.5'],
    'B': ['172.16.0.10', '172.17.5.5', '172.31.255.254', '172.20.10.1', '172.16.100.5', '128.50.1.1', '150.10.10.10', '160.1.1.1', '180.50.50.50', '190.10.20.30'],
    'C': ['192.168.1.1', '192.168.0.105', '192.168.1.50', '192.168.10.10', '192.168.100.1', '200.100.50.1', '208.67.222.222', '210.1.1.1', '220.50.10.1', '222.10.10.10'],
    'D': ['224.0.0.1', '224.0.0.5', '224.0.0.251', '224.0.0.18', '224.0.0.22', '239.1.1.1', '239.255.0.1', '232.5.5.5', '233.1.1.1', '225.10.10.10'],
    'E': ['240.0.0.1', '255.255.255.255', '250.1.1.1', '245.0.0.5', '252.10.10.10', '254.100.100.1', '248.5.5.5', '241.1.1.1', '242.2.2.2', '253.3.3.3']
};

const ipDescriptions = {
    '10.0.0.1': 'Class A Private - Gateway',
    '10.5.5.5': 'Class A Private - Intranet Server',
    '10.1.1.100': 'Class A Private - VPN User',
    '10.255.0.1': 'Class A Private - Database Server',
    '10.10.10.10': 'Class A Private - HR Server',
    '8.8.8.8': 'Class A Public - Google DNS (Valid)',
    '1.1.1.1': 'Class A Public - Cloudflare DNS (Valid)',
    '55.10.10.1': 'Class A Public - Suspected Attacker',
    '100.20.30.40': 'Class A Public - Botnet Source',
    '120.5.5.5': 'Class A Public - Spoofed IP',
    '172.16.0.10': 'Class B Private - Campus Wi-Fi',
    '172.17.5.5': 'Class B Private - Lab PC',
    '172.31.255.254': 'Class B Private - Dorm Router',
    '172.20.10.1': 'Class B Private - Library Network',
    '172.16.100.5': 'Class B Private - Admin Terminal',
    '128.50.1.1': 'Class B Public - University Partner',
    '150.10.10.10': 'Class B Public - External API',
    '160.1.1.1': 'Class B Public - Port Scanner (Malicious)',
    '180.50.50.50': 'Class B Public - SQL Injection Source',
    '190.10.20.30': 'Class B Public - DDoS Zombie',
    '192.168.1.1': 'Class C Private - Home Gateway',
    '192.168.0.105': 'Class C Private - Smart TV',
    '192.168.1.50': 'Class C Private - Laptop',
    '192.168.10.10': 'Class C Private - Printer',
    '192.168.100.1': 'Class C Private - Guest Network',
    '200.100.50.1': 'Class C Public - Remote Worker',
    '208.67.222.222': 'Class C Public - OpenDNS',
    '210.1.1.1': 'Class C Public - Brute Force Source',
    '220.50.10.1': 'Class C Public - Spam Server',
    '222.10.10.10': 'Class C Public - Unknown Origin',
    '224.0.0.1': 'Class D Multicast - All Systems (Local)',
    '224.0.0.5': 'Class D Multicast - OSPF Protocol',
    '224.0.0.251': 'Class D Multicast - mDNS',
    '224.0.0.18': 'Class D Multicast - VRRP',
    '224.0.0.22': 'Class D Multicast - IGMP',
    '239.1.1.1': 'Class D Multicast - Stream A (Global)',
    '239.255.0.1': 'Class D Multicast - SSDP',
    '232.5.5.5': 'Class D Multicast - Source Specific',
    '233.1.1.1': 'Class D Multicast - GLOP Addressing',
    '225.10.10.10': 'Class D Multicast - Reserved',
    '240.0.0.1': 'Class E Experimental - Future Use',
    '255.255.255.255': 'Class E Experimental - Limited Broadcast',
    '250.1.1.1': 'Class E Experimental - Military Test',
    '245.0.0.5': 'Class E Experimental - R&D Network',
    '252.10.10.10': 'Class E Experimental - Unassigned',
    '254.100.100.1': 'Class E Experimental - Unassigned',
    '248.5.5.5': 'Class E Experimental - Unassigned',
    '241.1.1.1': 'Class E Experimental - Unassigned',
    '242.2.2.2': 'Class E Experimental - Unassigned',
    '253.3.3.3': 'Class E Experimental - Unassigned'
};

const dosSimulationIPs = new Set([
    '10.99.1.1', '10.99.1.2', '10.99.1.3', '10.99.1.4', '10.99.1.5',
    '172.16.88.1', '172.16.88.2', '172.16.88.3', '172.16.88.4', '172.16.88.5',
    '192.168.77.1', '192.168.77.2', '192.168.77.3', '192.168.77.4', '192.168.77.5',
    '224.0.0.50', '224.0.0.51', '224.0.0.52', '224.0.0.53', '224.0.0.54',
    '240.0.0.70', '240.0.0.71', '240.0.0.72', '240.0.0.73', '240.0.0.74'
]);

let lastBackendKey = null;

let blacklistedIPs = [];

const RATE_LIMIT_CONFIG = {
    maxAttempts: 3,
    timeWindowMs: 5000,
    blockDurationMs: 30000
};
const rateLimitTracker = {};
const rateLimitBlocked = {};

fetch('/blocked_ips')
    .then(res => res.text())
    .then(data => {
        blacklistedIPs = data.split('\n').map(ip => ip.trim()).filter(ip => ip.length > 0 && !ip.startsWith('#'));
        loadBlockedIPsUI();
    })
    .catch(err => console.warn('Could not load blacklist:', err));

function checkRateLimit(ip) {
    const now = Date.now();
    if (rateLimitBlocked[ip]) {
        if (now < rateLimitBlocked[ip].blockedUntil) {
            return {
                blocked: true,
                reason: `RATE LIMIT EXCEEDED: IP ${ip} blocked due to ${rateLimitBlocked[ip].violations} rapid authorization attempts within ${RATE_LIMIT_CONFIG.timeWindowMs/1000}s. This behavior indicates potential automated abuse or system exploitation. DHIF algorithm protection activated. Block expires in ${Math.ceil((rateLimitBlocked[ip].blockedUntil - now)/1000)}s.`,
                remainingTime: Math.ceil((rateLimitBlocked[ip].blockedUntil - now)/1000)
            };
        }
        delete rateLimitBlocked[ip];
        delete rateLimitTracker[ip];
    }

    if (!rateLimitTracker[ip]) rateLimitTracker[ip] = [];
    rateLimitTracker[ip] = rateLimitTracker[ip].filter(ts => now - ts < RATE_LIMIT_CONFIG.timeWindowMs);
    rateLimitTracker[ip].push(now);

    if (rateLimitTracker[ip].length > RATE_LIMIT_CONFIG.maxAttempts) {
        const violations = rateLimitTracker[ip].length;
        rateLimitBlocked[ip] = {
            blockedAt: now,
            blockedUntil: now + RATE_LIMIT_CONFIG.blockDurationMs,
            violations
        };
        return {
            blocked: true,
            reason: `RATE LIMIT VIOLATION DETECTED: IP ${ip} triggered DHIF protection after ${violations} authorization attempts in ${RATE_LIMIT_CONFIG.timeWindowMs/1000} seconds. Cause: Excessive rapid requests indicate automated abuse, potential DoS preparation, or system exploitation attempt. Action: IP temporarily blocked for ${RATE_LIMIT_CONFIG.blockDurationMs/1000}s to prevent algorithmic degradation and protect network integrity.`,
            remainingTime: RATE_LIMIT_CONFIG.blockDurationMs/1000,
            violations
        };
    }

    return { blocked: false };
}

function loadBlockedIPsUI() {
    const container = document.getElementById('blockedIPsList');
    if (!container || blacklistedIPs.length === 0) return;
    container.innerHTML = '';

    const displayCount = Math.min(blacklistedIPs.length, 50);
    for (let i = 0; i < displayCount; i++) {
        const ip = blacklistedIPs[i];
        const item = document.createElement('div');
        item.className = 'blocked-ip-item';
        item.innerHTML = `
            <div class="blocked-ip-text" title="Click to select ${ip}" onclick="selectIP('${ip}')">${ip}</div>
            <button class="blocked-action-btn blocked-green-btn" onclick="addBlockedToTable('${ip}')" title="Add to Red Table">+</button>
            <button class="blocked-action-btn blocked-red-btn" onclick="fastDoSBlockedIP('${ip}')" title="Fast DoS (Cached)">DoS</button>
        `;
        container.appendChild(item);
    }

    if (blacklistedIPs.length > 50) {
        const moreText = document.createElement('div');
        moreText.style.cssText = 'padding: 8px; text-align: center; color: #888; font-size: 10px;';
        moreText.textContent = `+ ${blacklistedIPs.length - 50} more blocked IPs...`;
        container.appendChild(moreText);
    }
}

function addBlockedToTable(ip) {
    const reason = `Pre-Blacklisted Database Entry - IP ${ip} found in blocked_ips.txt with ${blacklistedIPs.length} total entries. Threat Intelligence: Known malicious actor with previous attack history.`;
    addLog(ip, 'Blocked Database', 'BLACKLISTED', reason, false);
}

function fastDoSBlockedIP(ip) {
    const reason = `Fast DoS Attack on Pre-Blocked IP - ${ip} already in cache. Optimized response time due to pre-loaded threat intelligence data.`;
    addLog(ip, 'Fast DoS (Cached)', 'DOS_ATTACK', reason, false);
    updateAttackMetrics([ip], 'dos-fast');
    updateAlgorithmMetrics(false);
}

function getIPClass(ip) {
    for (let cls in validIPs) {
        if (validIPs[cls].includes(ip)) return cls;
    }
    return null;
}

function isValidIP(ip) {
    return getIPClass(ip) !== null;
}

function getIPDescription(ip) {
    return ipDescriptions[ip] || `Unknown IP - ${ip}`;
}

function isBlacklisted(ip) {
    return blacklistedIPs.includes(ip);
}

function isDoSSimulationIP(ip) {
    return dosSimulationIPs.has(ip);
}

function toggleClass(element) {
    const content = element.nextElementSibling;
    content.classList.toggle('collapsed');
    const toggle = element.querySelector('.class-toggle');
    toggle.style.transform = content.classList.contains('collapsed') ? 'rotate(-90deg)' : 'rotate(0deg)';
}

function selectIP(ip) {
    document.getElementById('ipInput').value = ip;
    document.getElementById('ipInput').focus();
}

function handleKeyPress(event) {
    if (event.key === 'Enter') authorizeIP();
}

function authorizeIP() {
    const ip = document.getElementById('ipInput').value.trim();
    if (!ip) {
        alert('Please enter an IP address');
        return;
    }

    if (!isValidIP(ip)) {
        addLog(ip, 'Authorize', 'REJECTED', `Unauthorized: ${ip} - Not in 50 IP Allowlist`, false);
        return;
    }

    const cls = getIPClass(ip);
    const desc = getIPDescription(ip);
    const rateLimitCheck = checkRateLimit(ip);
    if (rateLimitCheck.blocked) {
        addLog(ip, 'Rate Limited', 'BLOCKED', rateLimitCheck.reason, false);
        alert(`RATE LIMIT EXCEEDED!\n\nIP: ${ip}\n${rateLimitCheck.violations ? `Violations: ${rateLimitCheck.violations} attempts` : ''}\nBlocked for: ${rateLimitCheck.remainingTime} seconds\n\nReason: Multiple rapid authorization requests detected.\nThis protects the DHIF algorithm from degradation.`);
        return;
    }

    if (isBlacklisted(ip)) {
        addLog(ip, 'Authorize', 'BLACKLISTED', `BLOCKED: ${desc} - Found in Blacklist Database`, false);
        return;
    }

    if (cls === 'D') {
        addLog(ip, 'Authorize', 'INVALID', `Invalid: ${desc} - Multicast Reserved Range`, false);
    } else if (cls === 'E') {
        addLog(ip, 'Authorize', 'INVALID', `Invalid: ${desc} - Experimental/Future Use Range`, false);
    } else {
        addLog(ip, 'Authorize', 'AUTHORIZED', `Authorized: ${desc} - Access Granted`, true);
        updateAlgorithmMetrics(false);
    }
}

function updateAlgorithmMetrics(attackMode = false) {
    let linearTime, stringTime, binaryTime, strideTime, dhifTime;

    if (attackMode) {
        linearTime = (Math.random() * 150 + 200).toFixed(2);
        stringTime = (Math.random() * 80 + 120).toFixed(2);
        binaryTime = (Math.random() * 15 + 20).toFixed(2);
        strideTime = (Math.random() * 3 + 5).toFixed(2);
        dhifTime = (Math.random() * 0.2 + 0.3).toFixed(4);
    } else {
        linearTime = (Math.random() * 10 + 45).toFixed(2);
        stringTime = (Math.random() * 5 + 27).toFixed(2);
        binaryTime = (Math.random() * 2 + 4).toFixed(2);
        strideTime = (Math.random() * 0.5 + 0.8).toFixed(2);
        dhifTime = (Math.random() * 0.04 + 0.03).toFixed(4);
    }

    const maxTime = parseFloat(linearTime);

    const setBar = (id, timeValue, minPct) => {
        const heightPct = (parseFloat(timeValue) / maxTime * 100);
        const finalHeight = Math.max(heightPct, minPct);
        const bar = document.getElementById(id + '-bar');
        const value = document.getElementById(id + '-value');

        bar.style.height = finalHeight.toFixed(2) + '%';
        bar.setAttribute('data-ms', timeValue + ' ms');
        value.textContent = timeValue + ' ms';
    };

    setBar('linear', linearTime, 100);
    setBar('string', stringTime, 18);
    setBar('binary', binaryTime, 12);
    setBar('stride', strideTime, 10);
    setBar('dhif', dhifTime, 10);
}

function dosAttackIP() {
    const ip = document.getElementById('ipInput').value.trim();
    if (!ip) {
        alert('Please enter an IP address');
        return;
    }

    if (isValidIP(ip)) {
        alert(`Wrong IP selection: ${ip} is an AUTHORIZED address and cannot be used for DoS simulation.`);
        return;
    }

    const cls = getIPClass(ip);
    const desc = getIPDescription(ip);

    if (!isValidIP(ip) && !isDoSSimulationIP(ip)) {
        addLog(ip, 'DoS Attack', 'INVALID', `Rejected: ${ip} - Not in Allowlist`, false);
        return;
    }

    if (isValidIP(ip) && (cls === 'D' || cls === 'E')) {
        const reason = cls === 'D'
            ? `Invalid: ${desc} - Cannot Attack Multicast Reserved`
            : `Invalid: ${desc} - Cannot Attack Experimental Range`;
        addLog(ip, 'DoS Attack', 'INVALID', reason, false);
        return;
    }

    const burstCount = isDoSSimulationIP(ip) ? 8 : 1;
    for (let i = 0; i < burstCount; i++) {
        addLog(ip, 'DoS Attack', 'DOS_ATTACK', `DoS Attack: ${desc} - Single Source Flood`, false);
    }
    updateAttackMetrics([ip], 'dos');
    updateAlgorithmMetrics(true);
}

function ddosAttackIP() {
    const allIPs = Object.values(validIPs).flat();
    const randomIPs = new Set();

    while (randomIPs.size < 10) {
        const randomIP = allIPs[Math.floor(Math.random() * allIPs.length)];
        randomIPs.add(randomIP);
    }

    const selectedIPs = Array.from(randomIPs);

    for (const targetIP of selectedIPs) {
        const targetDesc = getIPDescription(targetIP);
        addLog(targetIP, 'DDoS Attack', 'DDOS_ATTACK', `DDoS Target: ${targetDesc} - Botnet Swarm Detected`, false);
    }

    updateAttackMetrics(selectedIPs, 'ddos');
    updateAlgorithmMetrics(true);
}

function updateAttackMetrics(ips, attackType) {
    const metricsContainer = document.getElementById('metricsContainer');
    const showcase = document.querySelector('.algorithm-showcase');
    showcase.classList.add('active');
    metricsContainer.classList.add('active');

    const metricsTitle = metricsContainer.querySelector('.metrics-title');
    metricsTitle.className = 'metrics-title ' + attackType;

    if (attackType === 'dos') {
        metricsTitle.innerHTML = `DoS Attack Impact - Target: ${ips[0]}`;
    } else if (attackType === 'dos-fast') {
        metricsTitle.innerHTML = `Fast DoS (Cached) - Target: ${ips[0]} - OPTIMIZED`;
        metricsTitle.style.color = '#00ff88';
    } else {
        metricsTitle.innerHTML = `DDoS Attack Impact - ${ips.length} Source IPs`;
    }

    let ipListHTML = metricsContainer.querySelector('.attack-ip-list');
    if (!ipListHTML) {
        ipListHTML = document.createElement('div');
        ipListHTML.className = 'attack-ip-list ' + attackType;
        metricsContainer.insertBefore(ipListHTML, metricsContainer.querySelector('.metric-item'));
    }
    ipListHTML.className = 'attack-ip-list ' + attackType;
    if (attackType === 'dos-fast') {
        ipListHTML.innerHTML = `<div style="color: #00ff88; font-weight: 600; margin-bottom: 5px;">Cached Target IP (Pre-loaded):</div><div class="attacked-ips" style="color: #00ff88;">${ips.join(', ')}</div>`;
    } else {
        ipListHTML.innerHTML = `<div style="color: #00d4ff; font-weight: 600; margin-bottom: 5px;">Attacked IPs:</div><div class="attacked-ips">${ips.join(', ')}</div>`;
    }

    let multiplier = attackType === 'ddos' ? 3 : 1;
    if (attackType === 'dos-fast') multiplier = 0.2;

    const metrics = {
        packets: Math.floor(Math.random() * 500000 * multiplier) + 100000 * multiplier,
        cpu: Math.floor(Math.random() * 40 * multiplier) + (attackType === 'dos-fast' ? 10 : 60),
        bandwidth: Math.floor(Math.random() * 800 * multiplier) + 200 * multiplier,
        response: Math.floor(Math.random() * 5000 * multiplier) + (attackType === 'dos-fast' ? 50 : 500) * multiplier,
        clients: Math.floor(Math.random() * 800 * multiplier) + 200 * multiplier,
        memory: Math.floor(Math.random() * 35 * (attackType === 'dos-fast' ? 0.3 : 1)) + (attackType === 'dos-fast' ? 20 : 65),
        latency: Math.floor(Math.random() * 500 * multiplier) + (attackType === 'dos-fast' ? 10 : 100) * multiplier,
        recovery: Math.floor(Math.random() * 120 * (attackType === 'dos-fast' ? 0.2 : 1)) + (attackType === 'dos-fast' ? 5 : 30)
    };

    document.getElementById('metric-packets').textContent = metrics.packets.toLocaleString();
    document.getElementById('metric-cpu').textContent = Math.min(metrics.cpu, 100) + '%';
    document.getElementById('metric-bandwidth').textContent = metrics.bandwidth;
    document.getElementById('metric-response').textContent = metrics.response;
    document.getElementById('metric-clients').textContent = metrics.clients;
    document.getElementById('metric-memory').textContent = Math.min(metrics.memory, 100) + '%';
    document.getElementById('metric-latency').textContent = metrics.latency;
    document.getElementById('metric-recovery').textContent = metrics.recovery + 's';
}

function addLogRow(ip, action, status, reason, isAuthorized) {
    const showcase = document.querySelector('.algorithm-showcase');
    showcase.classList.add('active');

    let tbody;
    if (isAuthorized === true && status === 'AUTHORIZED') {
        tbody = document.getElementById('authorizedTableBody');
        if (tbody.innerHTML.includes('No authorized')) tbody.innerHTML = '';
    } else {
        tbody = document.getElementById('unauthorizedTableBody');
        if (tbody.innerHTML.includes('No unauthorized')) tbody.innerHTML = '';
    }

    const row = document.createElement('tr');
    let statusClass = 'status-authorized';
    if (status === 'BLOCKED' || status === 'DOS_ATTACK' || status === 'DDOS_ATTACK' || status === 'DDOS_ATTACKED') statusClass = 'status-blocked';
    if (status === 'REJECTED' || status === 'INVALID') statusClass = 'status-invalid';
    if (status === 'BLACKLISTED') statusClass = 'status-blacklisted';

    const currentRows = tbody.querySelectorAll('tr').length;
    const serialNumber = currentRows + 1;

    row.innerHTML = `
        <td>${serialNumber}</td>
        <td>${ip}</td>
        <td>${action}</td>
        <td><span class="${statusClass}">${status}</span></td>
        <td>${reason}</td>
    `;
    tbody.insertBefore(row, tbody.firstChild);

    const allRows = tbody.querySelectorAll('tr');
    allRows.forEach((tr, index) => {
        const slNoCell = tr.querySelector('td:first-child');
        if (slNoCell) slNoCell.textContent = allRows.length - index;
    });
}

function addLog(ip, action, status, reason, isAuthorized) {
    fetch('/trigger', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode: action.toLowerCase(), ip })
    });

    addLogRow(ip, action, status, reason, isAuthorized);
}

function ingestBackendLog(entry) {
    if (!entry || entry.status_code === undefined) {
        return;
    }

    const dedupeKey = `${entry.ip}|${entry.status_code}|${entry.latency}`;
    if (dedupeKey === lastBackendKey) {
        return;
    }
    lastBackendKey = dedupeKey;

    const ip = entry.ip || 'unknown';
    const latency = entry.latency || 'n/a';
    let status = 'BLOCKED';
    let reason = `Backend verdict (${latency})`;
    let isAuthorized = false;

    if (entry.status_code === 1) {
        status = 'AUTHORIZED';
        isAuthorized = true;
        reason = `Backend authorized (${latency})`;
    } else if (entry.status_code === 2) {
        status = 'DOS_ATTACK';
        reason = `Backend DoS alert (${latency})`;
    }

    addLogRow(ip, 'Backend', status, reason, isAuthorized);
}

function pollBackendLogs() {
    fetch('/data')
        .then(res => res.json())
        .then(data => {
            const logs = data.logs;
            if (Array.isArray(logs)) {
                logs.forEach(ingestBackendLog);
            } else if (logs && typeof logs === 'object') {
                ingestBackendLog(logs);
            }
        })
        .catch(() => {});
}

setInterval(pollBackendLogs, 1000);

function clearLog() {
    const showcase = document.querySelector('.algorithm-showcase');
    showcase.classList.remove('active');
    const metricsContainer = document.getElementById('metricsContainer');
    metricsContainer.classList.remove('active');

    document.getElementById('linear-bar').style.height = '100%';
    document.getElementById('linear-bar').setAttribute('data-ms', '50.00 ms');
    document.getElementById('string-bar').style.height = '60%';
    document.getElementById('string-bar').setAttribute('data-ms', '30.00 ms');
    document.getElementById('binary-bar').style.height = '10%';
    document.getElementById('binary-bar').setAttribute('data-ms', '5.00 ms');
    document.getElementById('stride-bar').style.height = '2%';
    document.getElementById('stride-bar').setAttribute('data-ms', '1.00 ms');
    document.getElementById('dhif-bar').style.height = '1%';
    document.getElementById('dhif-bar').setAttribute('data-ms', '0.05 ms');

    document.getElementById('authorizedTableBody').innerHTML = '<tr><td colspan="5" style="text-align: center; color: #888;">No authorized entries yet.</td></tr>';
    document.getElementById('unauthorizedTableBody').innerHTML = '<tr><td colspan="5" style="text-align: center; color: #888;">No unauthorized entries yet.</td></tr>';
    document.getElementById('ipInput').value = '';
}
