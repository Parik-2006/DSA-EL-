from flask import Flask, jsonify, render_template, request
import json
import os
import logging
import subprocess

# Silence Flask default logs
log = logging.getLogger("werkzeug")
log.setLevel(logging.ERROR)

app = Flask(__name__)
app.config["SEND_FILE_MAX_AGE_DEFAULT"] = 0


# -------------------- ROUTES --------------------

@app.route("/")
def home():
    return render_template("index.html")


@app.route("/attack")
def attack():
    return render_template("dashboard.html")


@app.route("/defense")
def defense():
    return render_template("defense.html")


# -------------------- LEGACY TRIGGER --------------------

@app.route("/trigger_legacy", methods=["POST"])
def trigger_legacy():
    data = request.get_json(force=True)
    action = data.get("action", "normal")
    user_ip = data.get("ip", "0.0.0.0")

    filename = "cmd_legacy_normal.txt" if action == "normal" else "cmd_legacy_attack.txt"

    try:
        with open(filename, "w") as f:
            f.write(user_ip)
            f.flush()
            os.fsync(f.fileno())
    except Exception as e:
        return jsonify({"error": str(e)}), 500

    return jsonify({"status": "sent"})


# -------------------- DEFENSE TRIGGER --------------------

@app.route("/trigger_defense", methods=["POST"])
def trigger_defense():
    data = request.get_json(force=True)
    action = data.get("action", "normal")
    user_ip = data.get("ip", "0.0.0.0")

    filename = "cmd_defense_normal.txt" if action == "normal" else "cmd_defense_attack.txt"

    try:
        with open(filename, "w") as f:
            f.write(user_ip)
            f.flush()
            os.fsync(f.fileno())
    except Exception as e:
        return jsonify({"error": str(e)}), 500

    return jsonify({"status": "sent"})


# -------------------- C PROGRAM EXECUTION --------------------

@app.route("/run_c")
def run_c_program():
    try:
        result = subprocess.run(
            ["./backend"],
            capture_output=True,
            text=True,
            timeout=5
        )
        return jsonify({
            "output": result.stdout,
            "error": result.stderr
        })
    except Exception as e:
        return jsonify({"error": str(e)}), 500


# -------------------- LOGS & RULES --------------------

@app.route("/logs")
def logs_old():
    try:
        with open("logs.json") as f:
            return jsonify(json.load(f))
    except:
        return jsonify([])


@app.route("/logs_defense")
def logs_new():
    try:
        with open("logs_defense.json") as f:
            return jsonify(json.load(f))
    except:
        return jsonify([])


@app.route("/trie_rules")
def trie_rules():
    try:
        with open("trie_view.json") as f:
            return jsonify(json.load(f))
    except:
        return jsonify([])


# -------------------- NO-CACHE FIX --------------------

@app.after_request
def add_no_cache_headers(response):
    response.headers["Cache-Control"] = "no-store, no-cache, must-revalidate, max-age=0"
    response.headers["Pragma"] = "no-cache"
    response.headers["Expires"] = "0"
    return response


# -------------------- SERVER START --------------------

if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5000))
    print(f"Server running on port {port}")
    app.run(host="0.0.0.0", port=port)
