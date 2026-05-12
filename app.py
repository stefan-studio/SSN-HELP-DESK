import json
import os
import subprocess
import sys
import secrets
import heapq
from datetime import datetime, timedelta
from flask_wtf.csrf import CSRFProtect
from PIL import Image
from functools import wraps
from werkzeug.security import generate_password_hash, check_password_hash
from flask import Flask, jsonify, redirect, render_template, request, session, url_for

app = Flask(__name__)
app.secret_key = "ssn-helpdesk-secret-2024"

csrf = CSRFProtect(app)
BASE_DIR    = os.path.dirname(os.path.abspath(__file__))
HELPDESK_EXE = os.path.join(BASE_DIR, "helpdesk")
if sys.platform.startswith("win"):
    HELPDESK_EXE += ".exe"

# --- Smart Analysis Knowledge Base ---
PROBLEM_DB = {
    "critical": {"score": 5, "note": "Urgent keyword detected"}, # P3
    "fire":     {"score": 10, "note": "Safety Hazard / Emergency"}, # P1
    "smoke":    {"score": 10, "note": "Safety Hazard / Emergency"}, # P1
    "leak":     {"score": 7,  "note": "Infrastructure Damage risk"}, # P2
    "exam":     {"score": 8,  "note": "Academic Impact"}, # P2
    "server":   {"score": 6,  "note": "Network wide disruption"}, # P3
    "wifi":     {"score": 4,  "note": "Connectivity issue"}, # P4
    "broken":   {"score": 2,  "note": "Furniture/Hardware repair"}, # P5
    "dead":     {"score": 5,  "note": "Total failure"}, # P3
    "smell":    {"score": 6,  "note": "Potential electrical hazard"} # P3
}

# --- Image Upload Settings ---
MAX_IMAGE_DIMENSION = 1024  # Max width or height for uploaded images
JPEG_QUALITY = 85           # JPEG compression quality (1-95)

def run_helpdesk(*args):
    if not os.path.isfile(HELPDESK_EXE):
        return {"success": False, "error": f"Compiled binary not found at '{HELPDESK_EXE}'. Run: gcc -o helpdesk helpdesk.c"}
    cmd = [HELPDESK_EXE] + [str(a) for a in args]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10, cwd=BASE_DIR)
    except Exception as exc:
        return {"success": False, "error": str(exc)}

    stdout = result.stdout.strip()
    if not stdout:
        return {"success": False, "error": result.stderr.strip() or "No output from C binary."}
    try:
        return json.loads(stdout)
    except json.JSONDecodeError:
        return {"success": False, "error": f"Invalid JSON from binary: {stdout[:200]}"}

def login_required(role=None):
    def decorator(fn):
        @wraps(fn)
        def wrapper(*args, **kwargs):
            if "user_id" not in session:
                return jsonify({"success": False, "error": "Not logged in"}), 401
            if role and session.get("role") != role:
                return jsonify({"success": False, "error": "Forbidden"}), 403
            return fn(*args, **kwargs)
        return wrapper
    return decorator

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/user-dashboard")
def user_dashboard():
    if "user_id" not in session or session.get("role") != "user": return redirect(url_for("index"))
    return render_template("user-dashboard.html")

@app.route("/admin-dashboard")
def admin_dashboard():
    if "user_id" not in session or session.get("role") != "admin": return redirect(url_for("index"))
    return render_template("admin-dashboard.html")

@app.route("/engineer-dashboard")
def engineer_dashboard():
    if "user_id" not in session or session.get("role") != "engineer": return redirect(url_for("index"))
    return render_template("engineer-dashboard.html")

