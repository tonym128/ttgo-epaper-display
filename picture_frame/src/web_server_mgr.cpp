#include "web_server_mgr.h"
#include "config.h"
#include "storage_mgr.h"
#include <ArduinoJson.h>

static WebServer server(HTTP_PORT);
static bool triggerDisplay = false;
static String requestedPhoto = "";

bool WebServerManager::isTriggerDisplayRequested() {
    return triggerDisplay;
}

String WebServerManager::getRequestedPhotoToDisplay() {
    return requestedPhoto;
}

void WebServerManager::clearTriggerDisplay() {
    triggerDisplay = false;
    requestedPhoto = "";
}

bool WebServerManager::isAuthenticated() {
    if (server.hasHeader("Cookie")) {
        String cookie = server.header("Cookie");
        if (cookie.indexOf("picauth=1") != -1) {
            return true;
        }
    }
    return false;
}

// Embedded Web Portal SPA
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>E-Paper Picture Frame</title>
<style>
  :root {
    --bg: #0f172a; --card: #1e293b; --text: #f8fafc; --text-muted: #94a3b8;
    --accent: #38bdf8; --accent-hover: #0284c7; --danger: #ef4444; --border: #334155;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; background: var(--bg); color: var(--text); padding: 16px; max-width: 960px; margin: 0 auto; line-height: 1.5; }
  header { display: flex; justify-content: space-between; align-items: center; padding-bottom: 16px; border-bottom: 1px solid var(--border); margin-bottom: 24px; flex-wrap: wrap; gap: 12px; }
  h1 { font-size: 1.4rem; display: flex; align-items: center; gap: 8px; }
  .badge { background: #334155; padding: 4px 10px; border-radius: 9999px; font-size: 0.78rem; font-weight: 500; }
  .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 20px; margin-bottom: 24px; }
  .card { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 18px; }
  h2 { font-size: 1.1rem; margin-bottom: 14px; color: var(--accent); border-bottom: 1px solid var(--border); padding-bottom: 8px; }
  button { background: var(--accent); color: #000; border: none; padding: 8px 14px; border-radius: 6px; font-weight: 600; cursor: pointer; transition: 0.15s; }
  button:hover { background: var(--accent-hover); color: #fff; }
  button.danger { background: var(--danger); color: #fff; }
  button.secondary { background: #475569; color: #fff; }
  button.secondary:hover { background: #64748b; }
  input, select { background: #0f172a; border: 1px solid var(--border); color: #fff; padding: 8px 12px; border-radius: 6px; width: 100%; margin-top: 4px; margin-bottom: 12px; font-size: 0.9rem; }
  .preview-box { background: #e2e8f0; border-radius: 6px; padding: 12px; display: flex; flex-direction: column; align-items: center; justify-content: center; margin-bottom: 14px; min-height: 146px; }
  canvas { image-rendering: pixelated; border: 2px solid #64748b; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.3); }
  .controls-row { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 8px; }
  .range-label { display: flex; justify-content: space-between; font-size: 0.8rem; color: var(--text-muted); }
  .gallery-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(260px, 1fr)); gap: 16px; }
  .photo-card { background: #0f172a; border: 1px solid var(--border); border-radius: 8px; padding: 10px; display: flex; flex-direction: column; align-items: center; }
  .photo-title { font-size: 0.85rem; font-weight: 600; margin: 8px 0; max-width: 240px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
  .photo-actions { display: flex; gap: 6px; width: 100%; justify-content: center; }
  .photo-actions button { padding: 4px 8px; font-size: 0.75rem; }
  #loginModal { position: fixed; inset: 0; background: rgba(0,0,0,0.85); display: flex; align-items: center; justify-content: center; z-index: 99; }
  .modal-content { background: var(--card); border: 1px solid var(--border); padding: 24px; border-radius: 12px; width: 320px; text-align: center; }
  .hidden { display: none !important; }
</style>
</head>
<body>

<div id="loginModal" class="hidden">
  <div class="modal-content">
    <h2 style="border:none; margin-bottom:12px;">🔒 Picture Frame Login</h2>
    <p style="color:var(--text-muted); font-size:0.85rem; margin-bottom:14px;">Enter your admin password</p>
    <input type="password" id="loginPass" placeholder="Password (default: admin)">
    <button style="width:100%" onclick="doLogin()">Log In</button>
  </div>
</div>

<header>
  <h1>🖼️ LilyGo Picture Frame</h1>
  <div style="display:flex; gap:8px; align-items:center;">
    <span class="badge" id="storageBadge">Storage: --</span>
    <span class="badge" id="photoCountBadge">0 Photos</span>
    <button class="secondary" style="padding:4px 10px; font-size:0.8rem" onclick="logout()">Logout</button>
  </div>
</header>

<div class="grid">
  <!-- Upload & Live Dithering Studio -->
  <div class="card">
    <h2>🎨 Photo Studio & Upload</h2>
    <input type="file" id="fileInput" accept="image/*">
    
    <div class="preview-box">
      <canvas id="canvas" width="250" height="122"></canvas>
      <span style="color:#475569; font-size:0.75rem; margin-top:6px;">250×122 1-Bit E-Paper Preview</span>
    </div>

    <div class="controls-row">
      <div>
        <div class="range-label"><span>Brightness</span><span id="bVal">0</span></div>
        <input type="range" id="brightness" min="-100" max="100" value="0" oninput="processImage()">
      </div>
      <div>
        <div class="range-label"><span>Contrast</span><span id="cVal">0</span></div>
        <input type="range" id="contrast" min="-100" max="100" value="0" oninput="processImage()">
      </div>
    </div>

    <div class="controls-row">
      <div>
        <label style="font-size:0.8rem; color:var(--text-muted)">Dither Mode</label>
        <select id="ditherMode" onchange="processImage()">
          <option value="floyd">Floyd-Steinberg (Photos)</option>
          <option value="atkinson">Atkinson (Crisp)</option>
          <option value="threshold">Threshold (Text/Art)</option>
        </select>
      </div>
      <div style="display:flex; align-items:center; gap:8px; margin-top:20px;">
        <input type="checkbox" id="invertCheck" style="width:auto; margin:0;" onchange="processImage()">
        <label for="invertCheck" style="font-size:0.85rem">Invert Colors</label>
      </div>
    </div>

    <input type="text" id="photoName" placeholder="Photo Name (e.g. landscape.raw)">
    <button style="width:100%" onclick="uploadProcessedImage()">⬆️ Upload to Picture Frame</button>
  </div>

  <!-- Slideshow & Hardware Settings -->
  <div class="card">
    <h2>⚙️ Slideshow Settings</h2>
    
    <label style="font-size:0.85rem; color:var(--text-muted)">Refresh Interval</label>
    <select id="refreshInterval">
      <option value="1">Every 1 Minute (Testing)</option>
      <option value="5">Every 5 Minutes</option>
      <option value="15">Every 15 Minutes</option>
      <option value="30">Every 30 Minutes</option>
      <option value="60">Every 1 Hour</option>
      <option value="360">Every 6 Hours</option>
      <option value="720">Every 12 Hours</option>
      <option value="1440">Every 24 Hours</option>
    </select>

    <div style="display:flex; align-items:center; gap:8px; margin-bottom:14px;">
      <input type="checkbox" id="shuffleCheck" style="width:auto; margin:0;">
      <label for="shuffleCheck" style="font-size:0.85rem">Shuffle / Random Order</label>
    </div>

    <div style="display:flex; align-items:center; gap:8px; margin-bottom:14px;">
      <input type="checkbox" id="overlayCheck" style="width:auto; margin:0;">
      <label for="overlayCheck" style="font-size:0.85rem">Show subtle filename & battery bar</label>
    </div>

    <label style="font-size:0.85rem; color:var(--text-muted)">New Admin Password</label>
    <input type="password" id="newPass" placeholder="Leave blank to keep current">

    <button style="width:100%" onclick="saveSettings()">💾 Save Settings</button>
  </div>
</div>

<!-- Gallery -->
<div class="card">
  <h2>📚 Photo Playlist</h2>
  <div class="gallery-grid" id="gallery">
    <p style="color:var(--text-muted)">Loading photos...</p>
  </div>
</div>

<script>
let sourceImg = null;
let currentPhotos = [];

function checkAuth() {
  if (document.cookie.indexOf("picauth=1") === -1) {
    document.getElementById("loginModal").classList.remove("hidden");
  } else {
    document.getElementById("loginModal").classList.add("hidden");
    loadDashboard();
  }
}

async function doLogin() {
  const p = document.getElementById("loginPass").value;
  const res = await fetch("/api/login", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: "password=" + encodeURIComponent(p)
  });
  if (res.ok) {
    document.cookie = "picauth=1; path=/; max-age=86400";
    document.getElementById("loginModal").classList.add("hidden");
    loadDashboard();
  } else {
    alert("Incorrect password!");
  }
}

function logout() {
  document.cookie = "picauth=; path=/; max-age=0";
  location.reload();
}

async function loadDashboard() {
  try {
    const res = await fetch("/api/status");
    const st = await res.json();
    const usedKB = Math.round(st.storage_used / 1024);
    const totalKB = Math.round(st.storage_total / 1024);
    document.getElementById("storageBadge").innerText = `${usedKB} KB / ${totalKB} KB (${Math.round(st.storage_used/st.storage_total*100)}%)`;
    document.getElementById("photoCountBadge").innerText = `${st.photos_count} Photos`;
    document.getElementById("refreshInterval").value = st.refresh_interval;
    document.getElementById("shuffleCheck").checked = st.shuffle;
    document.getElementById("overlayCheck").checked = st.overlay;
    loadPhotos();
  } catch (e) {
    console.error(e);
  }
}

async function loadPhotos() {
  const g = document.getElementById("gallery");
  try {
    const res = await fetch("/api/photos");
    const list = await res.json();
    currentPhotos = list;
    if (list.length === 0) {
      g.innerHTML = `<p style="color:var(--text-muted); grid-column:1/-1;">No photos stored on SPIFFS yet. Upload one above!</p>`;
      return;
    }
    g.innerHTML = "";
    list.forEach((name, idx) => {
      const card = document.createElement("div");
      card.className = "photo-card";
      card.innerHTML = `
        <canvas width="250" height="122" id="thumb_${idx}"></canvas>
        <div class="photo-title">#${idx + 1}: ${name}</div>
        <div class="photo-actions">
          <button onclick="displayNow('${name}')">Display</button>
          <button class="secondary" onclick="movePhoto(${idx}, -1)" ${idx === 0 ? "disabled":""}>▲</button>
          <button class="secondary" onclick="movePhoto(${idx}, 1)" ${idx === list.length-1 ? "disabled":""}>▼</button>
          <button class="danger" onclick="deletePhoto('${name}')">🗑️</button>
        </div>
      `;
      g.appendChild(card);
      renderThumbnail(name, `thumb_${idx}`);
    });
  } catch (e) {
    g.innerHTML = `<p style="color:var(--danger)">Error loading gallery.</p>`;
  }
}

async function renderThumbnail(name, canvasId) {
  try {
    const res = await fetch(`/api/thumbnail?name=${encodeURIComponent(name)}`);
    if (!res.ok) return;
    const b64 = await res.text();
    const bin = atob(b64.trim());
    const bytes = new Uint8Array(bin.length);
    for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);

    const cvs = document.getElementById(canvasId);
    if (!cvs) return;
    const ctx = cvs.getContext("2d");
    const imgData = ctx.createImageData(250, 122);
    const d = imgData.data;

    let byteIdx = 0;
    for (let y = 0; y < 122; y++) {
      for (let byteInRow = 0; byteInRow < 32; byteInRow++) {
        const b = bytes[byteIdx++];
        for (let bit = 7; bit >= 0; bit--) {
          const x = byteInRow * 8 + (7 - bit);
          if (x < 250) {
            const isBlack = (b >> bit) & 1;
            const pxIdx = (y * 250 + x) * 4;
            const val = isBlack ? 0 : 255;
            d[pxIdx] = val;
            d[pxIdx+1] = val;
            d[pxIdx+2] = val;
            d[pxIdx+3] = 255;
          }
        }
      }
    }
    ctx.putImageData(imgData, 0, 0);
  } catch (e) {
    console.error(e);
  }
}

async function displayNow(name) {
  await fetch(`/api/display?name=${encodeURIComponent(name)}`, { method: "POST" });
  alert(`Triggered refresh for ${name}!`);
}

async function deletePhoto(name) {
  if (!confirm(`Delete ${name}?`)) return;
  await fetch(`/api/delete?name=${encodeURIComponent(name)}`, { method: "POST" });
  loadDashboard();
}

async function movePhoto(idx, dir) {
  const newIdx = idx + dir;
  if (newIdx < 0 || newIdx >= currentPhotos.length) return;
  const temp = currentPhotos[idx];
  currentPhotos[idx] = currentPhotos[newIdx];
  currentPhotos[newIdx] = temp;
  await fetch("/api/reorder", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(currentPhotos)
  });
  loadPhotos();
}

async function saveSettings() {
  const payload = {
    refresh_interval: parseInt(document.getElementById("refreshInterval").value),
    shuffle: document.getElementById("shuffleCheck").checked,
    overlay: document.getElementById("overlayCheck").checked,
    admin_password: document.getElementById("newPass").value
  };
  await fetch("/api/settings", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload)
  });
  alert("Settings saved!");
  document.getElementById("newPass").value = "";
}

// Canvas & Dithering Engine
document.getElementById("fileInput").addEventListener("change", function(e) {
  const file = e.target.files[0];
  if (!file) return;
  document.getElementById("photoName").value = file.name.replace(/\.[^/.]+$/, "") + ".raw";
  const reader = new FileReader();
  reader.onload = function(evt) {
    sourceImg = new Image();
    sourceImg.onload = function() { processImage(); };
    sourceImg.src = evt.target.result;
  };
  reader.readAsDataURL(file);
});

function processImage() {
  if (!sourceImg) return;
  const canvas = document.getElementById("canvas");
  const ctx = canvas.getContext("2d");
  const W = 250, H = 122;

  // Aspect ratio crop / fit
  const imgRatio = sourceImg.width / sourceImg.height;
  const targetRatio = W / H;
  let sx, sy, sw, sh;
  if (imgRatio > targetRatio) {
    sh = sourceImg.height;
    sw = sh * targetRatio;
    sx = (sourceImg.width - sw) / 2;
    sy = 0;
  } else {
    sw = sourceImg.width;
    sh = sw / targetRatio;
    sx = 0;
    sy = (sourceImg.height - sh) / 2;
  }

  ctx.drawImage(sourceImg, sx, sy, sw, sh, 0, 0, W, H);
  const imgData = ctx.getImageData(0, 0, W, H);
  const d = imgData.data;

  const brightness = parseInt(document.getElementById("brightness").value);
  const contrast = parseInt(document.getElementById("contrast").value);
  document.getElementById("bVal").innerText = brightness;
  document.getElementById("cVal").innerText = contrast;

  const factor = (259 * (contrast + 255)) / (255 * (259 - contrast));
  const invert = document.getElementById("invertCheck").checked;
  const mode = document.getElementById("ditherMode").value;

  // Convert to grayscale with contrast/brightness adjustments
  const gray = new Float32Array(W * H);
  for (let i = 0; i < W * H; i++) {
    const idx = i * 4;
    let r = d[idx], g = d[idx+1], b = d[idx+2];
    let lum = 0.299 * r + 0.587 * g + 0.114 * b;
    lum = factor * (lum - 128) + 128 + brightness;
    if (lum < 0) lum = 0;
    if (lum > 255) lum = 255;
    gray[i] = lum;
  }

  // Dithering
  for (let y = 0; y < H; y++) {
    for (let x = 0; x < W; x++) {
      const idx = y * W + x;
      const oldVal = gray[idx];
      let newVal = oldVal < 128 ? 0 : 255;
      if (mode === "threshold") {
        gray[idx] = newVal;
        continue;
      }
      const err = oldVal - newVal;
      gray[idx] = newVal;

      if (mode === "floyd") {
        if (x + 1 < W) gray[idx + 1] += err * (7/16);
        if (x - 1 >= 0 && y + 1 < H) gray[idx + W - 1] += err * (3/16);
        if (y + 1 < H) gray[idx + W] += err * (5/16);
        if (x + 1 < W && y + 1 < H) gray[idx + W + 1] += err * (1/16);
      } else if (mode === "atkinson") {
        if (x + 1 < W) gray[idx + 1] += err * (1/8);
        if (x + 2 < W) gray[idx + 2] += err * (1/8);
        if (x - 1 >= 0 && y + 1 < H) gray[idx + W - 1] += err * (1/8);
        if (y + 1 < H) gray[idx + W] += err * (1/8);
        if (x + 1 < W && y + 1 < H) gray[idx + W + 1] += err * (1/8);
        if (y + 2 < H) gray[idx + 2*W] += err * (1/8);
      }
    }
  }

  // Output back to canvas
  for (let i = 0; i < W * H; i++) {
    let v = gray[i] < 128 ? 0 : 255;
    if (invert) v = 255 - v;
    const idx = i * 4;
    d[idx] = v; d[idx+1] = v; d[idx+2] = v; d[idx+3] = 255;
  }
  ctx.putImageData(imgData, 0, 0);
}

function getRawBinaryFromCanvas() {
  const canvas = document.getElementById("canvas");
  const ctx = canvas.getContext("2d");
  const imgData = ctx.getImageData(0, 0, 250, 122).data;
  const rawBytes = new Uint8Array(32 * 122); // 3904 bytes

  let byteIdx = 0;
  for (let y = 0; y < 122; y++) {
    for (let byteInRow = 0; byteInRow < 32; byteInRow++) {
      let b = 0;
      for (let bit = 7; bit >= 0; bit--) {
        const x = byteInRow * 8 + (7 - bit);
        if (x < 250) {
          const pxIdx = (y * 250 + x) * 4;
          // In Adafruit GFX drawBitmap: bit=1 is black, bit=0 is white
          const isBlack = imgData[pxIdx] < 128;
          if (isBlack) b |= (1 << bit);
        }
      }
      rawBytes[byteIdx++] = b;
    }
  }
  return rawBytes;
}

async function uploadProcessedImage() {
  if (!sourceImg) {
    alert("Please select an image first!");
    return;
  }
  let name = document.getElementById("photoName").value.trim();
  if (!name) name = "photo_" + Date.now() + ".raw";
  if (!name.endsWith(".raw")) name += ".raw";

  let binary = "";
  for (let i = 0; i < rawBytes.byteLength; i++) {
    binary += String.fromCharCode(rawBytes[i]);
  }
  const b64 = btoa(binary);

  try {
    const res = await fetch("/api/upload", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ name: name, data: b64 })
    });
    if (res.ok) {
      alert(`"${name}" uploaded successfully!`);
      loadDashboard();
    } else {
      const err = await res.json();
      alert("Upload failed: " + (err.error || "Unknown error"));
    }
  } catch (e) {
    alert("Upload failed: " + e);
  }
}

checkAuth();
</script>
</body>
</html>
)rawliteral";

