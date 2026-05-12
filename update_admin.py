import re

with open('templates/admin-dashboard.html', 'r', encoding='utf-8') as f:
    html = f.read()

# Replace Fonts
html = html.replace('DM+Serif+Display&family=DM+Sans:wght@300;400;500;600', 'Outfit:wght@300;400;500;600;700')
html = html.replace("'DM Sans',sans-serif", "'Outfit',sans-serif")
html = html.replace("'DM Serif Display',serif", "'Outfit',sans-serif")

# Replace the style block
style_start = html.find('<style>')
style_end = html.find('</style>') + 8

new_style = """<style>
    :root {
      --bg-dark: #0f111a;
      --bg-panel: rgba(26, 30, 41, 0.65);
      --glass-border: rgba(255, 255, 255, 0.08);
      --accent-glow: #7000ff;
      --accent-cyan: #00f0ff;
      --text-main: #f0f4f8;
      --text-muted: #8e9bb0;
      --sidebar-w: 260px;
    }
    
    * { box-sizing: border-box; margin: 0; padding: 0; }
    
    body {
      font-family: 'Outfit', sans-serif;
      background-color: var(--bg-dark);
      color: var(--text-main);
      background-image: 
        radial-gradient(circle at 15% 50%, rgba(112, 0, 255, 0.15), transparent 25%),
        radial-gradient(circle at 85% 30%, rgba(0, 240, 255, 0.1), transparent 25%);
      background-attachment: fixed;
      min-height: 100vh;
    }

    /* Glassmorphic Sidebar */
    .sidebar { width: var(--sidebar-w); min-height: 100vh; background: var(--bg-panel); backdrop-filter: blur(16px); -webkit-backdrop-filter: blur(16px); border-right: 1px solid var(--glass-border); position: fixed; top: 0; left: 0; display: flex; flex-direction: column; z-index: 100; transition: transform 0.3s ease; }
    .sidebar-brand { padding: 1.5rem; border-bottom: 1px solid var(--glass-border); display: flex; align-items: center; gap: 1rem; }
    .brand-icon { width: 42px; height: 42px; background: linear-gradient(135deg, var(--accent-glow), var(--accent-cyan)); border-radius: 12px; display: grid; place-items: center; color: #fff; font-size: 1.3rem; box-shadow: 0 8px 24px rgba(112, 0, 255, 0.3); }
    .brand-name { font-size: 1.3rem; font-weight: 700; color: #fff; line-height: 1.1; letter-spacing: 0.5px; }
    .brand-sub { font-size: 0.75rem; color: var(--accent-cyan); font-weight: 500; text-transform: uppercase; letter-spacing: 0.1em; }
    .admin-badge-wrap { padding: 1rem 1.5rem; background: rgba(255, 255, 255, 0.03); border-bottom: 1px solid var(--glass-border); display: flex; align-items: center; gap: 0.8rem; }
    .admin-avatar { width: 38px; height: 38px; background: linear-gradient(135deg, #1a2035, #2a3454); border: 1px solid var(--glass-border); border-radius: 50%; display: grid; place-items: center; color: var(--accent-cyan); font-weight: 700; font-size: 0.9rem; }
    .admin-info .aname { font-size: 0.9rem; font-weight: 600; color: #fff; }
    .admin-info .arole { font-size: 0.75rem; color: var(--accent-cyan); font-weight:600; text-transform:uppercase; letter-spacing:.06em; }
    
    .sidebar-nav { flex: 1; padding: 1.5rem 1rem; }
    .nav-label { font-size: 0.68rem; font-weight: 700; text-transform: uppercase; letter-spacing: 0.15em; color: var(--text-muted); padding: 1rem 1rem 0.5rem; }
    .nav-item { margin-bottom: 4px; }
    .nav-link-item { display: flex; align-items: center; gap: 0.8rem; padding: 0.8rem 1rem; border-radius: 10px; color: var(--text-muted); font-size: 0.95rem; font-weight: 500; cursor: pointer; text-decoration: none; transition: all 0.25s ease; position: relative; overflow: hidden; }
    .nav-link-item:hover { background: rgba(255, 255, 255, 0.05); color: var(--text-main); }
    .nav-link-item.active { background: rgba(0, 240, 255, 0.1); color: var(--accent-cyan); border: 1px solid rgba(0, 240, 255, 0.2); }
    .nav-link-item.active::before { content: ''; position: absolute; left: 0; top: 15%; bottom: 15%; width: 3px; background: var(--accent-cyan); border-radius: 0 4px 4px 0; box-shadow: 0 0 10px var(--accent-cyan); }
    .sidebar-footer { padding: 1.5rem 1rem; border-top: 1px solid var(--glass-border); }

    .main-wrap { margin-left: var(--sidebar-w); min-height: 100vh; display: flex; flex-direction: column; }
    .topbar { height: 80px; padding: 0 2.5rem; display: flex; align-items: center; justify-content: space-between; position: sticky; top: 0; z-index: 90; background: rgba(15, 17, 26, 0.8); backdrop-filter: blur(12px); border-bottom: 1px solid var(--glass-border); }
    .topbar-title { font-size: 1.5rem; font-weight: 600; color: #fff; }
    .topbar-actions { display: flex; align-items: center; gap: 1rem; }
    .topbar-btn { width: 40px; height: 40px; background: rgba(255, 255, 255, 0.03); border: 1px solid var(--glass-border); border-radius: 10px; display: grid; place-items: center; color: var(--text-main); cursor: pointer; transition: all 0.2s; }
    .topbar-btn:hover { background: rgba(0, 240, 255, 0.1); color: var(--accent-cyan); border-color: rgba(0, 240, 255, 0.3); }
    .admin-tag { background: rgba(232,160,32,0.1); color: #e8a020; border: 1px solid rgba(232,160,32,0.2); border-radius: 6px; font-size: 0.75rem; font-weight: 700; padding: 0.3rem 0.7rem; letter-spacing: 0.05em; }

    .main-content { padding: 2.5rem; flex: 1; }
    
    .metric-card { background: var(--bg-panel); backdrop-filter: blur(16px); border: 1px solid var(--glass-border); border-radius: 14px; padding: 1.5rem; display: flex; align-items: flex-start; justify-content: space-between; transition: transform 0.3s; }
    .metric-card:hover { transform: translateY(-3px); }
    .metric-card .m-val { font-size: 2.2rem; font-weight: 700; color: #fff; line-height: 1; }
    .metric-card .m-label { font-size: 0.85rem; color: var(--text-muted); font-weight: 500; text-transform: uppercase; letter-spacing: 0.05em; margin-top: 6px; }
    .metric-card .m-change { font-size: 0.78rem; font-weight: 600; display: flex; align-items: center; gap: 4px; margin-top: 10px; }
    .metric-card .m-icon { width: 52px; height: 52px; border-radius: 12px; display: grid; place-items: center; font-size: 1.4rem; background: rgba(255,255,255,0.03); border: 1px solid var(--glass-border); flex-shrink: 0; }
    
    .icon-cyan { color: var(--accent-cyan); box-shadow: inset 0 0 20px rgba(0, 240, 255, 0.1); }
    .icon-yellow { color: #e8a020; box-shadow: inset 0 0 20px rgba(232, 160, 32, 0.1); }
    .icon-green { color: #00e676; box-shadow: inset 0 0 20px rgba(0, 230, 118, 0.1); }
    .icon-purple { color: #b537f2; box-shadow: inset 0 0 20px rgba(181, 55, 242, 0.1); }

    .section-header { display: flex; align-items: center; gap: 0.8rem; margin-bottom: 1.5rem; }
    .section-header .dot { width: 6px; height: 28px; background: var(--accent-cyan); border-radius: 6px; box-shadow: 0 0 10px var(--accent-cyan); }
    .section-header h3 { font-size: 1.5rem; font-weight: 600; color: #fff; margin: 0; }

    .card-custom { background: var(--bg-panel); backdrop-filter: blur(16px); border: 1px solid var(--glass-border); border-radius: 16px; overflow: hidden; box-shadow: 0 10px 30px rgba(0, 0, 0, 0.15); }
    .card-head { padding: 1.25rem 1.5rem; border-bottom: 1px solid var(--glass-border); display: flex; align-items: center; justify-content: space-between; gap: 0.75rem; flex-wrap: wrap; }
    .card-head h5 { font-size: 1.1rem; font-weight: 600; color: #fff; margin: 0; display: flex; align-items: center; gap: 0.6rem; }

    .table-custom { width: 100%; border-collapse: collapse; }
    .table-custom thead th { background: rgba(0,0,0,0.1); color: var(--text-muted); font-size: 0.75rem; font-weight: 600; text-transform: uppercase; letter-spacing: 0.08em; padding: 1rem 1.5rem; border-bottom: 1px solid var(--glass-border); text-align: left; white-space: nowrap; }
    .table-custom tbody tr { border-bottom: 1px solid rgba(255,255,255,0.03); transition: background 0.12s; }
    .table-custom tbody tr:hover { background: rgba(255,255,255,0.02); }
    .table-custom td { padding: 1rem 1.5rem; font-size: 0.9rem; vertical-align: middle; }
    .ticket-id { color: var(--accent-cyan); font-family: monospace; font-size: 0.9rem; font-weight: 600; }
    .eng-id { color: #b537f2; font-family: monospace; font-size: 0.85rem; font-weight: 600; }

    .badge-status { display: inline-flex; align-items: center; justify-content: center; padding: 0.35rem 0.8rem; border-radius: 50px; font-size: 0.75rem; font-weight: 600; letter-spacing: 0.05em; text-transform: uppercase; white-space: nowrap; }
    .s-open { background: rgba(232, 160, 32, 0.1); color: #e8a020; border: 1px solid rgba(232, 160, 32, 0.2); }
    .s-assigned { background: rgba(0, 240, 255, 0.1); color: var(--accent-cyan); border: 1px solid rgba(0, 240, 255, 0.2); }
    .s-inprogress { background: rgba(181, 55, 242, 0.1); color: #b537f2; border: 1px solid rgba(181, 55, 242, 0.2); }
    .s-resolved { background: rgba(0, 230, 118, 0.1); color: #00e676; border: 1px solid rgba(0, 230, 118, 0.2); }
    .s-closed { background: rgba(142, 155, 176, 0.1); color: var(--text-muted); border: 1px solid rgba(142, 155, 176, 0.2); }

    .btn-sm-close { background: rgba(220, 53, 69, 0.1); border: 1px solid rgba(220, 53, 69, 0.2); color: #ff4d4d; font-size: 0.78rem; font-weight: 600; padding: 0.4rem 0.8rem; border-radius: 8px; cursor: pointer; display: inline-flex; align-items: center; gap: 6px; transition: all 0.2s; }
    .btn-sm-close:hover { background: rgba(220, 53, 69, 0.2); }
    .btn-sm-close:disabled { opacity: 0.4; cursor: not-allowed; }
    
    .btn-sm-assign { background: rgba(0, 240, 255, 0.1); border: 1px solid rgba(0, 240, 255, 0.2); color: var(--accent-cyan); font-size: 0.78rem; font-weight: 600; padding: 0.4rem 0.8rem; border-radius: 8px; cursor: pointer; display: inline-flex; align-items: center; gap: 6px; transition: all 0.2s; }
    .btn-sm-assign:hover { background: rgba(0, 240, 255, 0.2); }
    .btn-sm-assign:disabled { opacity: 0.4; cursor: not-allowed; }

    .filter-bar { display: flex; flex-wrap: wrap; gap: 0.5rem; align-items: center; }
    .filter-bar select, .filter-bar input { background: rgba(0,0,0,0.2); border: 1px solid var(--glass-border); border-radius: 8px; padding: 0.4rem 0.8rem; font-family: inherit; font-size: 0.85rem; color: #fff; transition: all 0.2s; }
    .filter-bar select:focus, .filter-bar input:focus { outline: none; border-color: var(--accent-cyan); }
    .filter-bar select option { background: var(--bg-dark); color: #fff; }

    .load-bar-wrap { display: flex; align-items: center; gap: 0.5rem; }
    .load-bar { height: 6px; border-radius: 99px; background: rgba(255,255,255,0.05); flex: 1; min-width: 60px; overflow: hidden; }
    .load-bar-fill { height: 100%; border-radius: 99px; transition: width 0.4s ease; background: var(--accent-cyan); }

    .empty-state { text-align: center; padding: 4rem 2rem; color: var(--text-muted); }
    .empty-state i { font-size: 3rem; opacity: 0.2; display: block; margin-bottom: 1rem; color: var(--accent-cyan); }

    .toast-wrap { position: fixed; bottom: 2rem; right: 2rem; z-index: 9999; }
    .toast-custom { background: rgba(20, 24, 34, 0.95); backdrop-filter: blur(10px); border: 1px solid var(--glass-border); border-radius: 12px; color: #fff; padding: 1rem 1.25rem; box-shadow: 0 10px 30px rgba(0,0,0,0.3); display: flex; align-items: center; gap: 1rem; }

    .sidebar-toggle { display: none; background: none; border: none; color: #fff; font-size: 1.5rem; cursor: pointer; }
    .sidebar-overlay { display: none; position: fixed; inset: 0; background: rgba(0,0,0,0.6); backdrop-filter: blur(4px); z-index: 99; }
    .sidebar-overlay.show { display: block; }

    @media (max-width: 991px) {
      .sidebar { transform: translateX(-100%); }
      .sidebar.open { transform: translateX(0); }
      .main-wrap { margin-left: 0; }
      .sidebar-toggle { display: block; }
    }
  </style>"""