def get_users():
    pf = os.path.join(BASE_DIR, "users.json")
    if os.path.exists(pf):
        try:
            with open(pf, "r") as f: return json.load(f)
        except: pass
    
    default_users = {
        "user": {"user_id": 1, "password": "pass", "full_name": "Normal User", "role": "user"},
        "admin": {"user_id": 2, "password": "pass", "full_name": "Administrator", "role": "admin"},
        "middleman": {"user_id": 3, "password": "pass", "full_name": "Ticket Middleman", "role": "middleman"},
        "ravi": {"user_id": 101, "password": "pass", "full_name": "Ravi Kumar", "role": "engineer"}
    }
    save_users(default_users)
    return default_users

def save_users(users_dict):
    pf = os.path.join(BASE_DIR, "users.json")
    with open(pf, "w") as f: json.dump(users_dict, f, indent=2)

@app.route("/api/signup", methods=["POST"])
def api_signup():
    data = request.get_json(silent=True) or {}
    username = str(data.get("username", "")).strip()
    password = str(data.get("password", "")).strip()
    full_name = str(data.get("full_name", "")).strip()
    role = str(data.get("role", "")).strip()
    
    if not username or not password or not full_name or not role:
        return jsonify({"success": False, "error": "All fields are required"}), 400
        
    if role not in ["user", "middleman", "admin", "engineer"]:
        return jsonify({"success": False, "error": "Invalid role specified"}), 400
        
    users = get_users()
    if username in users:
        return jsonify({"success": False, "error": "Username already exists"}), 400
        
    new_id = max([u["user_id"] for u in users.values()] + [0]) + 1
    
    users[username] = {
        "user_id": new_id,
        "password": generate_password_hash(password),
        "full_name": full_name,
        "role": role
    }
    save_users(users)
    
    session["user_id"] = new_id
    session["username"] = username
    session["full_name"] = full_name
    session["role"] = role
    
    redirect_map = {"user": "/user-dashboard", "admin": "/admin-dashboard", "middleman": "/middleman-dashboard", "engineer": "/engineer-dashboard"}
    
    return jsonify({
        "success": True,
        "redirect": redirect_map.get(role, "/"),
        "user_id": new_id,
        "username": username,
        "full_name": full_name,
        "role": role
    }), 200

@app.route("/api/login", methods=["POST"])
def api_login():
    data = request.get_json(silent=True) or {}
    username = str(data.get("username", "")).strip()
    password = str(data.get("password", "")).strip()
    role = str(data.get("role", "")).strip()
    
    if not username or not password or not role:
        return jsonify({"success": False, "error": "Credentials and role required"}), 400
        
    users = get_users()
    if username not in users or not check_password_hash(users[username]["password"], password):
        return jsonify({"success": False, "error": "Invalid credentials"}), 401
        
    if users[username]["role"] != role:
        return jsonify({"success": False, "error": f"Cannot login to {role.capitalize()} portal with {users[username]['role']} credentials"}), 403
        
    u = users[username]
    session["user_id"] = u["user_id"]
    session["username"] = username
    session["full_name"] = u["full_name"]
    session["role"] = u["role"]
    
    redirect_map = {"user": "/user-dashboard", "admin": "/admin-dashboard", "middleman": "/middleman-dashboard", "engineer": "/engineer-dashboard"}
    
    return jsonify({
        "success": True,
        "redirect": redirect_map.get(role, "/"),
        "user_id": u["user_id"],
        "username": username,
        "full_name": u["full_name"],
        "role": u["role"]
    }), 200

@app.route("/api/logout", methods=["POST"])
def api_logout():
    session.clear()
    return jsonify({"success": True})

def apply_priorities(result, role):
    if not result.get("success"): return result
    
    if role == "admin":
        # Admin view sorts by priority using a min-heap
        heap = []
        unassigned = []
        for t in result.get("tickets", []):
            if t["priority"] > 0:
                heapq.heappush(heap, (t["priority"], t["id"], t))
            else:
                unassigned.append(t)
        
        ordered_tickets = []
        while heap:
            ordered_tickets.append(heapq.heappop(heap)[2])
        ordered_tickets.extend(unassigned)
        result["tickets"] = ordered_tickets
    return result