void WebServerManager::handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

void WebServerManager::handleLogin() {
    if (server.hasArg("password")) {
        String pass = server.arg("password");
        if (pass == StorageManager::config.admin_password) {
            server.sendHeader("Set-Cookie", "picauth=1; Path=/; Max-Age=86400");
            server.send(200, "application/json", "{\"success\":true}");
            return;
        }
    }
    server.send(401, "application/json", "{\"error\":\"Invalid password\"}");
}

void WebServerManager::handleGetStatus() {
    if (!isAuthenticated()) {
        server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }
    size_t used = 0, total = 0;
    StorageManager::getStorageStats(used, total);

    JsonDocument doc;
    doc["storage_used"] = used;
    doc["storage_total"] = total;
    doc["photos_count"] = StorageManager::config.photo_list.size();
    doc["current_index"] = StorageManager::config.current_index;
    doc["refresh_interval"] = StorageManager::config.refresh_interval_minutes;
    doc["shuffle"] = StorageManager::config.shuffle;
    doc["overlay"] = StorageManager::config.show_status_overlay;

    String res;
    serializeJson(doc, res);
    server.send(200, "application/json", res);
}

void WebServerManager::handleGetPhotos() {
    if (!isAuthenticated()) {
        server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& p : StorageManager::config.photo_list) {
        arr.add(p);
    }
    String res;
    serializeJson(doc, res);
    server.send(200, "application/json", res);
}

