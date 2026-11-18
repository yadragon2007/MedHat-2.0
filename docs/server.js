// Relay server: the ESP connects OUT to /esp, browsers connect to /ws,
// and every message from the ESP is forwarded to all browsers.
const path = require("path");
const http = require("http");
const express = require("express");
const { WebSocketServer, WebSocket } = require("ws");

const PORT = process.env.PORT || 3000;
const ESP_KEY = process.env.ESP_KEY || "dev-key";
if (!process.env.ESP_KEY) console.warn("WARNING: ESP_KEY not set, using the insecure default 'dev-key'");

const app = express();
app.use(express.static(path.join(__dirname, "public")));
app.get("/health", (_req, res) => res.send("ok"));

const server = http.createServer(app);
const viewers = new WebSocketServer({ noServer: true });
const esp = new WebSocketServer({ noServer: true });

server.on("upgrade", (req, socket, head) => {
  const url = new URL(req.url, "http://x");
  if (url.pathname === "/ws") {
    viewers.handleUpgrade(req, socket, head, ws => viewers.emit("connection", ws));
  } else if (url.pathname === "/esp" && url.searchParams.get("key") === ESP_KEY) {
    esp.handleUpgrade(req, socket, head, ws => esp.emit("connection", ws));
  } else {
    socket.write("HTTP/1.1 401 Unauthorized\r\n\r\n");
    socket.destroy();
  }
});

esp.on("connection", ws => {
  console.log("ESP connected");
  ws.on("message", (data, isBinary) => {
    if (isBinary) return;
    const text = data.toString();
    for (const v of viewers.clients) {
      if (v.readyState === WebSocket.OPEN && v.bufferedAmount < 1e6) v.send(text);
    }
  });
  ws.on("close", () => console.log("ESP disconnected"));
});

viewers.on("connection", () => console.log("viewer connected, total:", viewers.clients.size));

// Keep connections alive through proxies and drop dead ones
setInterval(() => {
  for (const group of [viewers, esp]) {
    for (const ws of group.clients) {
      if (ws.isAlive === false) { ws.terminate(); continue; }
      ws.isAlive = false;
      ws.ping();
    }
  }
}, 30000);
for (const group of [viewers, esp]) {
  group.on("connection", ws => { ws.isAlive = true; ws.on("pong", () => (ws.isAlive = true)); });
}

server.listen(PORT, () => console.log("Listening on port", PORT));
