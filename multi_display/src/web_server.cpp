#include "web_server.h"
#include "config.h"
#include "network_mgr.h"
#include "display_mgr.h"
#include "weather_mgr.h"
#include "picture_mgr.h"
#include "calendar_mgr.h"
#include "badge_mgr.h"
#include "news_mgr.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include "mbedtls/base64.h"

WebServer WebServerApp::server(80);
bool WebServerApp::refreshRequested = false;
AppRole WebServerApp::activeRole = ROLE_WEATHER;
PowerMode WebServerApp::powerMode = POWER_ALWAYS_ON;

static Preferences prefs;

// Embedded HTML UI (Zero CDN dependencies, works completely offline in AP mode!)
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>LilyGo Multi-Display Hub</title>
  <style>
    :root {
      --bg: #0b1120;
      --card: #1e293b;
      --border: #334155;
      --text: #f8fafc;
      --text-muted: #94a3b8;
      --accent: #3b82f6;
      --accent-hover: #2563eb;
      --success: #10b981;
      --warning: #f59e0b;
      --danger: #ef4444;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: var(--bg);
      color: var(--text);
      padding: 16px;
      display: flex;
      justify-content: center;
    }
    .container {
      width: 100%;
      max-width: 680px;
      display: flex;
      flex-direction: column;
      gap: 16px;
    }
    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-bottom: 1px solid var(--border);
      padding-bottom: 12px;
      flex-wrap: wrap;
      gap: 10px;
    }
    h1 { font-size: 1.3rem; font-weight: 700; display: flex; align-items: center; gap: 8px; }
    .status-badge {
      font-size: 0.75rem;
      padding: 3px 8px;
      border-radius: 9999px;
      font-weight: 600;
      background: #334155;
    }
    .status-badge.active { background: #065f46; color: #a7f3d0; }
    .status-badge.ap { background: #78350f; color: #fde68a; }

    /* Role Nav Tabs */
    .role-tabs {
      display: grid;
      grid-template-columns: repeat(6, 1fr);
      gap: 6px;
      background: #111827;
      padding: 4px;
      border-radius: 10px;
      border: 1px solid var(--border);
    }
    .role-btn {
      background: transparent;
      border: none;
      color: var(--text-muted);
      font-size: 0.8rem;
      font-weight: 600;
      padding: 10px 4px;
      border-radius: 8px;
      cursor: pointer;
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 4px;
      transition: 0.2s;
    }
    .role-btn:hover { color: var(--text); background: rgba(255,255,255,0.05); }
    .role-btn.active { background: var(--accent); color: white; }

    .card {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 18px;
    }
    .card-title {
      font-size: 1.05rem;
      font-weight: 700;
      margin-bottom: 12px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    .form-group {
      margin-bottom: 12px;
    }
    label {
      display: block;
      font-size: 0.8rem;
      font-weight: 600;
      color: var(--text-muted);
      margin-bottom: 4px;
    }
    input, select, textarea {
      width: 100%;
      background: #0f172a;
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 9px 12px;
      color: white;
      font-size: 0.9rem;
    }
    input:focus, select:focus, textarea:focus {
      outline: none;
      border-color: var(--accent);
    }
    .row {
      display: flex;
      gap: 12px;
    }
    .row > * { flex: 1; }

    .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      padding: 10px 16px;
      border-radius: 8px;
      font-weight: 600;
      font-size: 0.88rem;
      cursor: pointer;
      border: none;
      transition: 0.15s;
    }
    .btn-primary { background: var(--accent); color: white; }
    .btn-primary:hover { background: var(--accent-hover); }
    .btn-success { background: var(--success); color: white; }
    .btn-success:hover { background: #059669; }
    .btn-secondary { background: #334155; color: white; }
    .btn-secondary:hover { background: #475569; }
    .btn-danger { background: var(--danger); color: white; }
    .btn-block { width: 100%; }

    /* E-Paper Screen Simulation Canvas */
    .preview-wrapper {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 8px;
      margin-top: 10px;
    }
    canvas.epaper-canvas {
      width: 100%;
      max-width: 500px;
      aspect-ratio: 250 / 122;
      background: white;
      border: 4px solid #475569;
      border-radius: 8px;
      box-shadow: 0 4px 15px rgba(0,0,0,0.4);
      image-rendering: pixelated;
    }
    .notice-box {
      background: rgba(59, 130, 246, 0.1);
      border-left: 4px solid var(--accent);
      padding: 10px 12px;
      border-radius: 0 8px 8px 0;
      font-size: 0.85rem;
      margin-bottom: 12px;
    }
  </style>
</head>
<body>

<div class="container">
  <header>
    <div>
      <h1><span>⚡</span> LilyGo Multi-Display Hub</h1>
      <div style="font-size:0.8rem; color:var(--text-muted); margin-top:2px;">
        2.13" Monochrome E-Paper (250×122)
      </div>
    </div>
    <div style="display:flex; align-items:center; gap:8px;">
      <span class="status-badge" id="badgeRole">Weather</span>
      <span class="status-badge active" id="badgeWifi">STA: Connected</span>
      <span class="status-badge" id="badgeBatt">🔋 100%</span>
      <button class="btn btn-secondary" style="padding:6px 12px; font-size:0.8rem;" onclick="triggerDisplayRefresh()">🔄 Refresh</button>
    </div>
  </header>

  <!-- Role Navigation -->
  <div class="role-tabs">
    <button class="role-btn active" id="tabBtn0" onclick="selectRole(0)">
      <span>🌦️</span> Weather
    </button>
    <button class="role-btn" id="tabBtn1" onclick="selectRole(1)">
      <span>🖼️</span> Picture
    </button>
    <button class="role-btn" id="tabBtn2" onclick="selectRole(2)">
      <span>📜</span> Calendar
    </button>
    <button class="role-btn" id="tabBtn3" onclick="selectRole(3)">
      <span>📇</span> Badge
    </button>
    <button class="role-btn" id="tabBtn4" onclick="selectRole(4)">
      <span>📰</span> News
    </button>
    <button class="role-btn" id="tabBtn5" onclick="selectRole(5)">
      <span>⚙️</span> Wi-Fi
    </button>
  </div>

  <!-- ==================== TAB 0: WEATHER ==================== -->
  <div class="card tab-pane" id="paneWeather">
    <div class="card-title">
      <span>🌦️ Weather Station</span>
      <button class="btn btn-primary" style="padding:6px 14px; font-size:0.8rem;" onclick="applyRole(0)">Display Weather</button>
    </div>
    <div class="notice-box">
      Fetches real-time weather & 12h forecast trend from Open-Meteo. Requires Station (Router) Wi-Fi mode with internet access.
    </div>
    <div class="row">
      <div class="form-group">
        <label>Location Name</label>
        <input type="text" id="wLoc" value="Sydney">
      </div>
      <div class="form-group">
        <label>Timezone</label>
        <input type="text" id="wTz" value="Australia/Sydney">
      </div>
    </div>
    <div class="row">
      <div class="form-group">
        <label>Latitude</label>
        <input type="text" id="wLat" value="-33.8688">
      </div>
      <div class="form-group">
        <label>Longitude</label>
        <input type="text" id="wLon" value="151.2093">
      </div>
    </div>
    <button class="btn btn-secondary btn-block" onclick="saveWeatherConfig()">💾 Save Location & Fetch Now</button>
  </div>

  <!-- ==================== TAB 1: PICTURE FRAME ==================== -->
  <div class="card tab-pane" id="panePicture" style="display:none;">
    <div class="card-title">
      <span>🖼️ Digital Picture Frame</span>
      <button class="btn btn-primary" style="padding:6px 14px; font-size:0.8rem;" onclick="applyRole(1)">Display Image</button>
    </div>
    <div class="notice-box">
      Select any photo. Drag directly on the canvas to pan, mouse-wheel to zoom, or use the controls below to resize and frame it perfectly.
    </div>

    <div class="form-group">
      <label>Choose Image</label>
      <input type="file" id="picFile" accept="image/*" onchange="onPictureFileSelected(event)">
    </div>

    <!-- Canvas Preview with Drag Support -->
    <div class="preview-wrapper">
      <div style="font-size:0.75rem; color:var(--text-muted); margin-bottom:4px;">
        💡 <em>Drag directly on preview to position • Mouse wheel to zoom</em>
      </div>
      <canvas id="picCanvas" class="epaper-canvas" width="250" height="122" style="cursor:grab;"></canvas>
    </div>

    <!-- Framing & Sizing Controls -->
    <div style="background:#0f172a; border:1px solid var(--border); border-radius:8px; padding:12px; margin-top:12px; display:flex; flex-direction:column; gap:10px;">
      <div class="row">
        <div class="form-group" style="margin-bottom:0;">
          <label>Fit Mode</label>
          <select id="picFitMode" onchange="updatePicFit()">
            <option value="cover">Fill Screen (Cover, keep ratio)</option>
            <option value="contain">Fit Entire Image (Contain)</option>
            <option value="stretch">Stretch to 250×122</option>
          </select>
        </div>
        <div class="form-group" style="margin-bottom:0;">
          <label>Frame / Border Style</label>
          <select id="picFrameStyle" onchange="updatePicFrame()">
            <option value="none">None (Full Bleed)</option>
            <option value="simple">Simple 1px Border</option>
            <option value="double">Classic Double Mat Frame</option>
            <option value="card">Rounded Card Border</option>
            <option value="polaroid">Polaroid Style Frame</option>
          </select>
        </div>
      </div>

      <div class="row">
        <div class="form-group" style="margin-bottom:0;">
          <label>Zoom Scale (<span id="zoomVal">100%</span>)</label>
          <input type="range" id="picZoom" min="20" max="350" value="100" oninput="updatePicZoom(this.value)">
        </div>
        <div class="form-group" style="margin-bottom:0;">
          <label>Pan X (<span id="panXVal">0</span>px)</label>
          <input type="range" id="picPanX" min="-150" max="150" value="0" oninput="updatePicPanX(this.value)">
        </div>
        <div class="form-group" style="margin-bottom:0;">
          <label>Pan Y (<span id="panYVal">0</span>px)</label>
          <input type="range" id="picPanY" min="-100" max="100" value="0" oninput="updatePicPanY(this.value)">
        </div>
      </div>

      <div style="display:flex; gap:6px; flex-wrap:wrap;">
        <button class="btn btn-secondary" style="flex:1; padding:6px 10px; font-size:0.8rem;" onclick="rotateImage(90)">↷ Rotate 90°</button>
        <button class="btn btn-secondary" style="flex:1; padding:6px 10px; font-size:0.8rem;" onclick="flipImage()">⇄ Flip Horiz</button>
        <button class="btn btn-secondary" style="flex:1; padding:6px 10px; font-size:0.8rem;" onclick="toggleInvert()">☯️ Invert</button>
        <button class="btn btn-secondary" style="flex:1; padding:6px 10px; font-size:0.8rem;" onclick="resetPosition()">🎯 Center / Reset</button>
      </div>

      <div class="row">
        <div class="form-group" style="margin-bottom:0;">
          <label>Dither Threshold (<span id="threshVal">128</span>)</label>
          <input type="range" id="ditherThresh" min="30" max="220" value="128" oninput="updatePicThresh(this.value)">
        </div>
        <div class="form-group" style="margin-bottom:0;">
          <label>Brightness (<span id="brightVal">0</span>)</label>
          <input type="range" id="picBright" min="-60" max="60" value="0" oninput="updatePicBright(this.value)">
        </div>
        <div class="form-group" style="margin-bottom:0;">
          <label>Contrast (<span id="contrastVal">0</span>)</label>
          <input type="range" id="picContrast" min="-60" max="60" value="0" oninput="updatePicContrast(this.value)">
        </div>
      </div>
    </div>

    <div class="form-group" style="margin-top:12px;">
      <label>Caption Overlay (Optional)</label>
      <input type="text" id="picCaption" placeholder="e.g. Mountain Views 2026">
    </div>

    <button class="btn btn-success btn-block" id="uploadBtn" onclick="uploadProcessedPicture()" disabled>
      ⚡ Upload & Show on E-Paper
    </button>
  </div>

  <!-- ==================== TAB 2: DAILY CALENDAR ==================== -->
  <div class="card tab-pane" id="paneCalendar" style="display:none;">
    <div class="card-title">
      <span>📜 Daily Stoic & Word Calendar</span>
      <button class="btn btn-primary" style="padding:6px 14px; font-size:0.8rem;" onclick="applyRole(2)">Display Calendar</button>
    </div>
    <div class="row" style="margin-bottom:12px;">
      <div class="form-group">
        <label>Calendar Content Category</label>
        <select id="calCat" onchange="changeCalCategory()">
          <option value="0">Daily Stoic Philosophy (Aurelius, Seneca, Epictetus)</option>
          <option value="1">Word of the Day (Rare Vocabulary & Etymology)</option>
          <option value="2">This Day in History (Historic Milestones)</option>
        </select>
      </div>
    </div>
    <div style="background:#0f172a; border:1px solid var(--border); border-radius:8px; padding:12px; margin-bottom:14px;">
      <div style="font-size:0.75rem; color:var(--text-muted); text-transform:uppercase;" id="calPreviewCat">DAILY STOIC</div>
      <div style="font-size:1.1rem; font-weight:700; margin:4px 0;" id="calPreviewSub">Marcus Aurelius</div>
      <div style="font-size:0.88rem; line-height:1.4; color:#cbd5e1;" id="calPreviewBody">
        "You have power over your mind - not outside events. Realize this, and you will find strength."
      </div>
    </div>
    <div style="display:flex; gap:8px;">
      <button class="btn btn-secondary" style="flex:1;" onclick="calAction('prev')">⏮️ Prev Page</button>
      <button class="btn btn-success" style="flex:2;" onclick="calAction('next')">📜 Tear Off Next Page</button>
    </div>
  </div>

  <!-- ==================== TAB 3: SMART BADGE ==================== -->
  <div class="card tab-pane" id="paneBadge" style="display:none;">
    <div class="card-title">
      <span>📇 Smart Name Badge & Luggage Tag</span>
      <button class="btn btn-primary" style="padding:6px 14px; font-size:0.8rem;" onclick="applyRole(3)">Display Badge</button>
    </div>
    <div class="form-group">
      <label>Badge Mode</label>
      <select id="badgeSubMode" onchange="switchBadgeMode()">
        <option value="0">Conference Badge (Name, Role, Company, Handle + QR)</option>
        <option value="1">Luggage / Bag Tag (Owner, Phone, Email, Destination + QR)</option>
        <option value="2">Desk Status Notice (e.g. DO NOT DISTURB, Meeting)</option>
      </select>
    </div>

    <!-- Conference Fields -->
    <div id="badgeFormConf">
      <div class="row">
        <div class="form-group">
          <label>Attendee Name</label>
          <input type="text" id="bName" value="Alex Rivers">
        </div>
        <div class="form-group">
          <label>Social Handle</label>
          <input type="text" id="bHandle" value="@alexrivers">
        </div>
      </div>
      <div class="row">
        <div class="form-group">
          <label>Job Title</label>
          <input type="text" id="bTitle" value="Software Engineer">
        </div>
        <div class="form-group">
          <label>Company / Organization</label>
          <input type="text" id="bCompany" value="Acme Corp">
        </div>
      </div>
      <div class="form-group">
        <label>QR Code Link / Text</label>
        <input type="text" id="bQr" value="https://github.com">
      </div>
    </div>

    <!-- Luggage Fields -->
    <div id="badgeFormLugg" style="display:none;">
      <div class="row">
        <div class="form-group">
          <label>Owner Name</label>
          <input type="text" id="lOwner" value="Alex Rivers">
        </div>
        <div class="form-group">
          <label>Phone Number</label>
          <input type="text" id="lPhone" value="+1 555-0199">
        </div>
      </div>
      <div class="row">
        <div class="form-group">
          <label>Email Address</label>
          <input type="text" id="lEmail" value="alex@example.com">
        </div>
        <div class="form-group">
          <label>Flight / Reward Note</label>
          <input type="text" id="lNote" value="Reward if returned intact!">
        </div>
      </div>
    </div>

    <!-- Desk Notice Fields -->
    <div id="badgeFormStat" style="display:none;">
      <div class="form-group">
        <label>Notice Header Title</label>
        <input type="text" id="sTitle" value="DO NOT DISTURB">
      </div>
      <div class="form-group">
        <label>Subtitle / Reason</label>
        <input type="text" id="sSub" value="In Deep Focus Sprint">
      </div>
      <div class="form-group">
        <label>Footer / ETA</label>
        <input type="text" id="sFoot" value="Back online at 2:00 PM">
      </div>
    </div>

    <button class="btn btn-success btn-block" style="margin-top:12px;" onclick="saveBadgeConfig()">
      ⚡ Save & Show on Badge
    </button>
  </div>

  <!-- ==================== TAB 4: NEWS READER ==================== -->
  <div class="card tab-pane" id="paneNews" style="display:none;">
    <div class="card-title">
      <span>📰 Live News Reader</span>
      <button class="btn btn-primary" style="padding:6px 14px; font-size:0.8rem;" onclick="applyRole(4)">Display News</button>
    </div>
    <div class="notice-box">
      Browse top tech & world headlines on your e-paper screen with scannable QR code to open full articles on your phone.
    </div>
    <div class="form-group">
      <label>News Source</label>
      <select id="newsSource" onchange="switchNewsSource()">
        <option value="0">Hacker News (Top Stories)</option>
        <option value="1">Reddit (Subreddit Feed)</option>
        <option value="2">Custom RSS / Atom Feed</option>
      </select>
    </div>

    <!-- Reddit Subreddit input -->
    <div class="form-group" id="newsSubGroup" style="display:none;">
      <label>Subreddit Name (e.g. technology, programming, news, worldnews)</label>
      <input type="text" id="newsSubreddit" value="technology" placeholder="technology">
    </div>

    <!-- RSS URL input -->
    <div class="form-group" id="newsRssGroup" style="display:none;">
      <label>RSS / Atom Feed URL</label>
      <input type="text" id="newsRssUrl" value="https://feeds.bbci.co.uk/news/world/rss.xml" placeholder="https://...">
    </div>

    <!-- Preview Box -->
    <div style="background:#0f172a; border:1px solid var(--border); border-radius:8px; padding:12px; margin-bottom:14px;">
      <div style="display:flex; justify-content:space-between; align-items:center;">
        <span style="font-size:0.75rem; color:var(--accent); font-weight:700; text-transform:uppercase;" id="newsPreviewSource">HACKER NEWS</span>
        <span style="font-size:0.75rem; color:var(--text-muted);" id="newsPreviewCount">#1/10</span>
      </div>
      <div style="font-size:1.05rem; font-weight:700; margin:6px 0; color:#f8fafc;" id="newsPreviewTitle">
        Show HN: LilyGo E-Paper Multi-Display System
      </div>
      <div style="font-size:0.8rem; color:var(--text-muted); margin-bottom:6px;" id="newsPreviewMeta">
        340 pts • 92 cmts
      </div>
      <div style="font-size:0.75rem; color:#60a5fa; word-break:break-all;" id="newsPreviewUrl">
        https://news.ycombinator.com
      </div>
    </div>

    <div style="display:flex; gap:8px; margin-bottom:10px;">
      <button class="btn btn-secondary" style="flex:1;" onclick="newsAction('prev')">⏮️ Prev Story</button>
      <button class="btn btn-secondary" style="flex:1;" onclick="newsAction('next')">⏭️ Next Story</button>
    </div>

    <button class="btn btn-success btn-block" onclick="saveNewsConfigAndFetch()">
      ⚡ Save Source & Fetch Headlines
    </button>
  </div>

  <!-- ==================== TAB 5: WI-FI & SYSTEM ==================== -->
  <div class="card tab-pane" id="paneSystem" style="display:none;">
    <div class="card-title">
      <span>⚙️ Wi-Fi & System Settings</span>
    </div>
    
    <div class="form-group">
      <label>Wi-Fi Operating Mode</label>
      <select id="sysWifiMode" onchange="toggleWifiFields()">
        <option value="0">Connect to Supplied Router (Station Mode)</option>
        <option value="1">Self-Hosted Access Point (SoftAP Mode)</option>
      </select>
    </div>

    <!-- Router STA Settings -->
    <div id="staFields">
      <div class="form-group">
        <label>Router Wi-Fi SSID (Name)</label>
        <input type="text" id="sysStaSsid" placeholder="Your Wi-Fi SSID">
      </div>
      <div class="form-group">
        <label>Router Wi-Fi Password</label>
        <input type="password" id="sysStaPass" placeholder="Your Wi-Fi Password">
      </div>
    </div>

    <!-- SoftAP Settings -->
    <div id="apFields" style="display:none;">
      <div class="form-group">
        <label>Self-Hosted Access Point SSID</label>
        <input type="text" id="sysApSsid" value="MultiDisplay-AP">
      </div>
      <div class="form-group">
        <label>AP Password (Leave empty for open network, or min 8 chars)</label>
        <input type="password" id="sysApPass" placeholder="Optional password">
      </div>
    </div>

    <div class="form-group" style="margin-top:16px;">
      <label>Power Management</label>
      <select id="sysPowerMode">
        <option value="0">Always-On Web Server (Continuously accessible)</option>
        <option value="1">Power-Saver Deep Sleep (Sleeps after update; button wakes)</option>
      </select>
    </div>

    <div style="display:flex; gap:10px; margin-top:16px;">
      <button class="btn btn-primary" style="flex:2;" onclick="saveWifiAndSystem()">💾 Save & Apply</button>
      <button class="btn btn-danger" style="flex:1;" onclick="rebootSystem()">🔄 Reboot</button>
    </div>
  </div>

</div>

<script>
let currentRole = 0;
let rawBitmapBytes = null;

// Initialise
window.addEventListener('DOMContentLoaded', () => {
  fetchStatus();
  initCanvas();
});

function selectRole(role) {
  for (let i = 0; i < 6; i++) {
    const pane = document.querySelectorAll('.tab-pane')[i];
    const btn = document.getElementById('tabBtn' + i);
    if (pane) pane.style.display = (i === role) ? 'block' : 'none';
    if (btn) btn.classList.toggle('active', i === role);
  }
}

async function fetchStatus() {
  try {
    const res = await fetch('/api/status');
    const data = await res.json();
    currentRole = data.role;
    selectRole(currentRole);

    const roles = ["🌦️ Weather", "🖼️ Picture", "📜 Calendar", "📇 Badge", "📰 News"];
    document.getElementById("badgeRole").innerText = roles[data.role] || "Ready";
    document.getElementById("badgeWifi").innerText = (data.wifi_mode === 1) ? ("AP: " + data.ssid) : ("STA: " + data.ip);
    document.getElementById("badgeWifi").className = (data.wifi_mode === 1) ? "status-badge ap" : "status-badge active";
    document.getElementById("badgeBatt").innerText = "🔋 " + data.batt_pct + "%";

    // Fill form fields
    if (data.weather) {
      document.getElementById("wLoc").value = data.weather.loc || "Sydney";
      document.getElementById("wLat").value = data.weather.lat || "-33.8688";
      document.getElementById("wLon").value = data.weather.lon || "151.2093";
      document.getElementById("wTz").value = data.weather.tz || "Australia/Sydney";
    }
    if (data.picture && data.picture.has_image && !loadedImageObj) {
      fetch('/api/picture/current')
        .then(r => r.json())
        .then(pd => {
          if (pd.has_image && pd.image && !loadedImageObj) {
            drawBitmapToCanvas(pd.image, pd.caption);
          }
        })
        .catch(e => console.warn(e));
    }
    if (data.badge) {
      document.getElementById("badgeSubMode").value = data.badge.submode || 0;
      document.getElementById("bName").value = data.badge.name || "Alex Rivers";
      document.getElementById("bTitle").value = data.badge.title || "Software Engineer";
      document.getElementById("bCompany").value = data.badge.comp || "Acme Corp";
      document.getElementById("bHandle").value = data.badge.handle || "@alexrivers";
      document.getElementById("bQr").value = data.badge.qr || "https://github.com";
      document.getElementById("lOwner").value = data.badge.name || "Alex Rivers";
      document.getElementById("lPhone").value = data.badge.phone || "+1 555-0199";
      document.getElementById("lEmail").value = data.badge.email || "alex@example.com";
      document.getElementById("lNote").value = data.badge.note || "Reward if returned intact!";
      document.getElementById("sTitle").value = data.badge.stitle || "DO NOT DISTURB";
      document.getElementById("sSub").value = data.badge.ssub || "In Deep Focus Sprint";
      document.getElementById("sFoot").value = data.badge.sfoot || "Back online at 2:00 PM";
      switchBadgeMode();
    }
    if (data.news) {
      document.getElementById("newsSource").value = data.news.source || 0;
      document.getElementById("newsSubreddit").value = data.news.subreddit || "technology";
      document.getElementById("newsRssUrl").value = data.news.rss_url || "https://feeds.bbci.co.uk/news/world/rss.xml";
      if (data.news.article) {
        document.getElementById("newsPreviewSource").innerText = data.news.article.source || "NEWS";
        document.getElementById("newsPreviewCount").innerText = "#" + ((data.news.article.index || 0) + 1) + "/" + (data.news.article.total || 1);
        document.getElementById("newsPreviewTitle").innerText = data.news.article.title || "";
        document.getElementById("newsPreviewMeta").innerText = data.news.article.meta || "";
        document.getElementById("newsPreviewUrl").innerText = data.news.article.url || "";
      }
      switchNewsSource();
    }
    if (data.network) {
      document.getElementById("sysWifiMode").value = data.network.mode || 0;
      document.getElementById("sysStaSsid").value = data.network.sta_ssid || "";
      document.getElementById("sysApSsid").value = data.network.ap_ssid || "MultiDisplay-AP";
      toggleWifiFields();
    }
    if (data.power !== undefined) {
      document.getElementById("sysPowerMode").value = data.power;
    }
  } catch (e) {
    console.warn("Status fetch failed:", e);
  }
}

async function applyRole(role) {
  try {
    const res = await fetch('/api/role', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ role: role })
    });
    const d = await res.json();
    alert("Role applied! Display is updating.");
    fetchStatus();
  } catch (e) {
    alert("Failed to apply role: " + e);
  }
}

async function triggerDisplayRefresh() {
  await fetch('/api/system/refresh', { method: 'POST' });
  alert("Refresh triggered! E-paper is redrawing.");
}

// Weather
async function saveWeatherConfig() {
  const payload = {
    loc: document.getElementById("wLoc").value,
    lat: document.getElementById("wLat").value,
    lon: document.getElementById("wLon").value,
    tz: document.getElementById("wTz").value
  };
  await fetch('/api/weather/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });
  applyRole(0);
}

// Calendar
async function calAction(act) {
  const cat = parseInt(document.getElementById("calCat").value);
  const res = await fetch('/api/calendar/action', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ action: act, cat: cat })
  });
  const data = await res.json();
  if (data.item) {
    document.getElementById("calPreviewCat").innerText = data.item.title;
    document.getElementById("calPreviewSub").innerText = data.item.subtitle;
    document.getElementById("calPreviewBody").innerText = data.item.body;
  }
}