#include <mbedtls/base64.h>

void WebServerManager::handleUpload() {
    if (!isAuthenticated()) {
        server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }
    String body = server.arg("plain");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    String filename = doc["name"] | "";
    String b64 = doc["data"] | "";

    if (filename.length() == 0 || b64.length() == 0) {
        server.send(400, "application/json", "{\"error\":\"Missing name or data\"}");
        return;
    }

    if (!filename.endsWith(".raw")) filename += ".raw";

    static uint8_t decoded[RAW_IMAGE_SIZE];
    size_t outLen = 0;
    int res = mbedtls_base64_decode(decoded, RAW_IMAGE_SIZE, &outLen, (const unsigned char*)b64.c_str(), b64.length());
    if (res != 0 || outLen != RAW_IMAGE_SIZE) {
        Serial.printf("Base64 decode error: res=%d, outLen=%u, expected=%u\n", res, outLen, RAW_IMAGE_SIZE);
        server.send(400, "application/json", "{\"error\":\"Image decode failed\"}");
        return;
    }

    if (StorageManager::savePhoto(filename, decoded, outLen)) {
        Serial.printf("Photo %s saved successfully (%u bytes)\n", filename.c_str(), outLen);
        requestedPhoto = filename;
        triggerDisplay = true;
        server.send(200, "application/json", "{\"success\":true}");
    } else {
        server.send(500, "application/json", "{\"error\":\"Failed to save to storage\"}");
    }
}

