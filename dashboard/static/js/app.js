// Dashboard JavaScript Application
let frequencyChart = null;
let memoryChart = null;
let updateInterval = null;

// Initialize dashboard
document.addEventListener('DOMContentLoaded', () => {
    setupNavigation();
    initializeCharts();
    startDataRefresh();
});

// Navigation between sections
function setupNavigation() {
    const navItems = document.querySelectorAll('.nav-item');
    navItems.forEach(item => {
        item.addEventListener('click', (e) => {
            e.preventDefault();
            const section = item.dataset.section;
            
            // Update active nav item
            navItems.forEach(nav => nav.classList.remove('active'));
            item.classList.add('active');
            
            // Show corresponding section
            document.querySelectorAll('.section').forEach(sec => {
                sec.classList.remove('active');
            });
            document.getElementById(section).classList.add('active');
        });
    });
}

// Initialize charts
function initializeCharts() {
    const chartOptions = {
        responsive: true,
        maintainAspectRatio: true,
        plugins: {
            legend: {
                labels: { color: '#e6e9ef' }
            }
        },
        scales: {
            y: {
                ticks: { color: '#8b92a7' },
                grid: { color: '#2d3548' }
            },
            x: {
                ticks: { color: '#8b92a7' },
                grid: { color: '#2d3548' }
            }
        }
    };

    // Frequency Chart
    const freqCtx = document.getElementById('frequencyChart').getContext('2d');
    frequencyChart = new Chart(freqCtx, {
        type: 'bar',
        data: {
            labels: [],
            datasets: [{
                label: 'Call Count',
                data: [],
                backgroundColor: 'rgba(74, 158, 255, 0.6)',
                borderColor: 'rgba(74, 158, 255, 1)',
                borderWidth: 1
            }]
        },
        options: chartOptions
    });

    // Memory Chart
    const memCtx = document.getElementById('memoryChart').getContext('2d');
    memoryChart = new Chart(memCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'CPU %',
                    data: [],
                    borderColor: 'rgba(74, 158, 255, 1)',
                    backgroundColor: 'rgba(74, 158, 255, 0.1)',
                    tension: 0.4
                },
                {
                    label: 'Memory (MB)',
                    data: [],
                    borderColor: 'rgba(179, 127, 235, 1)',
                    backgroundColor: 'rgba(179, 127, 235, 0.1)',
                    tension: 0.4
                }
            ]
        },
        options: chartOptions
    });
}

// Start periodic data refresh
function startDataRefresh() {
    fetchData();
    updateInterval = setInterval(fetchData, 1000);
}

// Fetch data from API
async function fetchData() {
    try {
        const [snapshot, status] = await Promise.all([
            fetch('/api/snapshot').then(r => r.json()),
            fetch('/api/status').then(r => r.json())
        ]);
        
        updateStatus(status, snapshot);
        updateDashboard(snapshot);
    } catch (error) {
        console.error('Error fetching data:', error);
        updateStatus({ snapshot_exists: false });
    }
}

// Update connection status
function updateStatus(status, snapshot = {}) {
    const statusDot = document.getElementById('statusDot');
    const statusText = document.getElementById('statusText');
    
    if (status.live) {
        statusDot.className = 'status-dot';
        statusText.textContent = 'Live';
    } else if (status.snapshot_exists || snapshot.status) {
        statusDot.className = 'status-dot demo';
        statusText.textContent = snapshot.status === 'demo' ? 'Demo Mode' : 'Offline';
    } else {
        statusDot.className = 'status-dot offline';
        statusText.textContent = 'No Data';
    }
}

// Update dashboard with snapshot data
function updateDashboard(data) {
    if (!data) return;
    
    // Update stats cards
    updateStatsCards(data);
    
    // Update process info
    updateProcessInfo(data);
    
    // Update system call stream
    updateSyscallStream(data);
    
    // Update frequency chart
    updateFrequencyChart(data);
    
    // Update memory chart
    updateMemoryChart(data);
    
    // Update categories
    updateCategories(data);
    
    // Update statistics
    updateStatistics(data);
    
    // Update logs
    updateLogs(data);
    
    // Update milestones
    updateMilestones(data);
    
    // Update process tree
    updateProcessTree(data);
}

// Update stats cards
function updateStatsCards(data) {
    document.getElementById('cpuUsage').textContent = 
        data.process ? `${data.process.cpu || 0}%` : '0%';
    
    document.getElementById('memUsage').textContent = 
        data.process ? `${data.process.mem_percent?.toFixed(1) || 0}%` : '0%';
    
    document.getElementById('activeProcesses').textContent = 
        data.threads ? data.threads.filter(t => t.alive).length : '0';
    
    document.getElementById('syscallRate').textContent = 
        (data.rate || 0).toFixed(1);
}

// Update process info
function updateProcessInfo(data) {
    if (data.process) {
        const p = data.process;
        document.getElementById('procPid').textContent = p.pid || '-';
        document.getElementById('procName').textContent = p.name || '-';
        document.getElementById('procState').textContent = p.state ? `${p.state} (${getStateDesc(p.state)})` : '-';
        document.getElementById('procCpu').textContent = p.cpu ? `${p.cpu}%` : '-';
        document.getElementById('procMem').textContent = p.rss_kb ? `${(p.rss_kb / 1024).toFixed(1)} MB` : '-';
        document.getElementById('procThreads').textContent = p.threads || '-';
        document.getElementById('procPpid').textContent = p.ppid || '-';
        document.getElementById('procCmd').textContent = p.cmdline || '-';
    } else {
        ['procPid', 'procName', 'procState', 'procCpu', 'procMem', 'procThreads', 'procPpid', 'procCmd']
            .forEach(id => document.getElementById(id).textContent = '-');
    }
}