function changeCalCategory() {
  calAction('cat');
}

// Badge
function switchBadgeMode() {
  const m = parseInt(document.getElementById("badgeSubMode").value);
  document.getElementById("badgeFormConf").style.display = (m === 0) ? "block" : "none";
  document.getElementById("badgeFormLugg").style.display = (m === 1) ? "block" : "none";
  document.getElementById("badgeFormStat").style.display = (m === 2) ? "block" : "none";
}

async function saveBadgeConfig() {
  const m = parseInt(document.getElementById("badgeSubMode").value);
  const payload = {
    submode: m,
    name: (m === 1) ? document.getElementById("lOwner").value : document.getElementById("bName").value,
    title: document.getElementById("bTitle").value,
    comp: document.getElementById("bCompany").value,
    handle: document.getElementById("bHandle").value,
    qr: document.getElementById("bQr").value,
    phone: document.getElementById("lPhone").value,
    email: document.getElementById("lEmail").value,
    note: document.getElementById("lNote").value,
    stitle: document.getElementById("sTitle").value,
    ssub: document.getElementById("sSub").value,
    sfoot: document.getElementById("sFoot").value
  };
  await fetch('/api/badge/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });
  applyRole(3);
}

// News
function switchNewsSource() {
  const src = parseInt(document.getElementById("newsSource").value);
  document.getElementById("newsSubGroup").style.display = (src === 1) ? "block" : "none";
  document.getElementById("newsRssGroup").style.display = (src === 2) ? "block" : "none";
}

async function newsAction(act) {
  const res = await fetch('/api/news/action', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ action: act })
  });
  const data = await res.json();
  if (data.article) {
    document.getElementById("newsPreviewSource").innerText = data.article.source || "NEWS";
    document.getElementById("newsPreviewCount").innerText = "#" + ((data.article.index || 0) + 1) + "/" + (data.article.total || 1);
    document.getElementById("newsPreviewTitle").innerText = data.article.title || "";
    document.getElementById("newsPreviewMeta").innerText = data.article.meta || "";
    document.getElementById("newsPreviewUrl").innerText = data.article.url || "";
  }
}

