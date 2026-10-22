#!/usr/bin/env python3
"""
Flask Dashboard for Linux System Call Tracer
Educational visualization of system calls and process monitoring
"""
from flask import Flask, render_template, jsonify, send_from_directory
import json
import os
import time
from datetime import datetime

app = Flask(__name__)
app.config['JSON_SORT_KEYS'] = False

# Path to the JSON snapshot file
SNAPSHOT_PATH = os.path.join(os.path.dirname(__file__), '..', 'logs', 'latest.json')

@app.route('/')
def index():
    """Main dashboard page"""
    return render_template('index.html')

@app.route('/api/snapshot')
def get_snapshot():
    """Get the latest tracer snapshot"""
    try:
        if os.path.exists(SNAPSHOT_PATH):
            with open(SNAPSHOT_PATH, 'r') as f:
                data = json.load(f)
                return jsonify(data)
        else:
            # Return demo data if no real trace is running
            return jsonify(get_demo_data())
    except Exception as e:
        return jsonify({'error': str(e), 'demo': True}), 500

@app.route('/api/status')
def get_status():
    """Get dashboard status"""
    snapshot_exists = os.path.exists(SNAPSHOT_PATH)
    if snapshot_exists:
        mtime = os.path.getmtime(SNAPSHOT_PATH)
        age = time.time() - mtime
        return jsonify({
            'snapshot_exists': True,
            'last_update': mtime,
            'age_seconds': age,
            'live': age < 5.0  # Consider live if updated within 5 seconds
        })
    return jsonify({
        'snapshot_exists': False,
        'demo_mode': True
    })

def get_demo_data():
    """Generate demo data for when no tracer is running"""
    return {
        'schema': 1,
        'status': 'demo',
        'updated_ms': int(time.time() * 1000),
        'started_ms': int((time.time() - 120) * 1000),
        'arch': 'x86_64',
        'tracer_pid': 0,
        'root_pid': 0,
        'command': 'demo_program',
        'log_file': 'logs/demo.log',
        'exit_code': 0,
        'exit_signal': 0,
        'total': 847,
        'unique': 24,
        'rate': 42.3,
        'top': {'name': 'read', 'count': 234},
        'categories': {
            'file': 412,
            'process': 89,
            'memory': 156,
            'io': 178,
            'other': 12
        },
        'counts': [
            {'name': 'read', 'nr': 0, 'count': 234, 'category': 'io'},
            {'name': 'write', 'nr': 1, 'count': 198, 'category': 'io'},
            {'name': 'openat', 'nr': 257, 'count': 167, 'category': 'file'},
            {'name': 'close', 'nr': 3, 'count': 143, 'category': 'file'},
            {'name': 'mmap', 'nr': 9, 'count': 89, 'category': 'memory'},
            {'name': 'brk', 'nr': 12, 'count': 45, 'category': 'memory'}
        ],
        'events': [],
        'milestones': [],
        'process': None,
        'history': [],
        'threads': []
    }

@app.route('/static/<path:filename>')
def static_files(filename):
    """Serve static files"""
    return send_from_directory('static', filename)

if __name__ == '__main__':
    print("=" * 60)
    print("  Linux System Call Tracer - Dashboard")
    print("=" * 60)
    print()
    print("  Dashboard URL: http://127.0.0.1:5000")
    print()
    print("  To trace a program, run in another terminal:")
    print("    cd ~/syscall-tracer")
    print("    ./build/syscall-tracer --run ./build/test_program")
    print()
    print("  Press Ctrl+C to stop the dashboard")
    print("=" * 60)
    print()
    
    app.run(host='127.0.0.1', port=5000, debug=True, use_reloader=False)
