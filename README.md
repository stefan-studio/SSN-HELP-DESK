# SSN HelpDesk — Full-Stack Integration

```
Frontend (HTML/JS)  →  Flask (Python API)  →  C Executable (business logic)
```

## Folder Structure

```
helpdesk/
├── helpdesk.c              ← C backend (modified for CLI + JSON output)
├── helpdesk.h              ← Structs & enums (unchanged API)
├── helpdesk  (compiled)    ← Generated after compilation
├── app.py                  ← Flask bridge server
├── requirements.txt
├── README.md
└── templates/
    ├── index.html           ← Login page
    ├── user-dashboard.html  ← User portal
    └── admin-dashboard.html ← Admin panel
```

Data files created at runtime (same folder as binary):
```
tickets.db          ← persisted ticket records
engineers.db        ← persisted engineer records
ticket_counter.dat  ← auto-increment ID counter
```

---

## Quick Start

### 1 — Compile the C backend

**Linux / macOS**
```bash
cd helpdesk/
gcc -o helpdesk helpdesk.c -lm
```

**Windows (MinGW / Git Bash)**
```bash
gcc -o helpdesk.exe helpdesk.c
```

### 2 — Install Python dependencies
```bash
pip install -r requirements.txt
```

### 3 — Run Flask
```bash
python app.py
```

Open your browser at **http://127.0.0.1:5000**

---

## Demo Credentials

| Role  | Username | Password  |
|-------|----------|-----------|
| User  | john     | 1234      |
| User  | jane     | 5678      |
| Admin | admin    | admin123  |

---

## REST API Reference

| Method | Endpoint                    | Auth   | Description                         |
|--------|-----------------------------|--------|-------------------------------------|
| GET    | `/`                         | —      | Login page                          |
| GET    | `/user-dashboard`           | user   | User dashboard page                 |
| GET    | `/admin-dashboard`          | admin  | Admin dashboard page                |
| GET    | `/logout`                   | —      | Clear session and redirect to login |
| POST   | `/api/login`                | —      | Authenticate; returns JWT-like info |
| POST   | `/api/logout`               | any    | Clear session                       |
| GET    | `/api/me`                   | any    | Current session info                |
| GET    | `/api/tickets`              | any    | All tickets (admin view)            |
| GET    | `/api/tickets/<user_id>`    | any    | Tickets for a specific user         |
| POST   | `/api/create_ticket`        | any    | Create a new ticket                 |
| POST   | `/api/close_ticket`         | any    | Close a ticket by ID                |
| POST   | `/api/assign_ticket`        | admin  | Re-assign a ticket (auto-balancer)  |
| GET    | `/api/engineers`            | any    | Engineer workload list              |
| GET    | `/api/health`               | —      | Server health / binary check        |

### POST /api/login
```json
// Request
{ "username": "john", "password": "1234" }

// Response (success)
{ "success": true, "user_id": 1, "username": "john",
  "full_name": "John Doe", "role": "user",
  "redirect": "/user-dashboard" }

// Response (failure)
{ "success": false, "error": "Invalid credentials" }
```

### POST /api/create_ticket
```json
// Request
{ "issue_type": "WiFi", "description": "No WiFi in Lab 3" }

// Response
{ "success": true, "ticket": {
    "id": 7, "user_id": 1, "engineer_id": 101,
    "issue_type": "WiFi", "status": "Assigned",
    "description": "No WiFi in Lab 3",
    "created_at": "2025-06-04T10:32:11",
    "assigned_at": "2025-06-04T10:32:11",
    "closed_at": null } }
```

### POST /api/close_ticket
```json
// Request
{ "ticket_id": 7 }

// Response
{ "success": true, "message": "Ticket #7 closed", "ticket": { ... } }
```

---

## C CLI Commands (called internally by Flask)

You can also run the C binary directly for testing:

```bash
# Login
./helpdesk login john 1234

# Create ticket (user_id=1, type=WiFi, description follows)
./helpdesk create_ticket 1 WiFi "WiFi keeps dropping in Block B"

# List tickets for user 1
./helpdesk list_tickets 1

# List ALL tickets (user_id=0)
./helpdesk list_tickets 0

# Close ticket #3
./helpdesk close_ticket 3

# Reassign ticket #3
./helpdesk assign_ticket 3

# List all engineers
./helpdesk list_engineers

# Interactive text menu
./helpdesk
```

---

## Architecture Notes

- **No external C libraries** — JSON is hand-serialised with proper escaping.
- **Data persistence** — `tickets.db` and `engineers.db` are raw binary files; engineers are seeded automatically on first run.
- **Session management** — Flask server-side sessions (cookie-signed). `sessionStorage` on the client mirrors the user info for UI display only.
- **Auto-assignment** — The C binary assigns the least-loaded engineer matching the issue specialty; falls back to any engineer if no specialist exists.
- **Error handling** — Every API call has try/catch; the Flask layer catches subprocess failures and returns structured JSON errors.
