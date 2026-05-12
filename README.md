# SSN Help Desk System

A hybrid Help Desk Management System featuring a high-performance **C Backend** for data structures and a **Python/Flask Frontend** for the web interface.

## Architecture
- **Backend (C)**: Utilizes a Binary Search Tree (BST) for $O(\log n)$ ticket lookups and a Min-Heap for priority-based sorting.
- **Frontend (Python/Flask)**: Handles user authentication, session management, and a "Smart Analysis" engine that automatically categorizes ticket urgency.

## Features
- **Smart Dispatcher**: Automatically identifies high-priority keywords (fire, server, leak) and assigns tickets to specialized engineers.
- **Binary Search Tree Explorer**: Backend-driven ticket retrieval.
- **Image Support**: Users can upload images of issues, which are compressed via Pillow to save server space.
- **Security**: Implements CSRF protection via Flask-WTF and password hashing with Werkzeug.

## Prerequisites
- **C Compiler**: `gcc` (MinGW for Windows).
- **Python**: 3.8 or higher.
- **Dependencies**:
  ```bash
  pip install -r requirements.txt
  ```

## Setup and Execution

1. **Compile the C Backend**:
   ```bash
   gcc -o helpdesk helpdesk.c
   ```

2. **Initialize Uploads Directory**:
   ```bash
   mkdir static/uploads
   ```

3. **Run the Flask App**:
   ```bash
   python app.py
   ```
   Access the dashboard at `http://127.0.0.1:5000`.