async function saveNewsConfigAndFetch() {
  const payload = {
    source: parseInt(document.getElementById("newsSource").value),
    subreddit: document.getElementById("newsSubreddit").value,
    rss_url: document.getElementById("newsRssUrl").value
  };
  await fetch('/api/news/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });
  applyRole(4);
}

// Wi-Fi Settings
function toggleWifiFields() {
  const m = parseInt(document.getElementById("sysWifiMode").value);
  document.getElementById("staFields").style.display = (m === 0) ? "block" : "none";
  document.getElementById("apFields").style.display = (m === 1) ? "block" : "none";
}

async function saveWifiAndSystem() {
  const payload = {
    mode: parseInt(document.getElementById("sysWifiMode").value),
    sta_ssid: document.getElementById("sysStaSsid").value,
    sta_pass: document.getElementById("sysStaPass").value,
    ap_ssid: document.getElementById("sysApSsid").value,
    ap_pass: document.getElementById("sysApPass").value,
    power: parseInt(document.getElementById("sysPowerMode").value)
  };
  await fetch('/api/wifi', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });
  alert("Network settings saved! If Wi-Fi mode or credentials changed, reboot to connect.");
}

async function rebootSystem() {
  if (confirm("Reboot LilyGo Multi-Display?")) {
    await fetch('/api/system/reboot', { method: 'POST' });
    alert("Rebooting... please wait 10 seconds and reconnect.");
  }
}

