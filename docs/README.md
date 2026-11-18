# Pacemaker dashboard – externally hosted

```
ESP32 ──(wss, outbound)──►  your server  ──(wss)──►  browsers
        /esp?key=SECRET      relays data              /ws
```

The ESP connects OUT to the server, so it works behind any home or school
router with no port forwarding. The ESP must be on a Wi-Fi network that has
internet access (not its own access point).

## Run locally (to test)

    npm install
    ESP_KEY=mysecret npm start        # PowerShell: $env:ESP_KEY="mysecret"; npm start

Open http://localhost:3000 (add ?demo to see fake data).

## Deploy

Any host that runs Node.js and supports WebSockets works (Render, Railway,
Fly.io, a VPS, ...). On all of them:

1. Put this folder in a GitHub repo and connect it to the host.
2. Build command: `npm install`   Start command: `npm start`
3. Set an environment variable `ESP_KEY` to a long random string.
4. Open the URL the host gives you. HTTPS is automatic and the page switches
   to wss:// by itself.

Free tiers may put the app to sleep when nobody visits. Open the page a minute
before you present, and check the host's current free-tier limits.

## ESP32 code (library: "WebSockets" by Markus Sattler, Links2004)

```cpp
#include <WiFi.h>
#include <WebSocketsClient.h>

WebSocketsClient ws;

void setup() {
  WiFi.begin("SSID", "PASSWORD");
  while (WiFi.status() != WL_CONNECTED) delay(300);

  // host WITHOUT https://, port 443, path with the key
  ws.beginSSL("your-app.onrender.com", 443, "/esp?key=YOUR_ESP_KEY");
  ws.setReconnectInterval(3000);
  ws.enableHeartbeat(15000, 3000, 2);
}

void loop() {
  ws.loop();                         // must run constantly

  static uint32_t last = 0;
  if (millis() - last >= 100 && ws.isConnected()) {   // 10 messages/s
    last = millis();
    char buf[160];
    snprintf(buf, sizeof(buf),
      "{\"t\":%lu,\"hr\":%.0f,\"pm\":%.2f,\"comb\":%.2f,\"temp\":%.1f,\"sweat\":%d}",
      millis(), heartBpm, pacemakerSignal, combinedSignal, tempC, sweatRaw);
    ws.sendTXT(buf);
  }
}
```

Replace `heartBpm`, `pacemakerSignal`, `combinedSignal`, `tempC` and `sweatRaw`
with your own variables. To send faster, collect several samples and send
them as one JSON array: `[{...},{...},{...}]`.

## Files

- `server.js` – relay + static file server
- `public/index.html`, `public/chart.umd.js` – the dashboard
- Anyone with the page URL can watch the graphs. Only a device that knows
  `ESP_KEY` can send data.
