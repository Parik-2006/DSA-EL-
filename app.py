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
            blocked_ips = [line.strip() for line in f.readlines() if line.strip() and not line.startswith("#")]
    return render_template("index.html", blocked_ips=blocked_ips)

@app.route("/documentation")
def documentation():
    return render_template("documentation.html")

@app.route("/references")
def references():
    return render_template("references.html")

@app.route("/trigger", methods=["POST"])
def trigger():
    data = request.get_json(force=True)
    with open("cmd_trigger.txt", "w") as f:
        f.write(f"{data['mode']} {data['ip']}")
    return jsonify({"status": "sent"})

@app.route("/blocked_ips")
def blocked_ips_endpoint():
    if os.path.exists("blocked_ips.txt"):
        with open("blocked_ips.txt", "r") as f:
            return f.read(), 200, {'Content-Type': 'text/plain'}
    return "", 200, {'Content-Type': 'text/plain'}

@app.route("/data")
def data():
    stats = {"array": 0, "string": 0, "binary": 0, "stride": 0}
    logs = []
    try:
        if os.path.exists("stats.json"):
            with open("stats.json") as f: stats = json.load(f)
        if os.path.exists("simulation_logs.json"):
            with open("simulation_logs.json") as f: logs = json.load(f)
            os.remove("simulation_logs.json")
    except: pass
    return jsonify({"stats": stats, "logs": logs})

@app.after_request
def add_header(r):
    r.headers["Cache-Control"] = "no-cache, no-store, must-revalidate"
    return r

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)