// Get state description
function getStateDesc(state) {
    const states = {
        'R': 'Running',
        'S': 'Sleeping',
        'D': 'Disk Sleep',
        'Z': 'Zombie',
        'T': 'Stopped',
        't': 'Tracing'
    };
    return states[state] || 'Unknown';
}

// Update syscall stream
function updateSyscallStream(data) {
    if (!data.events || data.events.length === 0) return;
    
    const tbody = document.getElementById('syscallTableBody');
    const rows = data.events.slice(-10).reverse().map(event => {
        const statusClass = event.fail ? 'error' : 'success';
        const statusText = event.fail ? 'ERROR' : 'SUCCESS';
        const retVal = event.has_ret ? event.ret : '-';
        
        return `
            <tr>
                <td>${event.time}</td>
                <td>${event.pid}</td>
                <td>${event.name}</td>
                <td>${event.nr}</td>
                <td>${retVal}</td>
                <td><span class="badge ${statusClass}">${statusText}</span></td>
            </tr>
        `;
    }).join('');
    
    tbody.innerHTML = rows || '<tr><td colspan="6" class="no-data">No system calls yet</td></tr>';
}

// Update frequency chart
function updateFrequencyChart(data) {
    if (!data.counts || data.counts.length === 0) return;
    
    const top10 = data.counts.slice(0, 10);
    frequencyChart.data.labels = top10.map(c => c.name);
    frequencyChart.data.datasets[0].data = top10.map(c => c.count);
    
    // Color by category
    const categoryColors = {
        'file': 'rgba(74, 158, 255, 0.6)',
        'process': 'rgba(82, 196, 26, 0.6)',
        'memory': 'rgba(179, 127, 235, 0.6)',
        'io': 'rgba(250, 173, 20, 0.6)',
        'other': 'rgba(139, 146, 167, 0.6)'
    };
    
    frequencyChart.data.datasets[0].backgroundColor = top10.map(c => 
        categoryColors[c.category] || categoryColors.other
    );
    
    frequencyChart.update('none');
}

// Update memory chart
function updateMemoryChart(data) {
    if (!data.history || data.history.length === 0) return;
    
    const history = data.history.slice(-30);
    memoryChart.data.labels = history.map((h, i) => i);
    memoryChart.data.datasets[0].data = history.map(h => h.cpu);
    memoryChart.data.datasets[1].data = history.map(h => h.rss_kb / 1024);
    memoryChart.update('none');
}

// Update categories
function updateCategories(data) {
    if (!data.categories) return;
    
    document.getElementById('catFile').textContent = data.categories.file || 0;
    document.getElementById('catProcess').textContent = data.categories.process || 0;
    document.getElementById('catMemory').textContent = data.categories.memory || 0;
    document.getElementById('catIo').textContent = data.categories.io || 0;
    document.getElementById('catOther').textContent = data.categories.other || 0;
}

// Update statistics
function updateStatistics(data) {
    document.getElementById('statTotal').textContent = data.total || 0;
    document.getElementById('statUnique').textContent = data.unique || 0;
    document.getElementById('statTop').textContent = data.top ? `${data.top.name} (${data.top.count})` : '-';
    document.getElementById('statArch').textContent = data.arch || '-';
    
    // Update top syscalls table
    if (data.counts && data.counts.length > 0) {
        const total = data.total || 1;
        const tbody = document.getElementById('topSyscallsBody');
        tbody.innerHTML = data.counts.slice(0, 15).map(c => {
            const percent = ((c.count / total) * 100).toFixed(1);
            return `
                <tr>
                    <td>${c.name}</td>
                    <td>${c.nr}</td>
                    <td>${c.count}</td>
                    <td>${c.category}</td>
                    <td>${percent}%</td>
                </tr>
            `;
        }).join('');
    }
}

// Update logs
function updateLogs(data) {
    document.getElementById('logFile').textContent = data.log_file || '-';
    document.getElementById('logCommand').textContent = data.command || '-';
    
    if (data.started_ms) {
        const date = new Date(data.started_ms);
        document.getElementById('logStarted').textContent = date.toLocaleString();
    } else {
        document.getElementById('logStarted').textContent = '-';
    }
    
    document.getElementById('logStatus').textContent = data.status || '-';
}

// Update milestones
function updateMilestones(data) {
    const container = document.getElementById('milestones');
    
    if (!data.milestones || data.milestones.length === 0) {
        container.innerHTML = '<div class="no-data">No milestone events yet</div>';
        return;
    }
    
    container.innerHTML = data.milestones.slice(-10).reverse().map(m => `
        <div class="milestone-item">
            <div class="milestone-time">${m.time}</div>
            <div class="milestone-syscall">${m.name}(${m.detail}) = ${m.has_ret ? m.ret : '?'}</div>
        </div>
    `).join('');
}

// Update process tree
function updateProcessTree(data) {
    const container = document.getElementById('processTree');
    
    if (!data.threads || data.threads.length === 0) {
        container.innerHTML = '<div class="no-data">No process data available</div>';
        return;
    }
    
    // Build simple tree
    let tree = '<div class="tree-node">Tracer Process</div>';
    tree += '<div class="tree-node">├── Root Process';
    if (data.root_pid) tree += ` (PID: ${data.root_pid})`;
    tree += '</div>';
    
    data.threads.forEach((t, i) => {
        if (t.alive && t.pid !== data.root_pid) {
            const prefix = i === data.threads.length - 1 ? '└──' : '├──';
            tree += `<div class="tree-node">    ${prefix} ${t.name || 'Thread'} (PID: ${t.pid})</div>`;
        }
    });
    
    container.innerHTML = tree;
}

// Cleanup on page unload
window.addEventListener('beforeunload', () => {
    if (updateInterval) {
        clearInterval(updateInterval);
    }
});