void WebServerManager::handleDelete() {
    if (!isAuthenticated()) {
        server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }
    if (!server.hasArg("name")) {
        server.send(400, "application/json", "{\"error\":\"Missing photo name\"}");
        return;
    }
    String name = server.arg("name");
    StorageManager::deletePhoto(name);
    server.send(200, "application/json", "{\"success\":true}");
}

void WebServerManager::handleDisplayNow() {
    if (!isAuthenticated()) {
        server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }
    if (!server.hasArg("name")) {
        server.send(400, "application/json", "{\"error\":\"Missing photo name\"}");
        return;
    }
    requestedPhoto = server.arg("name");
    triggerDisplay = true;
    server.send(200, "application/json", "{\"success\":true}");
}

void WebServerManager::handleReorder() {
    if (!isAuthenticated()) {
        server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }
    String body = server.arg("plain");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    std::vector<String> newOrder;
    JsonArray arr = doc.as<JsonArray>();
    for (JsonVariant v : arr) {
        newOrder.push_back(v.as<String>());
    }
    StorageManager::config.photo_list = newOrder;
    StorageManager::saveConfig(StorageManager::config);
    server.send(200, "application/json", "{\"success\":true}");
}

void WebServerManager::handleSaveSettings() {
    if (!isAuthenticated()) {
        server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }
    String body = server.arg("plain");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    StorageManager::config.refresh_interval_minutes = doc["refresh_interval"] | DEFAULT_REFRESH_MINUTES;
    StorageManager::config.shuffle = doc["shuffle"] | false;
    StorageManager::config.show_status_overlay = doc["overlay"] | false;
    String newPass = doc["admin_password"] | "";
    if (newPass.length() > 0) {
        StorageManager::config.admin_password = newPass;
    }
    StorageManager::saveConfig(StorageManager::config);
    server.send(200, "application/json", "{\"success\":true}");
}

