SSN HelpDesk: Hybrid Systems Implementation

A high-performance ticketing management system designed for Shiv Nadar - SSN Institutions. This project demonstrates a Hybrid Architecture, utilizing C for low-level memory management and data structures, and Python (Flask) for a modern, responsive web interface.
🚀 The Core Innovation: Priority Queueing

Unlike standard helpdesks that use simple FIFO (First-In-First-Out) logic, this system implements a Priority Queue using a Sorted Linked List in C. This ensures that critical infrastructure issues (like network outages) are automatically escalated to the top of the queue.
🛠️ Tech Stack

    Backend Engine: C (C11) – Handles logic, sorting, and binary file persistence.

    Middleware: Python 3.12 + Flask – Serves as the API bridge and orchestrator.

    Frontend: HTML5, CSS3 (Bootstrap 5.3), JavaScript (ES6).

    Database: Binary File I/O (.db files) – Fast, low-overhead data storage.

🏗️ System Architecture

The application utilizes a Subprocess Bridge to communicate between the high-level web server and the low-level logic engine:

    Frontend: Collects user data and sends a JSON request via fetch().

    Flask (Python): Validates the session and invokes the compiled C binary.

    C Engine: Executes the requested operation (Create, List, Sort) and outputs a JSON string to STDOUT.

    Flask: Captures the output and relays the JSON back to the user.

📋 Features

    Role-Based Access Control: Distinct dashboards for Students (Users) and Service Engineers (Admins).

    Persistent Storage: Data is saved in a binary format to ensure it survives server restarts.

    Dynamic Sorting: High-priority tickets (Level 5) are visually and logically prioritized over low-priority ones.

    Responsive UI: Designed specifically with the SSN branding and professional DM Sans typography.

⚙️ Setup & Installation
1. Prerequisites

    GCC Compiler (MinGW for Windows)

    Python 3.10+

    VS Code (Recommended)

2. Compilation (The C Engine)

First, compile the core logic engine into an executable:
PowerShell

gcc helpdesk.c -o helpdesk.exe

3. Environment Setup

Create a virtual environment and install dependencies:
PowerShell

python -m venv .venv
.\.venv\Scripts\activate
pip install flask

4. Run the Application
PowerShell

python app.py

Access the portal at http://127.0.0.1:5000
🧠 Data Structure Details


    Communication Protocol: Standardized JSON exchange between C and Python.
