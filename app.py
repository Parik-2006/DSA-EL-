from flask import Flask, jsonify, render_template, request
import json
import os
import logging

# MUTE THE CONSOLE
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

app = Flask(__name__)
app.config['SEND_FILE_MAX_AGE_DEFAULT'] = 0

@app.route('/')
def index():
    return render_template('dashboard.html')

@app.route('/trigger', methods=['POST'])
def trigger():
    action = request.json.get('action')
    filename = "cmd_normal.txt" if action == 'normal' else "cmd_attack.txt"
    try:
        with open(filename, "w") as f: f.write("1")
    except:
        pass
    return jsonify({"status": "sent"})

@app.route('/logs')
def get_logs():
    try:
        if os.path.exists("logs.json"):
            with open("logs.json", "r") as f:
                return jsonify(json.load(f))
    except:
        pass
    return jsonify([])

if __name__ == '__main__':
    print("Dashboard Ready: http://127.0.0.1:5000")
    app.run(debug=True, port=5000)