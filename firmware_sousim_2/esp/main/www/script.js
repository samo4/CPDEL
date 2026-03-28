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
      ch.setpointValue = this.getSetpointByMode(ch, ch.mode);
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
        if (typeof payload.current === "number") ch.measuredCurrent = payload.current;
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
          if (ch.mode === "CV") ch.setpointValue = value;
          break;
        case "SET_CURRENT":
          ch.ccSetpoint = value;
          if (ch.mode === "CC") ch.setpointValue = value;
          break;
        case "SET_POWER":
          ch.cpSetpoint = value;
          if (ch.mode === "CP") ch.setpointValue = value;
          break;
        case "SET_RESISTANCE":
          ch.crSetpoint = value;
          if (ch.mode === "CR") ch.setpointValue = value;
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
