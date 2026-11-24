import { memo, useEffect, useRef, useState } from "react";
import {
  Chart,
  LineController,
  LineElement,
  PointElement,
  LinearScale,
} from "chart.js";

Chart.register(LineController, LineElement, PointElement, LinearScale);

/* ============================================================
   CONFIG – edit this part to match your ESP
   ============================================================ */
const CONFIG = {
  // WebSocket path on the host that served the page.
  // Override the host without editing: ?host=192.168.4.1
  wsPath: "/ws",
  windowSeconds: 30, // how much history each graph shows
  maxFps: 30, // redraw rate limit

  // One entry per graph. `key` is the JSON field the ESP sends.
  charts: [
    { key: "hr", title: "Heart rate", sub: "Unhealthy heart, no pacemaker",
      unit: "bpm", color: "#c0392b", wide: true, decimals: 0 },
    { key: "pm", title: "Pacemaker only", sub: "Isolated pacemaker output",
      unit: "", color: "#1f7a8c", decimals: 2 },
    { key: "comb", title: "Combined", sub: "Heart + pacemaker",
      unit: "", color: "#7b4fa3", decimals: 2 },
    { key: "temp", title: "Temperature", sub: "Temperature sensor",
      unit: "°C", color: "#d27a12", decimals: 1 },
    { key: "sweat", title: "Sweat", sub: "Sweat sensor",
      unit: "", color: "#2f6fcf", decimals: 0 },
  ],
};

const cssVar = (name) =>
  getComputedStyle(document.documentElement).getPropertyValue(name).trim();

/* ============================================================
   One card = title + latest value + one Chart.js graph
   ============================================================ */
const ChartCard = memo(function ChartCard({ cfg, panels, clock }) {
  const canvasRef = useRef(null);
  const valueRef = useRef(null);

  useEffect(() => {
    const data = [];
    const chart = new Chart(canvasRef.current, {
      type: "line",
      data: {
        datasets: [
          {
            data,
            borderColor: cfg.color,
            backgroundColor: cfg.color,
            borderWidth: 2,
            pointRadius: 0,
            tension: 0,
            spanGaps: false,
          },
        ],
      },
      options: {
        parsing: false,
        normalized: true,
        animation: false,
        maintainAspectRatio: false,
        scales: {
          x: {
            type: "linear",
            ticks: {
              maxTicksLimit: 6,
              color: cssVar("--muted"),
              callback: (v) => (v - clock.current.xNow).toFixed(0) + "s",
            },
            grid: { display: false },
            border: { color: cssVar("--line") },
          },
          y: {
            ...(cfg.min !== undefined ? { min: cfg.min } : {}),
            ...(cfg.max !== undefined ? { max: cfg.max } : {}),
            grace: "10%",
            ticks: { color: cssVar("--muted"), maxTicksLimit: 5 },
            grid: { color: cssVar("--grid") },
            border: { display: false },
          },
        },
      },
    });

    panels.current[cfg.key] = { cfg, chart, data, valueEl: valueRef.current };
    return () => {
      chart.destroy();
      delete panels.current[cfg.key];
    };
  }, [cfg, panels, clock]);

  return (
    <section className={"card" + (cfg.wide ? " wide" : "")}>
      <div className="card-head">
        <div>
          <h2>{cfg.title}</h2>
          <p className="sub">{cfg.sub}</p>
        </div>
        <div className="value">
          <span ref={valueRef}>--</span>
          <small>{cfg.unit}</small>
        </div>
      </div>
      <div className="chart-box">
        <canvas ref={canvasRef} />
      </div>
    </section>
  );
});

/* ============================================================
   App
   ============================================================ */
