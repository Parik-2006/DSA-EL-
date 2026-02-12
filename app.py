from flask import Flask, jsonify, redirect, render_template, request
import json
import os

app = Flask(__name__)
app.config["SEND_FILE_MAX_AGE_DEFAULT"] = 3600

BLOCKED_IPS_PATH = "blocked_ips.txt"
_blocked_ips_cache = {"mtime": None, "data": []}


def _is_valid_ipv4(ip):
    if not isinstance(ip, str):
        return False
    parts = ip.strip().split(".")
    if len(parts) != 4:
        return False
    for part in parts:
        if not part.isdigit():
            return False
        value = int(part)
        if value < 0 or value > 255:
            return False
    return True


def _load_blocked_ips():
    try:
        mtime = os.path.getmtime(BLOCKED_IPS_PATH)
    except OSError:
        _blocked_ips_cache["mtime"] = None
        _blocked_ips_cache["data"] = []
        return []

    if _blocked_ips_cache["mtime"] != mtime:
        with open(BLOCKED_IPS_PATH, "r") as f:
            _blocked_ips_cache["data"] = [
                line.strip()
                for line in f.readlines()
                if line.strip() and not line.startswith("#")
            ]
        _blocked_ips_cache["mtime"] = mtime

    return _blocked_ips_cache["data"]


@app.route("/")
def home():
    blocked_ips = _load_blocked_ips()
    return render_template("index.html", blocked_ips=blocked_ips)

@app.route("/documentation")
def documentation():
    return render_template("documentation.html")

@app.route("/references")
def references():
    return render_template("references.html")

@app.route("/comparision")
def comparision():
    return render_template("comparision.html")

@app.route("/comaprision")
def comaprision_redirect():
    return redirect("/comparision", code=301)

@app.route("/hashtable")
def hashtable():
    return render_template("hashtable.html")

@app.route("/hash_table")
def hash_table_redirect():
    return redirect("/hashtable", code=301)

@app.route("/trigger", methods=["POST"])
def trigger():
    data = request.get_json(force=True)
    ip = data.get("ip")
    if not _is_valid_ipv4(ip):
        return jsonify({"status": "error", "message": "Invalid IPv4 address"}), 400
    with open("cmd_trigger.txt", "w") as f:
        f.write(f"{data['mode']} {ip}")
    return jsonify({"status": "sent"})

@app.route("/blocked_ips")
def blocked_ips_endpoint():
    blocked_ips = _load_blocked_ips()
    return "\n".join(blocked_ips), 200, {'Content-Type': 'text/plain'}

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
    if request.path.startswith("/static/"):
        r.headers["Cache-Control"] = "public, max-age=604800, immutable"
    else:
        r.headers["Cache-Control"] = "no-cache, no-store, must-revalidate"
    return r

if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5000))
    app.run(host="0.0.0.0", port=port)