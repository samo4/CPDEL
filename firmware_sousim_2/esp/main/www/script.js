const { createApp } = Vue;

createApp({
  data() {
    return {
      selectedChannel: 0,
      rawScpi: "",
      logs: [],
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