// -------------------------------------------------------------
// Client-Side Canvas Floyd-Steinberg Dithering (Picture Frame)
// -------------------------------------------------------------
let loadedImageObj = null;
let picState = {
  zoom: 1.0,
  panX: 0,
  panY: 0,
  rotation: 0,
  flipH: false,
  fit: 'cover',
  frame: 'none',
  thresh: 128,
  bright: 0,
  contrast: 0,
  invert: false
};

function initCanvas() {
  const canvas = document.getElementById("picCanvas");
  const ctx = canvas.getContext("2d");
  ctx.fillStyle = "#ffffff";
  ctx.fillRect(0, 0, 250, 122);
  ctx.fillStyle = "#64748b";
  ctx.font = "14px sans-serif";
  ctx.textAlign = "center";
  ctx.fillText("Select an image to preview", 125, 65);

  setupCanvasInteraction(canvas);
}

function setupCanvasInteraction(canvas) {
  let isDragging = false;
  let dragStartX = 0, dragStartY = 0;
  let initialPanX = 0, initialPanY = 0;

  canvas.addEventListener('mousedown', (e) => {
    if (!loadedImageObj) return;
    isDragging = true;
    dragStartX = e.clientX;
    dragStartY = e.clientY;
    initialPanX = picState.panX;
    initialPanY = picState.panY;
    canvas.style.cursor = 'grabbing';
  });

  window.addEventListener('mousemove', (e) => {
    if (!isDragging) return;
    const rect = canvas.getBoundingClientRect();
    const scaleX = 250 / rect.width;
    const scaleY = 122 / rect.height;
    const dx = (e.clientX - dragStartX) * scaleX;
    const dy = (e.clientY - dragStartY) * scaleY;
    picState.panX = Math.round(initialPanX + dx);
    picState.panY = Math.round(initialPanY + dy);
    document.getElementById("picPanX").value = picState.panX;
    document.getElementById("panXVal").innerText = picState.panX;
    document.getElementById("picPanY").value = picState.panY;
    document.getElementById("panYVal").innerText = picState.panY;
    reprocessImage();
  });

  window.addEventListener('mouseup', () => {
    if (isDragging) {
      isDragging = false;
      canvas.style.cursor = 'grab';
    }
  });

  canvas.addEventListener('touchstart', (e) => {
    if (!loadedImageObj || e.touches.length !== 1) return;
    isDragging = true;
    dragStartX = e.touches[0].clientX;
    dragStartY = e.touches[0].clientY;
    initialPanX = picState.panX;
    initialPanY = picState.panY;
  }, { passive: false });

  window.addEventListener('touchmove', (e) => {
    if (!isDragging || e.touches.length !== 1) return;
    const rect = canvas.getBoundingClientRect();
    const scaleX = 250 / rect.width;
    const scaleY = 122 / rect.height;
    const dx = (e.touches[0].clientX - dragStartX) * scaleX;
    const dy = (e.touches[0].clientY - dragStartY) * scaleY;
    picState.panX = Math.round(initialPanX + dx);
    picState.panY = Math.round(initialPanY + dy);
    document.getElementById("picPanX").value = picState.panX;
    document.getElementById("panXVal").innerText = picState.panX;
    document.getElementById("picPanY").value = picState.panY;
    document.getElementById("panYVal").innerText = picState.panY;
    reprocessImage();
    e.preventDefault();
  }, { passive: false });

  window.addEventListener('touchend', () => { isDragging = false; });

  canvas.addEventListener('wheel', (e) => {
    if (!loadedImageObj) return;
    e.preventDefault();
    const delta = e.deltaY < 0 ? 0.08 : -0.08;
    let newZoom = Math.min(3.5, Math.max(0.2, picState.zoom + delta));
    picState.zoom = Math.round(newZoom * 100) / 100;
    document.getElementById("picZoom").value = Math.round(picState.zoom * 100);
    document.getElementById("zoomVal").innerText = Math.round(picState.zoom * 100) + "%";
    reprocessImage();
  }, { passive: false });
}

