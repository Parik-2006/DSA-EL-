from flask import Flask, jsonify, render_template, request
import json
import os
import logging

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


# -------------------- BITWISE CIDR TRIGGER (NEW) --------------------

@app.route("/trigger_binary", methods=["POST"])
def trigger_binary():
    data = request.get_json(force=True)
    user_ip = data.get("ip", "0.0.0.0")

    try:
        # Writes the IP to a file that the C backend reads for the Bitwise Check
        with open("cmd_binary_check.txt", "w") as f:
            f.write(user_ip)
            f.flush()
            os.fsync(f.fileno())
    except Exception as e:
        return jsonify({"error": str(e)}), 500

    return jsonify({"status": "sent"})


# -------------------- LOGS & RULES --------------------

@app.route("/logs")
def logs_old():
    try:
        if os.path.exists("logs.json"):
            with open("logs.json", "r") as f:
                return jsonify(json.load(f))
        return jsonify([])
    except:
        return jsonify([])


@app.route("/logs_defense")
def logs_new():
    try:
        if os.path.exists("logs_defense.json"):
            with open("logs_defense.json", "r") as f:
                return jsonify(json.load(f))
        return jsonify([])
    except:
        return jsonify([])


@app.route("/trie_rules")
def trie_rules():
    try:
        if os.path.exists("trie_view.json"):
            with open("trie_view.json", "r") as f:
                return jsonify(json.load(f))
        return jsonify([])
    except:
        return jsonify([])


# -------------------- SYSTEM RESET --------------------

@app.route("/trigger_reset", methods=["POST"])
def trigger_reset():
    try:
        # Create the file that tells C backend to wipe memory
        with open("cmd_reset.txt", "w") as f:
            f.write("RESET")
            f.flush()
            os.fsync(f.fileno())
    except Exception as e:
        return jsonify({"error": str(e)}), 500
    
    return jsonify({"status": "reset"})


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
    # Note: On Render, gunicorn handles the execution, not app.run()
    app.run(host="0.0.0.0", port=port)