export default function App() {
  // Live data lives in refs, not React state, so 25+ updates per second
  // never trigger React re-renders.
  const panels = useRef({}); // key -> { cfg, chart, data, valueEl }
  const clock = useRef({ xNow: 0, t0: null }); // right edge of every graph (s)
  const pausedRef = useRef(false);
  const dirtyRef = useRef(false);

  const [paused, setPaused] = useState(false);
  const [status, setStatus] = useState({ state: "", text: "Connecting…" });
  const [theme, setTheme] = useState(() => {
    try {
      const saved = localStorage.getItem("theme");
      if (saved) return saved;
    } catch {}
    return matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
  });

  /* ---------- dark / light mode ---------- */
  useEffect(() => {
    document.documentElement.dataset.theme = theme;
    try { localStorage.setItem("theme", theme); } catch {}
    for (const key in panels.current) {
      const chart = panels.current[key].chart;
      const s = chart.options.scales;
      s.x.ticks.color = cssVar("--muted");
      s.x.border.color = cssVar("--line");
      s.y.ticks.color = cssVar("--muted");
      s.y.grid.color = cssVar("--grid");
      chart.update("none");
    }
  }, [theme]);

  /* ---------- data, drawing loop, WebSocket, demo mode ---------- */
  useEffect(() => {
    const params = new URLSearchParams(location.search);
    const clk = clock.current;
    let ws, retryTimer, demoTimer, raf;
    let closed = false, retry = 0, lastDraw = 0;

    const xFromSample = (s) => {
      if (typeof s.t === "number") {
        if (clk.t0 === null) clk.t0 = s.t;
        return (s.t - clk.t0) / 1000;
      }
      return performance.now() / 1000; // fallback: arrival time
    };

    const addSample = (s) => {
      if (!s || typeof s !== "object") return;
      const x = xFromSample(s);
      if (x > clk.xNow) clk.xNow = x;
      for (const key in panels.current) {
        const v = s[key];
        if (typeof v !== "number" || !isFinite(v)) continue;
        const p = panels.current[key];
        p.data.push({ x, y: v });
        p.valueEl.textContent = v.toFixed(p.cfg.decimals);
      }
      dirtyRef.current = true;
    };

    const onMessage = (text) => {
      let msg;
      try { msg = JSON.parse(text); } catch { return; }
      if (Array.isArray(msg)) msg.forEach(addSample);
      else addSample(msg);
    };

    const trimAndDraw = () => {
      const minX = clk.xNow - CONFIG.windowSeconds;
      for (const key in panels.current) {
        const { data, chart } = panels.current[key];
        let drop = 0;
        while (drop < data.length && data[drop].x < minX - 1) drop++;
        if (drop) data.splice(0, drop);
        chart.options.scales.x.min = minX;
        chart.options.scales.x.max = clk.xNow;
        chart.update("none");
      }
    };

    const loop = (now) => {
      if (dirtyRef.current && !pausedRef.current && now - lastDraw >= 1000 / CONFIG.maxFps) {
        lastDraw = now;
        dirtyRef.current = false;
        trimAndDraw();
      }
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);

    const connect = () => {
      const host = params.get("host") || location.host;
      const proto = location.protocol === "https:" ? "wss://" : "ws://";
      setStatus({ state: "", text: "Connecting…" });
      ws = new WebSocket(proto + host + CONFIG.wsPath);
      ws.onopen = () => {
        retry = 0;
        setStatus({ state: "on", text: "Live" });
      };
      ws.onmessage = (e) => {
        if (!pausedRef.current) onMessage(e.data);
      };
      ws.onclose = () => {
        if (closed) return;
        setStatus({ state: "off", text: "Disconnected – retrying…" });
        retryTimer = setTimeout(connect, Math.min(1000 * 2 ** retry++, 5000));
      };
      ws.onerror = () => ws.close();
    };

    // Demo mode: open the page with ?demo to test without an ESP
    const startDemo = () => {
      setStatus({ state: "on", text: "Demo data (no ESP)" });
      const start = performance.now();
      demoTimer = setInterval(() => {
        if (pausedRef.current) return;
        const t = performance.now() - start;
        const s = t / 1000;
        const bpm = 48 + 6 * Math.sin(s / 3) + (Math.random() - 0.5) * 2;
        const beat = Math.sin(s * 2 * Math.PI * 1.2) > 0.97 ? 1 : 0;
        const pm = Math.sin(s * 2 * Math.PI * 1.1) > 0.97 ? 1 : 0;
        onMessage(
          JSON.stringify({
            t,
            hr: bpm,
            pm: pm + (Math.random() - 0.5) * 0.03,
            comb: beat * 0.8 + pm * 0.6 + (Math.random() - 0.5) * 0.03,
            temp: 36.6 + 0.3 * Math.sin(s / 7) + (Math.random() - 0.5) * 0.05,
            sweat: 300 + 40 * Math.sin(s / 5) + Math.random() * 6,
          })
        );
      }, 40);
    };

    if (params.has("demo")) startDemo();
    else connect();

    return () => {
      closed = true;
      cancelAnimationFrame(raf);
      clearTimeout(retryTimer);
      clearInterval(demoTimer);
      if (ws) ws.close();
    };
  }, []);

  /* ---------- buttons ---------- */
  const togglePause = () => {
    const next = !paused;
    pausedRef.current = next;
    setPaused(next);
    if (!next) dirtyRef.current = true;
  };

  const clearAll = () => {
    for (const key in panels.current) {
      panels.current[key].data.length = 0;
      panels.current[key].valueEl.textContent = "--";
    }
    dirtyRef.current = true;
  };

  return (
    <>
      <header>
        <h1>Pacemaker Monitor</h1>
        <div className="status" role="status" aria-live="polite">
          <span className={"dot " + status.state} />
          <span>{status.text}</span>
        </div>
        <div className="tools">
          <button type="button" onClick={() => setTheme((t) => (t === "dark" ? "light" : "dark"))}>
            {theme === "dark" ? "Light mode" : "Dark mode"}
          </button>
          <button type="button" onClick={togglePause}>
            {paused ? "Resume" : "Pause"}
          </button>
          <button type="button" onClick={clearAll}>
            Clear
          </button>
        </div>
      </header>

      <main>
        {CONFIG.charts.map((cfg) => (
          <ChartCard key={cfg.key} cfg={cfg} panels={panels} clock={clock} />
        ))}
      </main>
    </>
  );
}