function onPictureFileSelected(e) {
  const file = e.target.files[0];
  if (!file) return;
  const reader = new FileReader();
  reader.onload = function(evt) {
    const img = new Image();
    img.onload = function() {
      loadedImageObj = img;
      resetPosition();
      document.getElementById("uploadBtn").disabled = false;
    };
    img.src = evt.target.result;
  };
  reader.readAsDataURL(file);
}

function applyFrameToGray(gray, w, h, frame) {
  function setPix(x, y, black) {
    if (x >= 0 && x < w && y >= 0 && y < h) {
      gray[y * w + x] = black ? 0 : 255;
    }
  }
  function hLine(y, x1, x2, black) {
    for (let x = x1; x <= x2; x++) setPix(x, y, black);
  }
  function vLine(x, y1, y2, black) {
    for (let y = y1; y <= y2; y++) setPix(x, y, black);
  }

  if (frame === 'simple') {
    hLine(0, 0, w - 1, true);
    hLine(h - 1, 0, w - 1, true);
    vLine(0, 0, h - 1, true);
    vLine(w - 1, 0, h - 1, true);
  } else if (frame === 'double') {
    hLine(0, 0, w - 1, true);
    hLine(h - 1, 0, w - 1, true);
    vLine(0, 0, h - 1, true);
    vLine(w - 1, 0, h - 1, true);

    for (let d = 1; d <= 2; d++) {
      hLine(d, d, w - 1 - d, false);
      hLine(h - 1 - d, d, w - 1 - d, false);
      vLine(d, d, h - 1 - d, false);
      vLine(w - 1 - d, d, h - 1 - d, false);
    }

    hLine(3, 3, w - 4, true);
    hLine(h - 4, 3, w - 4, true);
    vLine(3, 3, h - 4, true);
    vLine(w - 4, 3, h - 4, true);
  } else if (frame === 'card') {
    for (let d = 0; d < 4; d++) {
      hLine(d, 0, w - 1, false);
      hLine(h - 1 - d, 0, w - 1, false);
      vLine(d, 0, h - 1, false);
      vLine(w - 1 - d, 0, h - 1, false);
    }
    hLine(4, 7, w - 8, true);
    hLine(h - 5, 7, w - 8, true);
    vLine(4, 7, h - 8, true);
    vLine(w - 5, 7, h - 8, true);
    setPix(5, 5, true); setPix(6, 4, true); setPix(4, 6, true);
    setPix(w - 6, 5, true); setPix(w - 7, 4, true); setPix(w - 5, 6, true);
    setPix(5, h - 6, true); setPix(6, h - 5, true); setPix(4, h - 7, true);
    setPix(w - 6, h - 6, true); setPix(w - 7, h - 5, true); setPix(w - 5, h - 7, true);
  } else if (frame === 'polaroid') {
    for (let d = 0; d < 4; d++) {
      hLine(d, 0, w - 1, false);
      vLine(d, 0, h - 1, false);
      vLine(w - 1 - d, 0, h - 1, false);
    }
    for (let y = h - 22; y < h; y++) hLine(y, 0, w - 1, false);
    hLine(4, 4, w - 5, true);
    hLine(h - 23, 4, w - 5, true);
    vLine(4, 4, h - 23, true);
    vLine(w - 5, 4, h - 23, true);
  }
}

