const { createApp } = Vue;

createApp({
  data() {
    return {
      selectedChannel: 0,
      rawScpi: "",
      logs: [],
      wsConnected: false,
      ws: null,
      wsRetryTimer: null,
      editingSetpoint: false,
      graphHistory: [[], []], // ring buffer per channel: [{curr, volt}, ...], max 120 samples
      GRAPH_MAX: 120,
      channels: [
        {
          id: 1,
          mode: "CC",
          outputEnabled: false,
          measuredVoltage: 0.0,
          measuredCurrent: 0.0,
          measuredPower: 0.0,
          cvSetpoint: 0.0,
          ccSetpoint: 0.0,
          cpSetpoint: 0.0,
          crSetpoint: 0.0,
          setpointValue: 0.0,
          uvCutoffEnabled: false,
          uvCutoffValue: 0.0,
        },
        {
          id: 2,
          mode: "CC",
          outputEnabled: false,
          measuredVoltage: 0.0,
          measuredCurrent: 0.0,
          measuredPower: 0.0,
          cvSetpoint: 0.0,
          ccSetpoint: 0.0,
          cpSetpoint: 0.0,
          crSetpoint: 0.0,
          setpointValue: 0.0,
          uvCutoffEnabled: false,
          uvCutoffValue: 0.0,
        },
      ],
    };
  },
  computed: {
    activeChannel() {
      return this.channels[this.selectedChannel];
    },
  },
  mounted() {
    this.channels.forEach((channel) => {
      channel.setpointValue = this.getSetpointByMode(channel, channel.mode);
    });
    this.connectWebSocket();
  },
  beforeUnmount() {
    if (this.wsRetryTimer) {
      clearTimeout(this.wsRetryTimer);
      this.wsRetryTimer = null;
    }
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
  },
  watch: {
    selectedChannel() {
      this.$nextTick(() => this.drawGraph());
    },
  },
  methods: {
    addLog(cmd, ok) {
      const now = new Date();
      const time = now.toTimeString().slice(0, 8);
      this.logs.unshift({ time, cmd, ok });
      if (this.logs.length > 60) {
        this.logs.length = 60;
      }
    },
    modeFromNumber(mode) {
      if (mode === 0) return "CV";
      if (mode === 1) return "CC";
      if (mode === 2) return "CP";
      if (mode === 3) return "CR";
      return "CC";
    },
    setModeAndRefreshSetpoint(ch, modeName) {
      ch.mode = modeName;
      if (!this.editingSetpoint) {
        ch.setpointValue = this.getSetpointByMode(ch, ch.mode);
      }
    },
    connectWebSocket() {
      if (this.ws) {
        this.ws.close();
        this.ws = null;
      }

      const protocol = window.location.protocol === "https:" ? "wss" : "ws";
      const wsUrl = `${protocol}://${window.location.host}/ws`;
      const socket = new WebSocket(wsUrl);
      this.ws = socket;

      socket.onopen = () => {
        this.wsConnected = true;
      };

      socket.onmessage = (event) => {
        let payload;
        try {
          payload = JSON.parse(event.data);
        } catch (err) {
          return;
        }
        this.applyWsPayload(payload);
      };

      socket.onclose = () => {
        this.wsConnected = false;
        if (this.wsRetryTimer) clearTimeout(this.wsRetryTimer);
        this.wsRetryTimer = setTimeout(() => this.connectWebSocket(), 1500);
      };

      socket.onerror = () => {
        this.wsConnected = false;
      };
    },
    applyWsPayload(payload) {
      if (!payload || typeof payload.channel !== "number") return;
      const ch = this.channels[payload.channel];
      if (!ch) return;

      if (payload.type === "measurement") {
        if (typeof payload.voltage === "number") ch.measuredVoltage = payload.voltage;
        if (typeof payload.current === "number") {
          ch.measuredCurrent = payload.current;
          this.pushGraph(payload.channel, ch.measuredVoltage, payload.current);
        }
        if (typeof payload.power === "number") ch.measuredPower = payload.power;
        if (typeof payload.outputEnabled === "number") ch.outputEnabled = payload.outputEnabled !== 0;
        if (typeof payload.mode === "number") this.setModeAndRefreshSetpoint(ch, this.modeFromNumber(payload.mode));
        return;
      }

      if (payload.type !== "state") return;

      const value = Number(payload.value);
      switch (payload.cmd) {
        case "OUTPUT_STATE":
          ch.outputEnabled = value !== 0;
          break;
        case "SET_MODE":
          this.setModeAndRefreshSetpoint(ch, this.modeFromNumber(Math.round(value)));
          break;
        case "SET_VOLTAGE":
          ch.cvSetpoint = value;
          if (ch.mode === "CV" && !this.editingSetpoint) ch.setpointValue = value;
          break;
        case "SET_CURRENT":
          ch.ccSetpoint = value;
          if (ch.mode === "CC" && !this.editingSetpoint) ch.setpointValue = value;
          break;
        case "SET_POWER":
          ch.cpSetpoint = value;
          if (ch.mode === "CP" && !this.editingSetpoint) ch.setpointValue = value;
          break;
        case "SET_RESISTANCE":
          ch.crSetpoint = value;
          if (ch.mode === "CR" && !this.editingSetpoint) ch.setpointValue = value;
          break;
        case "SET_LOW_VOLTAGE_PROTECTION":
          ch.uvCutoffEnabled = value > 0 && value < 998;
          if (ch.uvCutoffEnabled) ch.uvCutoffValue = value;
          break;
        case "MEAS_VOLT":
          ch.measuredVoltage = value;
          break;
        case "MEAS_CURR":
          ch.measuredCurrent = value;
          ch.measuredPower = ch.measuredVoltage * ch.measuredCurrent;
          this.pushGraph(payload.channel, ch.measuredVoltage, value);
          break;
        default:
          break;
      }
    },
    async sendScpi(cmd) {
      const response = await fetch("/api/scpi", {
        method: "POST",
        headers: {
          "Content-Type": "text/plain",
        },
        body: cmd,
      });

      if (!response.ok) {
        let reason = "";
        try {
          reason = await response.text();
        } catch (err) {
          reason = "request failed";
        }
        throw new Error(reason || "request failed");
      }
    },
    pushGraph(chIdx, volt, curr) {
      const buf = this.graphHistory[chIdx];
      buf.push({ volt, curr });
      if (buf.length > this.GRAPH_MAX) buf.shift();
      if (chIdx === this.selectedChannel) this.drawGraph();
    },
    drawGraph() {
      const canvas = this.$refs.graphCanvas;
      if (!canvas) return;
      const buf = this.graphHistory[this.selectedChannel];
      const W = (canvas.width = canvas.offsetWidth);
      const H = (canvas.height = canvas.offsetHeight);
      const ctx = canvas.getContext("2d");
      ctx.clearRect(0, 0, W, H);
      if (buf.length < 2) return;

      const maxCurr = Math.max(...buf.map((p) => p.curr), 0.001);
      const maxVolt = Math.max(...buf.map((p) => p.volt), 0.001);
      const pad = { t: 8, r: 52, b: 20, l: 52 };
      const gW = W - pad.l - pad.r;
      const gH = H - pad.t - pad.b;
      const ticks = 4;

      // grid lines
      ctx.lineWidth = 1;
      ctx.strokeStyle = "#d6ddd7";
      for (let i = 0; i <= ticks; i++) {
        const y = pad.t + gH * (1 - i / ticks);
        ctx.beginPath();
        ctx.moveTo(pad.l, y);
        ctx.lineTo(pad.l + gW, y);
        ctx.stroke();
      }

      // left axis — current (green)
      ctx.fillStyle = "#1f6f5f";
      ctx.font = "11px 'IBM Plex Mono', monospace";
      ctx.textAlign = "right";
      for (let i = 0; i <= ticks; i++) {
        const y = pad.t + gH * (1 - i / ticks);
        ctx.fillText(((maxCurr * i) / ticks).toFixed(3) + "A", pad.l - 4, y + 4);
      }

      // right axis — voltage (orange)
      ctx.fillStyle = "#ef8a17";
      ctx.textAlign = "left";
      for (let i = 0; i <= ticks; i++) {
        const y = pad.t + gH * (1 - i / ticks);
        ctx.fillText(((maxVolt * i) / ticks).toFixed(2) + "V", pad.l + gW + 4, y + 4);
      }

      // sample count
      ctx.fillStyle = "#5d6a64";
      ctx.textAlign = "center";
      ctx.fillText(buf.length + " samples", pad.l + gW / 2, H - 4);

      const drawLine = (color, fillColor, getValue, maxVal) => {
        ctx.strokeStyle = color;
        ctx.lineWidth = 2;
        ctx.lineJoin = "round";
        ctx.beginPath();
        buf.forEach((p, i) => {
          const x = pad.l + (i / (buf.length - 1)) * gW;
          const y = pad.t + gH * (1 - getValue(p) / maxVal);
          i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
        });
        ctx.stroke();
        const last = buf[buf.length - 1];
        const lx = pad.l + gW;
        const ly = pad.t + gH * (1 - getValue(last) / maxVal);
        ctx.lineTo(lx, pad.t + gH);
        ctx.lineTo(pad.l, pad.t + gH);
        ctx.closePath();
        ctx.fillStyle = fillColor;
        ctx.fill();
      };

      drawLine("#1f6f5f", "rgba(31,111,95,0.10)", (p) => p.curr, maxCurr);
      drawLine("#ef8a17", "rgba(239,138,23,0.08)", (p) => p.volt, maxVolt);
    },
    setpointFieldLabel(mode) {
      if (mode === "CV") return "Set Voltage (V)";
      if (mode === "CC") return "Set Current (A)";
      if (mode === "CP") return "Set Power (W)";
      if (mode === "CR") return "Set Resistance (Ohm)";
      return "Setpoint";
    },
    getSetpointByMode(ch, mode) {
      if (mode === "CV") return ch.cvSetpoint;
      if (mode === "CC") return ch.ccSetpoint;
      if (mode === "CP") return ch.cpSetpoint;
      if (mode === "CR") return ch.crSetpoint;
      return 0;
    },
    setSetpointByMode(ch, mode, value) {
      if (mode === "CV") ch.cvSetpoint = value;
      else if (mode === "CC") ch.ccSetpoint = value;
      else if (mode === "CP") ch.cpSetpoint = value;
      else if (mode === "CR") ch.crSetpoint = value;
    },
    setpointLabel(ch) {
      if (ch.mode === "CV") return `${Number(ch.cvSetpoint).toFixed(2)} V`;
      if (ch.mode === "CC") return `${Number(ch.ccSetpoint).toFixed(3)} A`;
      if (ch.mode === "CP") return `${Number(ch.cpSetpoint).toFixed(2)} W`;
      if (ch.mode === "CR") return `${Number(ch.crSetpoint).toFixed(2)} Ohm`;
      return "--";
    },
    async toggleOutput(index) {
      const ch = this.channels[index];
      const next = !ch.outputEnabled;
      const cmd = `OUTP${ch.id}:STAT ${next ? "ON" : "OFF"}`;

      try {
        await this.sendScpi(cmd);
        ch.outputEnabled = next;
        this.addLog(cmd, true);
      } catch (err) {
        this.addLog(`${cmd} (${err.message})`, false);
      }
    },
    async applyMode(index) {
      const ch = this.channels[index];
      const modeMap = {
        CV: "VOLT",
        CC: "CURR",
        CP: "POW",
        CR: "RES",
      };

      const cmd = `SOUR${ch.id}:FUNC ${modeMap[ch.mode] || "VOLT"}`;
      ch.setpointValue = this.getSetpointByMode(ch, ch.mode);

      try {
        await this.sendScpi(cmd);
        this.addLog(cmd, true);
      } catch (err) {
        this.addLog(`${cmd} (${err.message})`, false);
      }
    },
    async applySetpoint(index) {
      const ch = this.channels[index];
      const value = Number(ch.setpointValue);
      this.setSetpointByMode(ch, ch.mode, value);

      let cmd;
      if (ch.mode === "CV") {
        cmd = `SOUR${ch.id}:VOLT ${value}`;
      } else if (ch.mode === "CC") {
        cmd = `SOUR${ch.id}:CURR ${value}`;
      } else if (ch.mode === "CP") {
        cmd = `SOUR${ch.id}:POW ${value}`;
      } else {
        cmd = `SOUR${ch.id}:RES ${value}`;
      }

      try {
        await this.sendScpi(cmd);
        this.addLog(cmd, true);
      } catch (err) {
        this.addLog(`${cmd} (${err.message})`, false);
      }
    },
    async applyUvCutoff(index) {
      const ch = this.channels[index];
      const value = ch.uvCutoffEnabled ? Number(ch.uvCutoffValue) : 999;
      const cmd = `BATT${ch.id}:LVP ${value}`;

      try {
        await this.sendScpi(cmd);
        this.addLog(cmd, true);
      } catch (err) {
        this.addLog(`${cmd} (${err.message})`, false);
      }
    },
    async sendRawScpi() {
      const cmd = this.rawScpi.trim();
      if (!cmd) return;

      try {
        await this.sendScpi(cmd);
        this.addLog(cmd, true);
      } catch (err) {
        this.addLog(`${cmd} (${err.message})`, false);
      }
      this.rawScpi = "";
    },
  },
}).mount("#app");