@app.route("/api/tickets", methods=["GET"])
@login_required()
def api_get_all_tickets():
    role = session.get("role")
    result = run_helpdesk("list_tickets", 0)
    result = apply_priorities(result, role)
    return jsonify(result), 200 if result.get("success") else 500

@app.route("/api/analytics/resolutions")
@login_required(role="admin")
def api_resolutions_analytics():
    result = run_helpdesk("list_tickets", 0)
    if not result.get("success"):
        return jsonify(result), 500
    
    tickets = result.get("tickets", [])
    today = datetime.now().date()
    
    # Generate keys for the last 7 days
    last_7_days = [(today - timedelta(days=i)).strftime('%Y-%m-%d') for i in range(6, -1, -1)]
    counts = {day: 0 for day in last_7_days}
    
    for t in tickets:
        # Count tickets that are Resolved or Closed and have a valid timestamp
        if t.get("status") in ["Resolved", "Closed"] and t.get("closed_at") and t.get("closed_at") != "null":
            try:
                # Date format from C is "YYYY-MM-DDTHH:MM:SS"
                closed_date = t["closed_at"].split('T')[0]
                if closed_date in counts:
                    counts[closed_date] += 1
            except:
                continue
                
    return jsonify({
        "success": True,
        "labels": last_7_days,
        "data": [counts[day] for day in last_7_days]
    })

@app.route("/api/tickets/<int:user_id>", methods=["GET"])
@login_required()
def api_get_tickets(user_id):
    role = session.get("role")
    result = run_helpdesk("list_tickets", user_id)
    result = apply_priorities(result, role)
    return jsonify(result), 200 if result.get("success") else 500

@app.route("/api/search_bst", methods=["GET"])
@login_required()
def api_search_bst():
    ticket_id = request.args.get("ticket_id")
    if not ticket_id: return jsonify({"success": False, "error": "Missing ticket_id"}), 400
    result = run_helpdesk("search_bst", ticket_id)
    return jsonify(result), 200 if result.get("success") else 500

@app.route("/api/create_ticket", methods=["POST"])
@login_required()
def api_create_ticket():
    if session.get("role") == "middleman":
        return jsonify({"success": False, "error": "Middlemen are not authorized to raise tickets"}), 403

    if request.is_json:
        data = request.get_json()
        file = None
    else:
        data = request.form
        file = request.files.get('image')

    original_description = str(data.get("description", "")).strip()
    desc_lower = original_description.lower()
    issue_type = str(data.get("issue_type", "")).strip()
    
    if not issue_type or not original_description:
        return jsonify({"success": False, "error": "Missing fields"}), 400

    image_path = "null"
    if file and file.filename != '':
        try:
            # Ensure the uploads directory exists
            upload_dir = os.path.join(BASE_DIR, "static", "uploads")
            os.makedirs(upload_dir, exist_ok=True)

            # Generate a unique filename with entropy to avoid collisions
            # We use .jpg because we are forcing JPEG format below
            random_hex = secrets.token_hex(4)
            unique_filename = f"ticket_{int(datetime.now().timestamp())}_{random_hex}.jpg"
            full_save_path = os.path.join(upload_dir, unique_filename)

            # Open the image with Pillow
            img = Image.open(file)
            
            # Backward compatibility for Pillow Resampling attribute
            resample_filter = getattr(Image, 'Resampling', Image).LANCZOS
            img.thumbnail((MAX_IMAGE_DIMENSION, MAX_IMAGE_DIMENSION), resample_filter)

            # Ensure mode is RGB (handles RGBA, P, LA, CMYK) for JPEG compatibility
            if img.mode != 'RGB':
                img = img.convert('RGB')
                
            img.save(full_save_path, format='JPEG', quality=JPEG_QUALITY)
            
            image_path = f"static/uploads/{unique_filename}"

        except Exception as e:
            print(f"Error processing image: {e}")
            # Optionally, return an error to the user or log it more robustly
            # For now, we'll just proceed without an image path
    
    # 2. Smart Analysis Engine
    total_score = 0
    notes = []
    for word, impact in PROBLEM_DB.items():
        if word in desc_lower:
            total_score += impact["score"]
            if impact["note"] not in notes: notes.append(impact["note"])
    
    # Map score to P1-P5 (1 is high in our system)
    inferred_prio = 5
    if total_score >= 10: inferred_prio = 1
    elif total_score >= 7: inferred_prio = 2
    elif total_score >= 4: inferred_prio = 3
    
    brief_note = " | ".join(notes) if notes else "Routine check"
    
    # 3. Create the ticket via C with all inferred data and image path
    res = run_helpdesk("create_ticket", session["user_id"], issue_type, original_description, inferred_prio, brief_note, image_path)
    if not res.get("success"): return jsonify(res), 500
    ticket_id = res["ticket"]["id"]
    
    # 4. Auto-assign engineer and return the UPDATED ticket
    assign_res = run_helpdesk("auto_assign_ticket", ticket_id)
    return jsonify({"success": True, "ticket": assign_res.get("ticket", res["ticket"]), "inferred_note": brief_note}), 200

