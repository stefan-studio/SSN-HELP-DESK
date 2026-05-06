import os
import subprocess
import json
import sys
from flask import Flask, jsonify, request, session, render_template, redirect, url_for

app = Flask(__name__)
# Professional practice: Using a secure key (can be any string for your demo)
app.secret_key = "ssn_institutions_secure_portal_key"

# Detect OS to point to the correct C executable
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
EXE_FILENAME = "helpdesk.exe" if sys.platform.startswith("win") else "helpdesk"
EXE_PATH = os.path.join(BASE_DIR, EXE_FILENAME)

def call_c_engine(*args):
    """Bridge function to communicate with the C-based Linked List engine."""
    cmd = [EXE_PATH] + [str(a) for a in args]
    try:
        # We use a timeout to prevent the app from hanging if the C code loops
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=3)
        if not proc.stdout.strip():
            return {"success": False, "error": "C Engine returned empty response"}
        return json.loads(proc.stdout)
    except json.JSONDecodeError:
        return {"success": False, "error": "C Engine output is not valid JSON"}
    except Exception as e:
        return {"success": False, "error": f"Bridge Error: {str(e)}"}

# --- Page Routes ---

@app.route("/")
def index():
    """Main Landing/Login Page for SSN."""
    return render_template("index.html")

@app.route("/dashboard")
def dashboard():
    """Route for raising new tickets."""
    if "user_id" not in session:
        return redirect(url_for('index'))
    return render_template("index.html", view="dashboard") # Or a separate dashboard.html

@app.route("/history")
def history():
    """Route for viewing the Priority Queue (Linked List)."""
    if "user_id" not in session:
        return redirect(url_for('index'))
    return render_template("history.html")

# --- API Routes ---

@app.route("/api/login", methods=["POST"])
def login():
    data = request.json
    # Pass credentials to C logic for verification
    res = call_c_engine("login", data.get('username'), data.get('password'))
    
    if res.get("success"):
        session.update({
            "user_id": res["user_id"], 
            "role": res["role"],
            "username": data.get('username')
        })
        # Institutional logic: Admin/Staff goes to full queue, Students to history
        res["redirect"] = "/history"
    return jsonify(res)

@app.route("/api/create_ticket", methods=["POST"])
def create():
    if "user_id" not in session:
        return jsonify({"success": False, "error": "Session expired"}), 401
    
    data = request.json
    # Ensure priority is passed as an integer for the C engine's sorting logic
    res = call_c_engine(
        "create_ticket", 
        session.get("user_id"), 
        data.get('category', 'Other'), 
        data.get('description', 'No description'), 
        int(data.get('priority', 1))
    )
    return jsonify(res)

@app.route("/api/tickets")
def get_tickets():
    if "user_id" not in session:
        return jsonify({"success": False, "error": "Unauthorized"}), 401
    
    # Logic: Admin sees ALL (uid=0), User sees only their OWN tickets
    uid_filter = 0 if session.get("role") == "admin" else session.get("user_id")
    res = call_c_engine("list_tickets", uid_filter)
    return jsonify(res)

@app.route("/api/logout")
def logout():
    session.clear()
    return redirect(url_for('index'))

if __name__ == "__main__":
    # Running on 0.0.0.0 allows access from other devices on the same SSN/SNU network
    app.run(host="0.0.0.0", port=5000, debug=True)