html = html[:style_start] + new_style + html[style_end:]

# Replace icons
html = html.replace('style="background:#e8f0fe"', 'class="m-icon icon-cyan"')
html = html.replace('style="background:#fff8e1"', 'class="m-icon icon-yellow"')
html = html.replace('style="background:#e8f5e9"', 'class="m-icon icon-green"')
html = html.replace('style="background:#f3e5f5"', 'class="m-icon icon-purple"')

html = html.replace('style="color:var(--blue)"', '')
html = html.replace('style="color:#e8a020"', '')
html = html.replace('style="color:#2e7d32"', '')
html = html.replace('style="color:#7b1fa2"', '')

html = html.replace('text-primary', 'style="color: var(--accent-cyan);"')

# Replace card body colors and stuff inside javascript
html = html.replace('background:#eef0f6', 'background:rgba(255,255,255,0.05)')
html = html.replace('background:#f8f9fd', 'background:rgba(0,0,0,0.2)')
html = html.replace('font-weight:600;color:var(--blue)', 'font-weight:700;color:var(--accent-cyan)')
html = html.replace('font-size:1.1rem; font-weight:600; border-bottom:1px solid var(--glass-border); padding-bottom:0.5rem; margin-top:0.3rem', 'font-size:1.1rem; font-weight:600; border-bottom:1px solid rgba(255,255,255,0.05); padding-bottom:0.5rem; margin-top:0.3rem')

# Profile colors
html = html.replace('style="background:#f8f9fd; cursor:not-allowed"', 'style="background:rgba(0,0,0,0.2); cursor:not-allowed"')

with open('templates/admin-dashboard.html', 'w', encoding='utf-8') as f:
    f.write(html)