function reprocessImage() {
  if (!loadedImageObj) return;
  const targetW = 250;
  const targetH = 122;
  const canvas = document.getElementById("picCanvas");
  const ctx = canvas.getContext("2d");

  ctx.fillStyle = "#ffffff";
  ctx.fillRect(0, 0, targetW, targetH);

  ctx.save();
  ctx.translate(targetW / 2 + picState.panX, targetH / 2 + picState.panY);
  if (picState.rotation !== 0) {
    ctx.rotate((picState.rotation * Math.PI) / 180);
  }
  if (picState.flipH) {
    ctx.scale(-1, 1);
  }

  const isRot90 = (picState.rotation === 90 || picState.rotation === 270);
  const curImgW = isRot90 ? loadedImageObj.height : loadedImageObj.width;
  const curImgH = isRot90 ? loadedImageObj.width : loadedImageObj.height;

  let baseScale = 1.0;
  if (picState.fit === 'cover') {
    baseScale = Math.max(targetW / curImgW, targetH / curImgH);
  } else if (picState.fit === 'contain') {
    baseScale = Math.min(targetW / curImgW, targetH / curImgH);
  }

  if (picState.fit === 'stretch' && !isRot90) {
    ctx.drawImage(loadedImageObj, (-targetW / 2) * picState.zoom, (-targetH / 2) * picState.zoom, targetW * picState.zoom, targetH * picState.zoom);
  } else {
    let dw = loadedImageObj.width * baseScale * picState.zoom;
    let dh = loadedImageObj.height * baseScale * picState.zoom;
    ctx.drawImage(loadedImageObj, -dw / 2, -dh / 2, dw, dh);
  }
  ctx.restore();

  const imgData = ctx.getImageData(0, 0, targetW, targetH);
  const d = imgData.data;

  let gray = new Float32Array(targetW * targetH);
  let factor = (259 * (picState.contrast + 255)) / (255 * (259 - picState.contrast));
  for (let i = 0; i < targetW * targetH; i++) {
    let r = d[i * 4];
    let g = d[i * 4 + 1];
    let b = d[i * 4 + 2];
    let lum = 0.299 * r + 0.587 * g + 0.114 * b + picState.bright;
    lum = factor * (lum - 128) + 128;
    gray[i] = Math.max(0, Math.min(255, lum));
  }

  for (let y = 0; y < targetH; y++) {
    for (let x = 0; x < targetW; x++) {
      let idx = y * targetW + x;
      let oldPixel = gray[idx];
      let newPixel = oldPixel < picState.thresh ? 0 : 255;
      gray[idx] = newPixel;
      let error = oldPixel - newPixel;

      if (x + 1 < targetW) gray[idx + 1] += error * 7 / 16;
      if (x - 1 >= 0 && y + 1 < targetH) gray[idx + targetW - 1] += error * 3 / 16;
      if (y + 1 < targetH) gray[idx + targetW] += error * 5 / 16;
      if (x + 1 < targetW && y + 1 < targetH) gray[idx + targetW + 1] += error * 1 / 16;
    }
  }

  if (picState.invert) {
    for (let i = 0; i < targetW * targetH; i++) {
      gray[i] = 255 - gray[i];
    }
  }

  applyFrameToGray(gray, targetW, targetH, picState.frame);

  const bytesPerRow = 32;
  rawBitmapBytes = new Uint8Array(bytesPerRow * targetH);

  for (let y = 0; y < targetH; y++) {
    for (let x = 0; x < targetW; x++) {
      let val = gray[y * targetW + x];
      let pixelBlack = (val === 0);

      if (pixelBlack) {
        let byteIdx = y * bytesPerRow + Math.floor(x / 8);
        let bitIdx = 7 - (x % 8);
        rawBitmapBytes[byteIdx] |= (1 << bitIdx);
      }

      let pIdx = (y * targetW + x) * 4;
      d[pIdx] = val;
      d[pIdx + 1] = val;
      d[pIdx + 2] = val;
      d[pIdx + 3] = 255;
    }
  }

  ctx.putImageData(imgData, 0, 0);
}

function updatePicZoom(val) {
  picState.zoom = parseFloat(val) / 100.0;
  document.getElementById("zoomVal").innerText = val + "%";
  reprocessImage();
}
function updatePicPanX(val) {
  picState.panX = parseInt(val);
  document.getElementById("panXVal").innerText = val;
  reprocessImage();
}
function updatePicPanY(val) {
  picState.panY = parseInt(val);
  document.getElementById("panYVal").innerText = val;
  reprocessImage();
}
function updatePicFit() {
  picState.fit = document.getElementById("picFitMode").value;
  reprocessImage();
}
function updatePicFrame() {
  picState.frame = document.getElementById("picFrameStyle").value;
  reprocessImage();
}
function rotateImage(deg) {
  picState.rotation = (picState.rotation + deg) % 360;
  reprocessImage();
}
function flipImage() {
  picState.flipH = !picState.flipH;
  reprocessImage();
}
function toggleInvert() {
  picState.invert = !picState.invert;
  reprocessImage();
}
function resetPosition() {
  picState.zoom = 1.0;
  picState.panX = 0;
  picState.panY = 0;
  picState.rotation = 0;
  picState.flipH = false;
  picState.fit = 'cover';
  picState.frame = 'none';
  picState.thresh = 128;
  picState.bright = 0;
  picState.contrast = 0;
  picState.invert = false;

  document.getElementById("picZoom").value = 100;
  document.getElementById("zoomVal").innerText = "100%";
  document.getElementById("picPanX").value = 0;
  document.getElementById("panXVal").innerText = "0";
  document.getElementById("picPanY").value = 0;
  document.getElementById("panYVal").innerText = "0";
  document.getElementById("picFitMode").value = 'cover';
  document.getElementById("picFrameStyle").value = 'none';
  document.getElementById("ditherThresh").value = 128;
  document.getElementById("threshVal").innerText = "128";
  document.getElementById("picBright").value = 0;
  document.getElementById("brightVal").innerText = "0";
  document.getElementById("picContrast").value = 0;
  document.getElementById("contrastVal").innerText = "0";
  reprocessImage();
}
function updatePicThresh(val) {
  picState.thresh = parseInt(val);
  document.getElementById("threshVal").innerText = val;
  reprocessImage();
}
function updatePicBright(val) {
  picState.bright = parseInt(val);
  document.getElementById("brightVal").innerText = val;
  reprocessImage();
}
function updatePicContrast(val) {
  picState.contrast = parseInt(val);
  document.getElementById("contrastVal").innerText = val;
  reprocessImage();
}