void WebServerManager::handleThumbnail() {
    if (!isAuthenticated()) {
        server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }
    if (!server.hasArg("name")) {
        server.send(400, "text/plain", "Missing name");
        return;
    }
    String name = server.arg("name");
    static uint8_t buf[RAW_IMAGE_SIZE];
    if (StorageManager::readPhoto(name, buf, RAW_IMAGE_SIZE)) {
        size_t b64Len = 0;
        char b64Out[5400];
        mbedtls_base64_encode((unsigned char*)b64Out, sizeof(b64Out), &b64Len, buf, RAW_IMAGE_SIZE);
        b64Out[b64Len] = '\0';
        server.send(200, "text/plain", b64Out);
    } else {
        server.send(404, "text/plain", "Not found");
    }
}

void WebServerManager::init() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/login", HTTP_POST, handleLogin);
    server.on("/api/status", HTTP_GET, handleGetStatus);
    server.on("/api/photos", HTTP_GET, handleGetPhotos);
    server.on("/api/upload", HTTP_POST, handleUpload);
    server.on("/api/delete", HTTP_POST, handleDelete);
    server.on("/api/display", HTTP_POST, handleDisplayNow);
    server.on("/api/reorder", HTTP_POST, handleReorder);
    server.on("/api/settings", HTTP_POST, handleSaveSettings);
    server.on("/api/thumbnail", HTTP_GET, handleThumbnail);

    // Collect Cookie header for auth
    const char* headerkeys[] = {"Cookie"};
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
    server.collectHeaders(headerkeys, headerkeyssize);

    server.begin();
    Serial.println("HTTP Web Server started on port 80");
}

void WebServerManager::handleClient() {
    server.handleClient();
}
