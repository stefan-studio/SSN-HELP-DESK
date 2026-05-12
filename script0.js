
  // ── Session guard ────────────────────────────────────────────────
  if (sessionStorage.getItem('role') !== 'admin') {
    window.location.href = '/';
  }

  const adminName = sessionStorage.getItem('full_name') || 'Administrator';
  document.getElementById('admin-display-name').textContent = adminName;
  document.getElementById('admin-avatar-initials').textContent =
          adminName.split(' ').map(w => w[0]).join('').toUpperCase().slice(0, 2);

  // Populate Profile
  document.getElementById('prof-fullname').textContent = adminName;
  document.getElementById('prof-username').textContent = sessionStorage.getItem('username');
  document.getElementById('prof-role').textContent = 'Super Admin';

  // ── Data stores ──────────────────────────────────────────────────
  let allTickets  = [];
  let allEngineers = [];
  const csrfToken = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content');

  // ── Helpers ──────────────────────────────────────────────────────
  const statusClass = { 'Open':'s-open','Assigned':'s-assigned','In Progress':'s-inprogress','Resolved':'s-resolved','Closed':'s-closed' };
  function statusBadge(s) { return `<span class="badge-status ${statusClass[s]||'s-open'}">${s}</span>`; }
  function fmtDate(iso)   { return iso ? iso.split('T')[0] : '—'; }
  function getEngName(id) { const e = allEngineers.find(e => e.id === id); return e ? e.name : null; }

  // ── Fetch all tickets ────────────────────────────────────────────
  async function loadAllTickets() {
    document.getElementById('all-tickets-loading').style.display = 'block';
    document.getElementById('all-tickets-wrap').style.display    = 'none';
    try {
      const res  = await fetch('/api/tickets');
      const data = await res.json();
      if (data.success) {
        allTickets = data.tickets || [];
        renderAllTickets();
        updateMetrics();
        renderBreakdown();
      } else {
        showToast(data.error || 'Failed to load tickets.', 'danger');
      }
    } catch (err) {
      showToast('Network error loading tickets. Check console.', 'danger');
      console.error(err);
    } finally {
      document.getElementById('all-tickets-loading').style.display = 'none';
      document.getElementById('all-tickets-wrap').style.display    = '';
    }
  }

  // ── Fetch engineers ───────────────────────────────────────────────
  async function loadEngineers() {
    document.getElementById('engineers-loading').style.display = 'block';
    document.getElementById('engineers-wrap').style.display    = 'none';
    try {
      const res  = await fetch('/api/engineers');
      const data = await res.json();
      if (data.success) {
        allEngineers = data.engineers || [];
        renderEngineers();
        document.getElementById('m-engineers').textContent = allEngineers.length;
      } else {
        showToast(data.error || 'Failed to load engineers.', 'danger');
      }
    } catch (err) {
      showToast('Network error loading engineers. Check console.', 'danger');
      console.error(err);
    } finally {
      document.getElementById('engineers-loading').style.display = 'none';
      document.getElementById('engineers-wrap').style.display    = '';
    }
  }

  async function refreshAll() {
    await Promise.all([loadAllTickets(), loadEngineers()]);
    showToast('Data refreshed.', 'success');
  }

  // ── Metrics ───────────────────────────────────────────────────────
  function updateMetrics() {
    const active   = allTickets.filter(t => !['Resolved','Closed'].includes(t.status)).length;
    const resolved = allTickets.filter(t =>  ['Resolved','Closed'].includes(t.status)).length;
    document.getElementById('m-total').textContent    = allTickets.length;
    document.getElementById('m-active').textContent   = active;
    document.getElementById('m-resolved').textContent = resolved;
    // engineers count updated separately
  }

  // ── Breakdown bars ────────────────────────────────────────────────
  function renderBreakdown() {
    if (!allTickets.length) return;

    const catCounts = {};
    allTickets.forEach(t => { catCounts[t.issue_type] = (catCounts[t.issue_type]||0)+1; });
    const catColors = { WiFi:'#1e4db7', Network:'#163580', Hardware:'#e8a020', Software:'#7b1fa2', Furniture:'#2e7d32', Other:'#0097a7' };
    document.getElementById('category-breakdown').innerHTML = Object.entries(catCounts).map(([k,v]) => `
      <div style="margin-bottom:.6rem">
        <div style="display:flex;justify-content:space-between;font-size:.82rem;margin-bottom:4px">
          <span style="font-weight:600">${k}</span><span style="color:var(--muted)">${v}</span>
        </div>
        <div style="height:7px;background:#eef0f6;border-radius:9px;overflow:hidden">
          <div style="height:100%;width:${Math.round(v/allTickets.length*100)}%;background:${catColors[k]||'#888'};border-radius:9px"></div>
        </div>
      </div>`).join('');

    const stCounts = {};
    allTickets.forEach(t => { stCounts[t.status] = (stCounts[t.status]||0)+1; });
    const stColors = { Open:'#e8a020', Assigned:'#1e4db7', 'In Progress':'#7b1fa2', Resolved:'#2e7d32', Closed:'#6b7a99' };
    document.getElementById('status-breakdown').innerHTML = Object.entries(stCounts).map(([k,v]) => `
      <div style="margin-bottom:.6rem">
        <div style="display:flex;justify-content:space-between;font-size:.82rem;margin-bottom:4px">
          <span style="font-weight:600">${k}</span><span style="color:var(--muted)">${v}</span>
        </div>
        <div style="height:7px;background:#eef0f6;border-radius:9px;overflow:hidden">
          <div style="height:100%;width:${Math.round(v/allTickets.length*100)}%;background:${stColors[k]||'#888'};border-radius:9px"></div>
        </div>
      </div>`).join('');
  }

    // ── Render all-tickets table ──────────────────────────────────────
    function applyFilters() { renderAllTickets(); }

    function renderAllTickets() {
      const fStatus = document.getElementById('filter-status').value;
      const fType   = document.getElementById('filter-type').value;
      const fSearch = document.getElementById('filter-search').value.toLowerCase();

      const filtered = allTickets.filter(t => {
        if (fStatus && t.status     !== fStatus) return false;
        if (fType   && t.issue_type !== fType)   return false;
        if (fSearch && !String(t.id).includes(fSearch)
                && !t.description.toLowerCase().includes(fSearch)
                && !String(t.engineer_id||'').includes(fSearch)) return false;
        return true;
      });

      const tbody = document.getElementById('all-tickets-tbody');
      if (!filtered.length) {
        tbody.innerHTML = `<tr><td colspan="9"><div class="empty-state"><i class="bi bi-search"></i>No tickets match the filters.</div></td></tr>`;
        return;
      }
      tbody.innerHTML = filtered.map(t => {
        const engName = getEngName(t.engineer_id);
        const engCell = t.engineer_id
                ? `<span class="eng-id">ENG-${t.engineer_id}</span>${engName ? `<div style="font-size:.73rem;color:var(--muted)">${engName}</div>` : ''}`
                : `<span style="color:var(--muted);font-size:.8rem;font-style:italic">Unassigned</span>`;
        const isClosed = t.status === 'Closed';
        const isDone   = ['Resolved','Closed'].includes(t.status);
        const prioBadge = t.priority && t.priority > 0 
                ? `<span style="font-weight:700;color:var(--blue);font-size:.85rem">P${t.priority}</span>` 
                : `<span style="font-style:italic;color:var(--muted);font-size:.8rem">Unassigned</span>`;
        return `
        <tr id="row-${t.id}">
          <td><span class="ticket-id">#${t.id}</span></td>
          <td style="font-size:.82rem;color:var(--muted)">${t.user_id}</td>
          <td><span style="font-size:.82rem">${t.issue_type}</span></td>
          <td>${statusBadge(t.status)}</td>
          <td>${prioBadge}</td>
          <td>${engCell}</td>
          <td style="max-width:200px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;font-size:.82rem" title="${t.description}">${t.description}</td>
          <td style="color:var(--muted);font-size:.8rem;white-space:nowrap">${fmtDate(t.created_at)}</td>
          <td>
            <div style="display:flex;gap:6px;flex-wrap:nowrap">
              <button class="btn-sm-assign" onclick="doAssign(${t.id})" ${isDone?'disabled':''}>
                <i class="bi bi-person-plus"></i> Assign
              </button>
              <button class="btn-sm-close" onclick="doClose(${t.id})" ${isClosed?'disabled':''}>
                <i class="bi bi-x-circle"></i> Close
              </button>
            </div>
          </td>
        </tr>`;
      }).join('');
    }

    // ── Close ticket via API ──────────────────────────────────────────
    async function doClose(ticketId) {
      try {
        const res  = await fetch('/api/close_ticket', {
          method:  'POST',
          headers: { 
            'Content-Type': 'application/json',
            'X-CSRFToken': csrfToken
          },
          body:    JSON.stringify({ ticket_id: ticketId })
        });
        const data = await res.json();
        if (data.success) {
          const t = allTickets.find(x => x.id === ticketId);
          if (t) { t.status = 'Closed'; t.closed_at = data.ticket?.closed_at || null; }
          renderAllTickets();
          updateMetrics();
          renderBreakdown();
          showToast(`Ticket #${ticketId} closed.`, 'success');
        } else {
          showToast(data.error || 'Failed to close ticket.', 'danger');
        }
      } catch (err) {
        showToast('Network error.', 'danger');
      }
    }

    // ── Assign ticket via API ─────────────────────────────────────────
    async function doAssign(ticketId) {
      try {
        const res  = await fetch('/api/assign_ticket', {
          method:  'POST',
          headers: { 
            'Content-Type': 'application/json',
            'X-CSRFToken': csrfToken
          },
          body:    JSON.stringify({ ticket_id: ticketId })
        });
        const data = await res.json();
        if (data.success) {
          const updated = data.ticket;
          const idx = allTickets.findIndex(x => x.id === ticketId);
          if (idx >= 0) allTickets[idx] = { ...allTickets[idx], ...updated };
          // Reload engineers too (their loads changed)
          await loadEngineers();
          renderAllTickets();
          updateMetrics();
          showToast(`Ticket #${ticketId} reassigned to engineer ${updated.engineer_id}.`, 'success');
        } else {
          showToast(data.error || 'Failed to assign ticket.', 'danger');
        }
      } catch (err) {
        showToast('Network error.', 'danger');
      }
    }

    // ── Render engineers table ────────────────────────────────────────
    const OVERLOAD = 5;

    function renderEngineers() {
      document.getElementById('eng-count-badge').textContent = `${allEngineers.length} engineers`;
      const tbody = document.getElementById('engineers-tbody');
      if (!allEngineers.length) {
        tbody.innerHTML = `<tr><td colspan="6"><div class="empty-state"><i class="bi bi-people"></i>No engineers found.</div></td></tr>`;
        return;
      }
      tbody.innerHTML = allEngineers.map(e => {
        const overloaded = e.active_tickets >= OVERLOAD;
        const pct        = Math.min(Math.round(e.active_tickets / OVERLOAD * 100), 100);
        const barColor   = overloaded ? '#dc3545' : pct > 60 ? '#e8a020' : '#2e7d32';
        const initials   = e.name.split(' ').map(w => w[0]).join('').toUpperCase().slice(0, 2);
        return `
        <tr>
          <td><span class="eng-id">ENG-${e.id}</span></td>
          <td>
            <div style="display:flex;align-items:center;gap:.5rem">
              <div style="width:30px;height:30px;border-radius:50%;background:linear-gradient(135deg,var(--blue),#4a90e2);display:grid;place-items:center;color:#fff;font-weight:700;font-size:.72rem;flex-shrink:0">${initials}</div>
              <span style="font-weight:500">${e.name}</span>
            </div>
          </td>
          <td><span style="font-size:.82rem;color:var(--muted)">${e.specialty}</span></td>
          <td>
            <span style="font-weight:700;font-size:.95rem;${overloaded?'color:#dc3545':''}">
              ${e.active_tickets}${overloaded ? ' <i class="bi bi-exclamation-triangle-fill" style="font-size:.8rem"></i>' : ''}
            </span>
          </td>
          <td style="min-width:120px">
            <div class="load-bar-wrap">
              <div class="load-bar"><div class="load-bar-fill" style="width:${pct}%;background:${barColor}"></div></div>
              <span style="font-size:.75rem;color:var(--muted);white-space:nowrap">${pct}%</span>
            </div>
          </td>
          <td><span style="font-weight:600;color:var(--navy-mid)">${e.total_resolved}</span></td>
        </tr>`;
      }).join('');
    }

    // ── Section navigation ────────────────────────────────────────────
    const sectionMap = { overview:'section-overview', 'all-tickets':'section-all-tickets', engineers:'section-engineers', profile:'section-profile' };
    const titleMap   = { overview:'Overview', 'all-tickets':'All Tickets', engineers:'Engineer Workloads', profile:'Admin Profile' };

    function showSection(key) {
      Object.keys(sectionMap).forEach(k => {
        document.getElementById(sectionMap[k]).style.display = (k === key) ? '' : 'none';
      });

      // Dynamically match the onclick attribute so the active state follows the click
      document.querySelectorAll('.nav-link-item').forEach((el) => {
        el.classList.remove('active');
        if (el.getAttribute('onclick') === `showSection('${key}')`) {
          el.classList.add('active');
        }
      });

      document.getElementById('topbar-title').textContent = titleMap[key];
      if (key === 'all-tickets') loadAllTickets();
      if (key === 'engineers')   loadEngineers();
      closeSidebar();
    }

    // ── Toast ─────────────────────────────────────────────────────────
    function showToast(msg, type) {
      const el = document.getElementById('app-toast');
      document.getElementById('toast-msg').textContent = msg;
      el.className = `toast align-items-center border-0 text-bg-${type === 'success' ? 'success' : 'danger'}`;
      new bootstrap.Toast(el, { delay: 3500 }).show();
    }

    // ── Logout ────────────────────────────────────────────────────────
    async function doLogout(e) {
      e.preventDefault();
      await fetch('/api/logout', { method:'POST' });
      sessionStorage.clear();
      window.location.href = '/';
    }

    // ── Sidebar mobile ────────────────────────────────────────────────
    function openSidebar()  { document.getElementById('sidebar').classList.add('open');    document.getElementById('sidebar-overlay').classList.add('show');    }
    function closeSidebar() { document.getElementById('sidebar').classList.remove('open'); document.getElementById('sidebar-overlay').classList.remove('show'); }

    // ── Init: load overview data immediately ──────────────────────────
    (async () => {
      await Promise.all([loadAllTickets(), loadEngineers()]);
    })();