function drawBitmapToCanvas(b64Data, caption) {
  try {
    const binary = atob(b64Data);
    const canvas = document.getElementById("picCanvas");
    const ctx = canvas.getContext("2d");
    const targetW = 250;
    const targetH = 122;
    const imgData = ctx.createImageData(targetW, targetH);
    const d = imgData.data;
    const bytesPerRow = 32;

    rawBitmapBytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) {
      rawBitmapBytes[i] = binary.charCodeAt(i);
    }

    for (let y = 0; y < targetH; y++) {
      for (let x = 0; x < targetW; x++) {
        let byteIdx = y * bytesPerRow + Math.floor(x / 8);
        let bitIdx = 7 - (x % 8);
        let isBlack = (rawBitmapBytes[byteIdx] & (1 << bitIdx)) !== 0;
        let val = isBlack ? 0 : 255;

        let pIdx = (y * targetW + x) * 4;
        d[pIdx] = val;
        d[pIdx + 1] = val;
        d[pIdx + 2] = val;
        d[pIdx + 3] = 255;
      }
    }

    ctx.putImageData(imgData, 0, 0);
    if (caption) {
      document.getElementById("picCaption").value = caption;
    }
    document.getElementById("uploadBtn").disabled = false;
  } catch(e) {
    console.warn("drawBitmapToCanvas error:", e);
  }
}

async function uploadProcessedPicture() {
  if (!rawBitmapBytes) return;
  const btn = document.getElementById("uploadBtn");
  btn.disabled = true;
  btn.innerText = "⏳ Uploading to E-Paper...";

  const caption = document.getElementById("picCaption").value || "";

  // Convert Uint8Array to Base64 (zero null bytes in JSON payload!)
  let binary = '';
  const len = rawBitmapBytes.byteLength;
  for (let i = 0; i < len; i++) {
    binary += String.fromCharCode(rawBitmapBytes[i]);
  }
  const b64 = btoa(binary);

  try {
    const res = await fetch('/api/picture/upload', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        image: b64,
        caption: caption
      })
    });
    const d = await res.json();
    if (d.status === 'ok') {
      alert("Success! Image received and displaying on e-paper.");
      fetchStatus();
    } else {
      alert("Upload failed: " + (d.error || "Server error"));
    }
  } catch (e) {
    alert("Upload failed: " + e);
  } finally {
    btn.disabled = false;
    btn.innerText = "⚡ Upload & Show on E-Paper";
  }
}
</script>
</body>
</html>
)rawliteral";

void WebServerApp::init() {
    prefs.begin("multi", false);
    activeRole = (AppRole)prefs.getInt("role", (int)ROLE_WEATHER);
    powerMode = (PowerMode)prefs.getInt("power", (int)POWER_ALWAYS_ON);
    prefs.end();

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/role", HTTP_POST, handleSetRole);
    server.on("/api/wifi", HTTP_POST, handleSetWifi);
    server.on("/api/weather/config", HTTP_POST, handleWeatherConfig);
    server.on("/api/picture/upload", HTTP_POST, handlePictureUpload);
    server.on("/api/picture/current", HTTP_GET, handlePictureCurrent);
    server.on("/api/calendar/action", HTTP_POST, handleCalendarAction);
    server.on("/api/badge/config", HTTP_POST, handleBadgeConfig);
    server.on("/api/news/config", HTTP_POST, handleNewsConfig);
    server.on("/api/news/action", HTTP_POST, handleNewsAction);
    server.on("/api/system/refresh", HTTP_POST, handleRefreshDisplay);
    server.on("/api/system/reboot", HTTP_POST, handleReboot);

    // Captive Portal Redirection
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("[WebServer] HTTP server started on port 80");
}

void WebServerApp::handleClient() {
    server.handleClient();
}

bool WebServerApp::isRefreshNeeded() { return refreshRequested; }
void WebServerApp::clearRefreshNeeded() { refreshRequested = false; }
AppRole WebServerApp::getActiveRole() { return activeRole; }

void WebServerApp::setActiveRole(AppRole role) {
    activeRole = role;
    prefs.begin("multi", false);
    prefs.putInt("role", (int)role);
    prefs.end();
    refreshRequested = true;
}

PowerMode WebServerApp::getPowerMode() { return powerMode; }

void WebServerApp::setPowerMode(PowerMode mode) {
    powerMode = mode;
    prefs.begin("multi", false);
    prefs.putInt("power", (int)mode);
    prefs.end();
}

void WebServerApp::handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

void WebServerApp::handleStatus() {
    JsonDocument doc;
    doc["role"] = (int)activeRole;
    doc["wifi_mode"] = (int)NetworkManager::getWifiMode();
    doc["ip"] = NetworkManager::getIpAddress();
    doc["ssid"] = NetworkManager::getSSID();
    doc["rssi"] = NetworkManager::getRSSI();
    doc["power"] = (int)powerMode;

    // Read battery voltage & %
    uint32_t raw = analogRead(PIN_BATTERY);
    float battV = (raw / 4095.0f) * 3.3f * 2.0f;
    int battPct = (battV >= 4.2f) ? 100 : ((battV <= 3.2f) ? 0 : (int)((battV - 3.2f) / (4.2f - 3.2f) * 100.0f));
    doc["batt_v"] = serialized(String(battV, 2));
    doc["batt_pct"] = battPct;

    // Weather
    String wLoc, wLat, wLon, wTz;
    WeatherManager::loadSettings(wLoc, wLat, wLon, wTz);
    JsonObject wObj = doc["weather"].to<JsonObject>();
    wObj["loc"] = wLoc;
    wObj["lat"] = wLat;
    wObj["lon"] = wLon;
    wObj["tz"] = wTz;

    // Picture
    JsonObject picObj = doc["picture"].to<JsonObject>();
    picObj["has_image"] = PictureManager::hasImage();
    picObj["size"] = (int)PictureManager::getImageSize();

    // Badge
    BadgeConfig& bc = BadgeManager::getConfig();
    JsonObject bObj = doc["badge"].to<JsonObject>();
    bObj["submode"] = bc.subMode;
    bObj["name"] = bc.name;
    bObj["title"] = bc.title;
    bObj["comp"] = bc.company;
    bObj["handle"] = bc.handle;
    bObj["qr"] = bc.qrUrl;
    bObj["phone"] = bc.phone;
    bObj["email"] = bc.email;
    bObj["note"] = bc.note;
    bObj["stitle"] = bc.statTitle;
    bObj["ssub"] = bc.statSub;
    bObj["sfoot"] = bc.statFoot;

    // News
    JsonObject newsObj = doc["news"].to<JsonObject>();
    newsObj["source"] = (int)NewsManager::getSource();
    newsObj["subreddit"] = NewsManager::getSubreddit();
    newsObj["rss_url"] = NewsManager::getRssUrl();
    NewsArticle art;
    if (NewsManager::getCurrentArticle(art)) {
        JsonObject aObj = newsObj["article"].to<JsonObject>();
        aObj["title"] = art.title;
        aObj["url"] = art.url;
        aObj["meta"] = art.meta;
        aObj["source"] = art.sourceName;
        aObj["index"] = art.index;
        aObj["total"] = art.total;
    }

    // Network
    String sSsid, sPass, aSsid, aPass;
    NetworkManager::getCredentials(sSsid, sPass, aSsid, aPass);
    JsonObject nObj = doc["network"].to<JsonObject>();
    nObj["mode"] = (int)NetworkManager::getWifiMode();
    nObj["sta_ssid"] = sSsid;
    nObj["ap_ssid"] = aSsid;

    String jsonStr;
    serializeJson(doc, jsonStr);
    server.send(200, "application/json", jsonStr);
}