@app.route("/api/edit_ticket", methods=["POST"])
@login_required()
def api_edit_ticket():
    data = request.get_json(silent=True) or {}
    ticket_id = data.get("ticket_id")
    new_description = data.get("description")
    result = run_helpdesk("edit_ticket", session["user_id"], ticket_id, new_description)
    return jsonify(result), 200 if result.get("success") else 400

@app.route("/api/delete_ticket", methods=["POST"])
@login_required()
def api_delete_ticket():
    data = request.get_json(silent=True) or {}
    ticket_id = data.get("ticket_id")
    result = run_helpdesk("delete_ticket", session["user_id"], ticket_id)
    return jsonify(result), 200 if result.get("success") else 400

@app.route("/api/close_ticket", methods=["POST"])
@login_required()
def api_close_ticket():
    data = request.get_json(silent=True) or {}
    ticket_id = int(data.get("ticket_id", 0))
    result = run_helpdesk("close_ticket", ticket_id)
    return jsonify(result), 200 if result.get("success") else 400

@app.route("/api/assign_ticket", methods=["POST"])
@login_required()
def api_assign_ticket():
    # Shared by admin and middleman
    if session.get("role") not in ["admin", "middleman"]:
        return jsonify({"success": False, "error": "Forbidden"}), 403
    data = request.get_json(silent=True) or {}
    ticket_id = int(data.get("ticket_id", 0))
    engineer_id = data.get("engineer_id")
    args = ["assign_ticket", ticket_id]
    if engineer_id: args.append(engineer_id)
    result = run_helpdesk(*args)
    return jsonify(result), 200 if result.get("success") else 400

@app.route("/api/set_priority", methods=["POST"])
@login_required(role="middleman")
def api_set_priority():
    data = request.get_json(silent=True) or {}
    ticket_id = int(data.get("ticket_id", 0))
    priority = int(data.get("priority", 0))
    if not (1 <= priority <= 5):
        return jsonify({"success": False, "error": "Priority must be between 1 and 5"}), 400
    
    result = run_helpdesk("set_priority", ticket_id, priority)
    return jsonify(result), 200 if result.get("success") else 400

@app.route("/middleman-dashboard")
def middleman_dashboard():
    if "user_id" not in session or session.get("role") != "middleman": return redirect(url_for("index"))
    return render_template("middleman-dashboard.html")

@app.route("/api/engineers", methods=["GET"])
@login_required()
def api_get_engineers():
    result = run_helpdesk("list_engineers")
    return jsonify(result), 200 if result.get("success") else 500

if __name__ == "__main__":
    app.run(debug=True, port=5000)