from flask import Flask, jsonify, render_template, request
import json
import os

app = Flask(__name__)
app.config["SEND_FILE_MAX_AGE_DEFAULT"] = 0

@app.route("/")
def home():
    blocked_ips = []
    if os.path.exists("blocked_ips.txt"):
        with open("blocked_ips.txt", "r") as f:
            blocked_ips = [line.strip() for line in f.readlines() if line.strip()]
    return render_template("index.html", blocked_ips=blocked_ips)

@app.route("/trigger", methods=["POST"])
def trigger():
    data = request.get_json(force=True)
    with open("cmd_trigger.txt", "w") as f:
        f.write(f"{data['mode']} {data['ip']}")
        f.flush()
        os.fsync(f.fileno())
    return jsonify({"status": "sent"})

@app.route("/data")
def data():
    stats = {"array": 0, "string": 0, "binary": 0, "stride": 0}
    logs = []
    
    if os.path.exists("stats.json"):
        try:
            with open("stats.json", "r") as f: stats = json.load(f)
        except: pass

    if os.path.exists("simulation_logs.json"):
        try:
            with open("simulation_logs.json", "r") as f: 
                logs = json.load(f)
            os.remove("simulation_logs.json") # Delete after read
        except: pass
        
    return jsonify({"stats": stats, "logs": logs})

@app.after_request
def add_header(r):
    r.headers["Cache-Control"] = "no-cache, no-store, must-revalidate"
    return r

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)