void WebServerApp::handleSetRole() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    int r = doc["role"] | 0;
    setActiveRole((AppRole)r);
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerApp::handleSetWifi() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    int mode = doc["mode"] | 0;
    String staSsid = doc["sta_ssid"] | "";
    String staPass = doc["sta_pass"] | "";
    String apSsid = doc["ap_ssid"] | DEFAULT_AP_SSID;
    String apPass = doc["ap_pass"] | "";
    int pMode = doc["power"] | (int)POWER_ALWAYS_ON;

    NetworkManager::saveCredentials((NetworkMode)mode, staSsid, staPass, apSsid, apPass);
    setPowerMode((PowerMode)pMode);

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerApp::handleWeatherConfig() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    String loc = doc["loc"] | LOCATION_NAME;
    String lat = doc["lat"] | LATITUDE;
    String lon = doc["lon"] | LONGITUDE;
    String tz = doc["tz"] | TIMEZONE;
    WeatherManager::saveSettings(loc, lat, lon, tz);
    WeatherData wd;
    WeatherManager::fetchWeatherData(wd);
    setActiveRole(ROLE_WEATHER);
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerApp::handlePictureUpload() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        Serial.printf("[Picture] JSON parse error: %s\n", err.c_str());
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    const char* b64Data = doc["image"] | "";
    String caption = doc["caption"] | "";

    size_t slen = strlen(b64Data);
    if (slen == 0) {
        server.send(400, "application/json", "{\"error\":\"Empty image data\"}");
        return;
    }

    uint8_t decoded[4096];
    size_t olen = 0;
    int ret = mbedtls_base64_decode(decoded, sizeof(decoded), &olen, (const unsigned char*)b64Data, slen);
    if (ret != 0 || olen < 3800) {
        Serial.printf("[Picture] Base64 decode failed or incomplete: ret=%d, olen=%u\n", ret, (unsigned int)olen);
        server.send(400, "application/json", "{\"error\":\"Base64 decode failed\"}");
        return;
    }

    bool saved = PictureManager::saveBitmap(decoded, olen, caption);
    if (!saved) {
        Serial.println("[Picture] Failed to save bitmap to LittleFS");
        server.send(500, "application/json", "{\"error\":\"Failed to save to LittleFS\"}");
        return;
    }

    Serial.printf("[Picture] Successfully uploaded %u bytes image (caption: '%s')\n", (unsigned int)olen, caption.c_str());
    setActiveRole(ROLE_PICTURE);
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerApp::handlePictureCurrent() {
    if (!PictureManager::hasImage()) {
        server.send(200, "application/json", "{\"has_image\":false}");
        return;
    }

    uint8_t buffer[4096];
    size_t actualLen = 0;
    String caption = "";
    bool ok = PictureManager::loadBitmap(buffer, sizeof(buffer), actualLen, caption);
    if (!ok || actualLen < 3800) {
        server.send(200, "application/json", "{\"has_image\":false}");
        return;
    }

    size_t dlen = ((actualLen + 2) / 3) * 4 + 1;
    unsigned char* b64Buf = (unsigned char*)malloc(dlen);
    if (!b64Buf) {
        server.send(500, "application/json", "{\"error\":\"OOM\"}");
        return;
    }

    size_t olen = 0;
    mbedtls_base64_encode(b64Buf, dlen, &olen, buffer, actualLen);
    b64Buf[olen] = '\0';

    JsonDocument doc;
    doc["has_image"] = true;
    doc["image"] = (char*)b64Buf;
    doc["caption"] = caption;

    String resp;
    serializeJson(doc, resp);
    free(b64Buf);

    server.send(200, "application/json", resp);
}

void WebServerApp::handleCalendarAction() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    String action = doc["action"] | "next";
    int cat = doc["cat"] | 0;

    CalendarManager::setCategory(cat);
    if (action == "next") {
        CalendarManager::nextItem();
    } else if (action == "prev") {
        CalendarManager::prevItem();
    }

    CalendarItem item;
    CalendarManager::getCurrentItem(item);

    JsonDocument resp;
    resp["status"] = "ok";
    JsonObject io = resp["item"].to<JsonObject>();
    io["title"] = item.title;
    io["subtitle"] = item.subtitle;
    io["body"] = item.body;
    io["footer"] = item.footer;

    String respStr;
    serializeJson(resp, respStr);
    server.send(200, "application/json", respStr);

    refreshRequested = true;
}

void WebServerApp::handleBadgeConfig() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));

    BadgeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.subMode = doc["submode"] | 0;

    strncpy(cfg.name, doc["name"] | "", sizeof(cfg.name) - 1);
    strncpy(cfg.title, doc["title"] | "", sizeof(cfg.title) - 1);
    strncpy(cfg.company, doc["comp"] | "", sizeof(cfg.company) - 1);
    strncpy(cfg.handle, doc["handle"] | "", sizeof(cfg.handle) - 1);
    strncpy(cfg.qrUrl, doc["qr"] | "", sizeof(cfg.qrUrl) - 1);

    strncpy(cfg.phone, doc["phone"] | "", sizeof(cfg.phone) - 1);
    strncpy(cfg.email, doc["email"] | "", sizeof(cfg.email) - 1);
    strncpy(cfg.note, doc["note"] | "", sizeof(cfg.note) - 1);

    strncpy(cfg.statTitle, doc["stitle"] | "", sizeof(cfg.statTitle) - 1);
    strncpy(cfg.statSub, doc["ssub"] | "", sizeof(cfg.statSub) - 1);
    strncpy(cfg.statFoot, doc["sfoot"] | "", sizeof(cfg.statFoot) - 1);

    BadgeManager::updateConfig(cfg);
    setActiveRole(ROLE_BADGE);

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerApp::handleNewsConfig() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    int src = doc["source"] | 0;
    String sub = doc["subreddit"] | DEFAULT_REDDIT_SUB;
    String rss = doc["rss_url"] | DEFAULT_RSS_URL;

    NewsManager::saveSettings((NewsSource)src, sub, rss);
    NewsManager::fetchArticles();
    setActiveRole(ROLE_NEWS);

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerApp::handleNewsAction() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    String action = doc["action"] | "next";

    if (action == "next") {
        NewsManager::nextArticle();
    } else if (action == "prev") {
        NewsManager::prevArticle();
    } else if (action == "fetch") {
        NewsManager::fetchArticles();
    }

    NewsArticle art;
    NewsManager::getCurrentArticle(art);

    JsonDocument resp;
    resp["status"] = "ok";
    JsonObject ao = resp["article"].to<JsonObject>();
    ao["title"] = art.title;
    ao["url"] = art.url;
    ao["meta"] = art.meta;
    ao["source"] = art.sourceName;
    ao["index"] = art.index;
    ao["total"] = art.total;

    String respStr;
    serializeJson(resp, respStr);
    server.send(200, "application/json", respStr);

    refreshRequested = true;
}

void WebServerApp::handleRefreshDisplay() {
    refreshRequested = true;
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerApp::handleReboot() {
    server.send(200, "application/json", "{\"status\":\"rebooting\"}");
    delay(500);
    ESP.restart();
}

void WebServerApp::handleNotFound() {
    // Captive portal detection & redirect
    if (NetworkManager::isApMode()) {
        server.sendHeader("Location", String("http://") + NetworkManager::getIpAddress(), true);
        server.send(302, "text/plain", "");
        return;
    }
    server.send(404, "text/plain", "Not Found");
}
