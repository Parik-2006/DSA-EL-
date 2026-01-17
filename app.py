from flask import Flask, jsonify, render_template, request
import json
import os
import logging

log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

app = Flask(__name__)
app.config['SEND_FILE_MAX_AGE_DEFAULT'] = 0

@app.route('/')
def home(): return render_template('index.html')

@app.route('/attack')
def attack(): return render_template('dashboard.html')

@app.route('/defense')
def defense(): return render_template('defense.html')

# --- LEGACY TRIGGER ---
@app.route('/trigger_legacy', methods=['POST'])
def trigger_legacy():
    data = request.json
    action = data.get('action')
    user_ip = data.get('ip', '0.0.0.0') # Default if empty
    
    filename = "cmd_legacy_normal.txt" if action == 'normal' else "cmd_legacy_attack.txt"
    try:
        with open(filename, "w") as f: f.write(user_ip)
    except: pass
    return jsonify({"status": "sent"})

# --- DEFENSE TRIGGER ---
@app.route('/trigger_defense', methods=['POST'])
def trigger_defense():
    data = request.json
    action = data.get('action')
    user_ip = data.get('ip', '0.0.0.0')

    filename = "cmd_defense_normal.txt" if action == 'normal' else "cmd_defense_attack.txt"
    try:
        with open(filename, "w") as f: f.write(user_ip)
    except: pass
    return jsonify({"status": "sent"})

# --- LOGS & RULES ---
@app.route('/logs')
def logs_old():
    try: return jsonify(json.load(open("logs.json")))
    except: return jsonify([])

@app.route('/logs_defense')
def logs_new():
    try: return jsonify(json.load(open("logs_defense.json")))
    except: return jsonify([])

@app.route('/trie_rules')
def trie_rules():
    try: return jsonify(json.load(open("trie_view.json")))
    except: return jsonify([])

if __name__ == '__main__':
    print("Dashboard: http://127.0.0.1:5000")
    app.run(port=5000)