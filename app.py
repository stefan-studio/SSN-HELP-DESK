import os
import subprocess
import json
import sys
import logging
from flask import Flask, jsonify, request, session, render_template, redirect, url_for

# Setup logging
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

app = Flask(__name__)
app.secret_key = "ssn_institutions_secure_portal_key"
app.config['SESSION_TYPE'] = 'filesystem'
app.config['JSONIFY_PRETTYPRINT_REGULAR'] = False

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
EXE_FILENAME = "helpdesk.exe" if sys.platform.startswith("win") else "helpdesk"
EXE_PATH = os.path.join(BASE_DIR, EXE_FILENAME) #

def call_c_engine(*args):
    """Bridge to the C-based Linked List engine."""
    cmd = [EXE_PATH] + [str(a) for a in args]
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=3)
        if not proc.stdout.strip():
            return {"success": False, "error": "C Engine empty response"}
        return json.loads(proc.stdout)
    except Exception as e:
        return {"success": False, "error": f"Bridge Error: {str(e)}"} #

# --- Page Routes ---
@app.route("/")
def index():
    return render_template("index.html") #

@app.route("/dashboard")
def dashboard():
    if "user_id" not in session:
        return redirect(url_for('index'))
    role = session.get("role")
    if role == "admin":
        return render_template("admin-dashboard.html") #
    if role == "engineer":
        return render_template("engineer-dashboard.html") #
    return render_template("user-dashboard.html") #

# --- API Routes (Matching your HTML Fetch calls) ---

@app.route("/api/login", methods=["POST"])
def login():
    data = request.json
    # Pass credentials to helpdesk.exe
    res = call_c_engine("login", data.get('username'), data.get('password'))
    if res.get("success"):
        session.update({
            "user_id": res["user_id"], 
            "role": res["role"],
            "full_name": data.get('username')
        })
        res["redirect"] = "/dashboard" #
    return jsonify(res)

@app.route("/api/create_ticket", methods=["POST"])
def create_ticket_api():
    logger.debug(f"Create ticket request. Session: {dict(session)}")
    logger.debug(f"Headers: {dict(request.headers)}")
    logger.debug(f"Method: {request.method}")
    
    if "user_id" not in session:
        logger.error("No user_id in session")
        return jsonify({"success": False, "error": "Unauthorized"}), 401
    
    try:
        data = request.get_json(force=True)
        logger.debug(f"Request data: {data}")
        
        # C Engine expects: create_ticket <uid> <category> <desc> <priority>
        res = call_c_engine(
            "create_ticket", 
            session.get("user_id"), 
            data.get('issue_type', 'Other'), 
            data.get('description', 'No description'), 
            1  # Default priority
        )
        logger.debug(f"C Engine response: {res}")
        
        # Format response to include full ticket object that the frontend expects
        if res.get("success") and "ticket" in res:
            return jsonify(res) # C engine now returns full ticket
        return jsonify(res)
    except Exception as e:
        logger.error(f"Error creating ticket: {str(e)}", exc_info=True)
        return jsonify({"success": False, "error": f"Server Error: {str(e)}"}), 500

@app.route("/api/tickets/<int:user_id>")
def get_user_tickets(user_id):
    # C Engine expects: list_tickets <uid_filter>
    res = call_c_engine("list_tickets", user_id)
    return jsonify(res)

@app.route("/api/engineer/tickets/<int:eng_id>")
def get_engineer_tickets(eng_id):
    # C Engine expects: list_tickets <uid_filter> <eng_filter>
    # We pass 0 for uid_filter to show all users, but filter by eng_id
    res = call_c_engine("list_tickets", 0, eng_id)
    return jsonify(res)

@app.route("/api/tickets")
def get_all_tickets():
    # Admin route: list all tickets
    if session.get("role") != "admin":
        return jsonify({"success": False, "error": "Admin access required"}), 403
    res = call_c_engine("list_tickets")
    return jsonify(res)

@app.route("/api/engineers")
def get_engineers():
    # Admin route: get all engineers
    if session.get("role") != "admin":
        return jsonify({"success": False, "error": "Admin access required"}), 403
    
    # The C engine currently returns hardcoded engineers.
    # In a real system, this would load from a database.
    res = call_c_engine("list_engineers") 
    
    # For demo, add dummy active_tickets and total_resolved
    if res.get("success") and "engineers" in res:
        for eng in res["engineers"]:
            # Simulate workload: count tickets assigned to this engineer
            all_tickets_res = call_c_engine("list_tickets", 0, eng["id"])
            if all_tickets_res.get("success"):
                eng["active_tickets"] = len([t for t in all_tickets_res["tickets"] if t["status"] not in ["Resolved", "Closed"]])
                eng["total_resolved"] = len([t for t in all_tickets_res["tickets"] if t["status"] == "Resolved"])
            else:
                eng["active_tickets"] = 0
                eng["total_resolved"] = 0
    return jsonify(res)


@app.route("/api/close_ticket", methods=["POST"])
def close_ticket():
    # Admin route: close a ticket
    if session.get("role") != "admin":
        return jsonify({"success": False, "error": "Admin access required"}), 403
    data = request.json #
    return jsonify(call_c_engine("close_ticket", data.get('ticket_id'))) #

@app.route("/api/assign_ticket", methods=["POST"])
def assign_ticket():
    # Admin route: assign ticket to engineer
    if session.get("role") != "admin":
        return jsonify({"success": False, "error": "Admin access required"}), 403
    data = request.json
    # C engine now returns the updated ticket object directly
    res = call_c_engine("assign_ticket", data.get('ticket_id'), data.get('engineer_id')) 
    return jsonify(res)

@app.route("/api/update_ticket_status", methods=["POST"])
def update_ticket_status():
    # Engineer route: update status of an assigned ticket
    if session.get("role") not in ["engineer", "admin"]:
        return jsonify({"success": False, "error": "Unauthorized"}), 403
    data = request.json
    ticket_id = data.get('ticket_id')
    new_status = data.get('status')
    
    res = call_c_engine("update_status", ticket_id, new_status)
    return jsonify(res)

@app.route("/api/logout", methods=["POST", "GET"])
def logout():
    session.clear()
    return jsonify({"success": True}) #

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True) #