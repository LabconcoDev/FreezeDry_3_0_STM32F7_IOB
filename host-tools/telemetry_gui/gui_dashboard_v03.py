import datetime
import queue
import struct
import threading
import time
import tkinter as tk
from tkinter import ttk, scrolledtext
import serial
import serial.tools.list_ports

SOF_L = 0x4C
SOF_C = 0x43

CLASS_READ  = 0x01
CLASS_WRITE = 0x02
CLASS_PUSH  = 0x03
CLASS_ACK   = 0x04

TYPE_SENSORS    = 0x01
TYPE_CONTROL    = 0x02
TYPE_ALERT      = 0x03
TYPE_SYSTEM     = 0x04
TYPE_SETTINGS   = 0x05
TYPE_DIAGNOSTIC = 0x06

TEMP_SENSORS = [
    ("Collector Temp",            0x01),
    ("Ambient Temp",              0x02),
    ("Lexsol Temp",               0x03),
    ("1st Stage HX Outlet",       0x04),
    ("2nd Stage HX Outlet",       0x05),
    ("1st Stage Suction",         0x06),
    ("2nd Stage Suction",         0x07),
    ("Shell Freezer Temp",        0x08),
    ("Mini Chamber Temp",         0x09),
    ("Shelf 1 Temp",              0x0A),
    ("Shelf 2 Temp",              0x0B),
    ("Shelf 3 Temp",              0x0C),
    ("Shelf 4 Temp",              0x0D),
    ("Shelf 5 Temp",              0x0E),
    ("Sample 1 Temp",             0x0F),
    ("Sample 2 Temp",             0x10),
    ("Sample 3 Temp",             0x11),
    ("Sample 4 Temp",             0x12),
    ("Sample 5 Temp",             0x13),
]

VAC_SENSORS = [
    ("System Vacuum",             0x14),
    ("Sample Vac 1",              0x15),
    ("Sample Vac 2",              0x16),
    ("Sample Vac 3",              0x17),
    ("Sample Vac 4",              0x18),
    ("Sample Vac 5",              0x19),
    ("Sample Vac 6",              0x1A),
    ("ST Dryer Vacuum",           0x1B),
    ("EPD Addon Vac 1",           0x1C),
    ("EPD Addon Vac 2",           0x1D),
]

ALERT_NAMES = {
    0x01: "ALERT_ACK_BY_USER_CLEAR_ALL",
    0x02: "ALERT_CLEAR_SPECIFIC_ALARM",
    0x03: "ALERT_SHELF_TEMP_VAR",
    0x04: "ALERT_COLLECTOR_TEMP_VAR",
    0x05: "ALERT_SHELL_TEMP_VAR",
    0x06: "ALERT_LEXSOL_TEMP_VAR",
    0x07: "ALERT_LEXSOL_TEMP_SET_POINT_FAIL",
    0x08: "ALERT_SHELF_TEMP_SET_POINT_TIMEOUT",
    0x09: "ALERT_TEMP_RANGE_OUT",
    0x0A: "ALERT_TEMP_PROBE_ERROR",
    0x0B: "ALERT_VACUUM_LEAK",
    0x0C: "ALERT_VACUUM_VAR",
    0x0D: "ALERT_VACUUM_SET_POINT_TIMEOUT",
    0x0E: "ALERT_VACUUM_PROBE_ERROR",
    0x0F: "ALERT_VACUUM_VAR_SET_POINT_STOPPERING_TRAY",
    0x10: "ALERT_PLATEN_TILT",
    0x11: "ALERT_COLLECTOR_MOISTURE",
    0x12: "ALERT_END_POINT1_ACHIEVED",
    0x13: "ALERT_END_POINT2_ACHIEVED",
    0x14: "ALERT_END_POINT3_ACHIEVED",
    0x15: "ALERT_END_POINT4_ACHIEVED",
    0x16: "ALERT_END_POINT5_ACHIEVED",
    0x17: "ALERT_PROGRAM_COMPLETE",
    0x18: "ALERT_PROGRAM_SEGMENT_COMPLETE",
    0x19: "ALERT_DOOR_STATUS",
    0x1A: "ALERT_POWER_FAIL_IOB",
    0x1B: "ALERT_POWER_FAIL_ACCESSORY",
}

CALIB_TEMP_MAP = {
    "1: Collector Temp":     1,
    "2: Lexsol/System Temp": 2,
    "3: Mini Chamber Temp":  3,
    "4: Shell Freezer Temp": 4,
    "5: Sample 1 Temp":      5,
    "6: Sample 2 Temp":      6,
    "7: Sample 3 Temp":      7,
    "8: Sample 4 Temp":      8,
    "9: Sample 5 Temp":      9,
    "10: Shelf 1 Temp":      10,
    "11: Shelf 2 Temp":      11,
    "12: Shelf 3 Temp":      12,
    "13: Shelf 4 Temp":      13,
    "14: Shelf 5 Temp":      14,
}

CALIB_VAC_MAP = {
    "1: System Vacuum":   1,
    "2: ST Dryer Vacuum": 2,
    "3: Sample Vac 1":    3,
    "4: Sample Vac 2":    4,
    "5: Sample Vac 3":    5,
    "6: Sample Vac 4":    6,
    "7: Sample Vac 5":    7,
    "8: Sample Vac 6":    8,
}

DIAG_TESTS = {
    "1: Defrost Relay":              1,
    "2: Vent Valve":                 2,
    "3: Pre-freeze / Collector Vlv": 3,
    "4: Vacuum Pump Relay":          4,
    "5: Compressor Relays":          5,
    "6: Lexsol Pump Relay":          6,
    "7: Lexsol Heater":              7,
    "8: Purge Valve":                8,
}


class FD3Dashboard(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Freeze Dry 3.0 — ICD Test & Telemetry UI")
        self.geometry("1220x900")

        self.ser = None
        self.rx_running = False
        self.rx_queue = queue.Queue()
        self.sensor_vars = {}
        self.ctrl_vars = {}
        self.pause_log_var = tk.BooleanVar(value=False)

        self._build_top_bar()
        self.paned = ttk.PanedWindow(self, orient=tk.VERTICAL)
        self.paned.pack(fill=tk.BOTH, expand=True, padx=6, pady=4)
        self._build_tabs()
        self._build_log_panel()
        self._refresh_ports()
        self.after(50, self._process_rx_queue)

    # ---------------- Protocol Helpers ----------------
    @staticmethod
    def calc_checksum(payload: bytes) -> int:
        return (len(payload) + sum(payload)) & 0xFFFF

    def send_packet(self, cls: int, msg_type: int, action: int, data: bytes = b""):
        if not self.ser or not self.ser.is_open:
            self._log("[ERR] Not connected to serial port")
            return
        payload = bytes([cls, msg_type, action]) + data
        csum = self.calc_checksum(payload)
        frame = bytes([SOF_L, SOF_C, len(payload)]) + payload + struct.pack("<H", csum)
        try:
            self.ser.write(frame)
            self._log(f"TX: {frame.hex(' ')}")
        except Exception as e:
            self._log(f"[TX ERR] {e}")

    def _rx_worker(self):
        while self.rx_running and self.ser and self.ser.is_open:
            try:
                b = self.ser.read(1)
                if not b or b[0] != SOF_L:
                    continue
                b2 = self.ser.read(1)
                if not b2 or b2[0] != SOF_C:
                    continue
                lb = self.ser.read(1)
                if not lb:
                    continue
                plen = lb[0]
                rest = self.ser.read(plen + 2)
                if len(rest) != plen + 2:
                    continue
                payload = rest[:plen]
                rx_cs = struct.unpack("<H", rest[plen:])[0]
                if rx_cs != self.calc_checksum(payload):
                    self.rx_queue.put(("ERR", f"Bad checksum: {rest.hex(' ')}"))
                    continue
                self.rx_queue.put(("PKT", (payload[0], payload[1], payload[2], payload[3:])))
            except Exception as e:
                if self.rx_running:
                    self.rx_queue.put(("ERR", str(e)))
                break

    def _process_rx_queue(self):
        while not self.rx_queue.empty():
            kind, val = self.rx_queue.get_nowait()
            if kind == "ERR":
                self._log(f"[RX ERR] {val}")
            else:
                cls, msg_type, action, data = val
                self._handle_packet(cls, msg_type, action, data)
        self.after(50, self._process_rx_queue)

    def _handle_packet(self, cls: int, msg_type: int, action: int, data: bytes):
        if cls == CLASS_ACK:
            self._log(f"RX ACK: type=0x{msg_type:02X}, action=0x{action:02X}")
            return

        if cls != CLASS_PUSH:
            self._log(f"RX cls=0x{cls:02X}, type=0x{msg_type:02X}, act=0x{action:02X}, data={data.hex(' ')}")
            return

        if msg_type == TYPE_SENSORS:
            self._handle_sensor_push(action, data)
        elif msg_type == TYPE_CONTROL:
            self._handle_control_push(action, data)
        elif msg_type == TYPE_SYSTEM:
            self._handle_system_push(action, data)
        elif msg_type == TYPE_SETTINGS:
            self._handle_settings_push(action, data)
        elif msg_type == TYPE_ALERT:
            self._handle_alert_push(action, data)
        elif msg_type == TYPE_DIAGNOSTIC:
            self._handle_diag_push(action, data)

    def _handle_sensor_push(self, action: int, data: bytes):
        if 0x01 <= action <= 0x13 and len(data) >= 2:
            raw = struct.unpack("<h", data[:2])[0]
            txt = "ERR/Stale" if raw == 0x7FFF else f"{raw / 10.0:.1f} °C"
        elif 0x14 <= action <= 0x1D and len(data) >= 2:
            raw = struct.unpack("<H", data[:2])[0]
            txt = "ERR/Stale" if raw == 0xFFFF else f"{raw / 1000.0:.3f} mbar"
        elif action == 0x1E and len(data) >= 1:
            txt = "ON (1)" if data[0] else "OFF (0)"
        elif action == 0x1F and len(data) >= 3:
            txt = f"Opt={data[0]} Valves=0x{data[1]:02X} EPD=0x{data[2]:02X}"
        elif action == 0x20 and len(data) >= 2:
            txt = f"Shelf=0x{data[0]:02X} Sample=0x{data[1]:02X}"
        else:
            txt = data.hex(" ")

        if action in self.sensor_vars:
            self.sensor_vars[action].set(txt)
        self._log(f"RX SENSOR 0x{action:02X}: {txt}")

    def _handle_control_push(self, action: int, data: bytes):
        if action == 0x0C and len(data) >= 4:
            seg, t_left, rh = struct.unpack("<BHB", data[:4])
            self.prog_status_var.set(f"Seg {seg} | Time Left: {t_left} min | {'Hold' if rh else 'Ramp'}")
            self._log(f"RX PROGRAM STATUS: {self.prog_status_var.get()}")
        elif len(data) >= 1:
            txt = f"0x{data[0]:02X} ({'ON' if (data[0] & 0x0F) else 'OFF'})"
            if action in self.ctrl_vars:
                self.ctrl_vars[action].set(txt)
            self._log(f"RX CONTROL 0x{action:02X}: {txt}")

    def _handle_system_push(self, action: int, data: bytes):
        if action in (0x01, 0x0A) and len(data) >= 3:
            maj, mino = chr(data[0]), chr(data[1])
            mic = data[2]
            prod = data[3:].decode("ascii", errors="replace")
            txt = f"{maj}.{mino}.{mic} ({prod})"
            (self.fw_iob_var if action == 0x01 else self.fw_accy_var).set(txt)
            self._log(f"RX FW VER (0x{action:02X}): {txt}")
        elif action in (0x02, 0x0B) and len(data) >= 12:
            m = data[0:4].decode("ascii", errors="replace")
            s = data[4:8].decode("ascii", errors="replace")
            sn = data[8:12].decode("ascii", errors="replace")
            txt = f"Model={m} Special={s} Serial={sn}"
            (self.model_iob_var if action == 0x02 else self.model_accy_var).set(txt)
            self._log(f"RX MODEL (0x{action:02X}): {txt}")
        elif action == 0x04 and len(data) >= 3:
            txt = f"{data[0]:02d}/{data[1]:02d}/20{data[2]:02d}"
            self.date_var.set(txt)
            self._log(f"RX DATE: {txt}")
        elif action == 0x05 and len(data) >= 3:
            txt = f"{data[2]:02d}:{data[1]:02d}:{data[0]:02d}"
            self.time_var.set(txt)
            self._log(f"RX TIME: {txt}")
        else:
            self._log(f"RX SYSTEM 0x{action:02X}: {data.hex(' ')}")

    def _handle_settings_push(self, action: int, data: bytes):
        if len(data) >= 3:
            sensor_id, offset = struct.unpack("<Bh", data[:3])
            if action == 0x01:
                self.calib_temp_read_var.set(f"Sensor {sensor_id}: {offset / 10.0:.1f} °C (raw={offset})")
            elif action == 0x02:
                self.calib_vac_read_var.set(f"Sensor {sensor_id}: {offset / 1000.0:.3f} mbar (raw={offset})")
            self._log(f"RX SETTINGS 0x{action:02X}: sensor={sensor_id}, offset={offset}")

    def _handle_alert_push(self, action: int, data: bytes):
        name = ALERT_NAMES.get(action, f"ALERT_0x{action:02X}")
        ts = datetime.datetime.now().strftime("%H:%M:%S")
        msg = f"[{ts}] {name} (data={data.hex(' ') or 'none'})\n"
        self.alert_log.insert(tk.END, msg)
        self.alert_log.see(tk.END)
        self._log(f"RX ALERT: {name} {data.hex(' ')}")

    def _handle_diag_push(self, action: int, data: bytes):
        if action == 0x01 and len(data) >= 2:
            tid, pf = data[0], data[1]
            res = "PASS" if pf == 1 else "FAIL"
            ts = datetime.datetime.now().strftime("%H:%M:%S")
            self.diag_log.insert(tk.END, f"[{ts}] Test ID {tid}: {res}\n")
            self.diag_log.see(tk.END)
            self._log(f"RX DIAG COMPLETE: Test {tid} = {res}")

    # ---------------- UI Construction ----------------
    def _build_top_bar(self):
        top = ttk.Frame(self, padding=6)
        top.pack(fill=tk.X)

        ttk.Label(top, text="Port:").pack(side=tk.LEFT)
        self.port_cb = ttk.Combobox(top, width=14, state="readonly")
        self.port_cb.pack(side=tk.LEFT, padx=4)
        ttk.Button(top, text="Refresh", command=self._refresh_ports).pack(side=tk.LEFT, padx=2)

        ttk.Label(top, text="Baud:").pack(side=tk.LEFT, padx=(10, 2))
        self.baud_cb = ttk.Combobox(top, width=8, values=["9600", "115200"], state="readonly")
        self.baud_cb.set("115200")
        self.baud_cb.pack(side=tk.LEFT)

        self.conn_btn = ttk.Button(top, text="Connect", command=self._toggle_connect)
        self.conn_btn.pack(side=tk.LEFT, padx=8)

        self.status_lbl = ttk.Label(top, text="Disconnected", foreground="red")
        self.status_lbl.pack(side=tk.LEFT, padx=6)

    def _build_tabs(self):
        self.nb = ttk.Notebook(self.paned)
        self.paned.add(self.nb, weight=3)

        self._build_sensors_tab()
        self._build_control_tab()
        self._build_system_tab()
        self._build_settings_tab()
        self._build_alerts_tab()
        self._build_diag_tab()

    def _build_sensors_tab(self):
        tab = ttk.Frame(self.nb, padding=8)
        self.nb.add(tab, text="Sensors")

        hdr = ttk.Frame(tab)
        hdr.pack(fill=tk.X, pady=(0, 6))
        ttk.Button(hdr, text="Read All Main IOB Sensors", command=self._read_all_sensors).pack(side=tk.LEFT)

        cols = ttk.Frame(tab)
        cols.pack(fill=tk.BOTH, expand=True)

        tf = ttk.LabelFrame(cols, text="Temperature Sensors (0x01 - 0x13)", padding=6)
        tf.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 4))
        for i, (name, act) in enumerate(TEMP_SENSORS):
            var = tk.StringVar(value="---")
            self.sensor_vars[act] = var
            ttk.Label(tf, text=f"{name} (0x{act:02X}):", width=26).grid(row=i, column=0, sticky=tk.W)
            ttk.Label(tf, textvariable=var, width=14).grid(row=i, column=1, sticky=tk.W)
            ttk.Button(tf, text="Read", width=6,
                       command=lambda a=act: self.send_packet(CLASS_READ, TYPE_SENSORS, a)).grid(row=i, column=2, padx=2, pady=1)

        vf = ttk.LabelFrame(cols, text="Vacuum & Hardware Detection (0x14 - 0x20)", padding=6)
        vf.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(4, 0))
        extra = VAC_SENSORS + [
            ("Accy AC Power",   0x1E),
            ("HW Detect IOB",   0x1F),
            ("HW Detect ACCY",  0x20),
        ]
        for i, (name, act) in enumerate(extra):
            var = tk.StringVar(value="---")
            self.sensor_vars[act] = var
            ttk.Label(vf, text=f"{name} (0x{act:02X}):", width=22).grid(row=i, column=0, sticky=tk.W)
            ttk.Label(vf, textvariable=var, width=26).grid(row=i, column=1, sticky=tk.W)
            ttk.Button(vf, text="Read", width=6,
                       command=lambda a=act: self.send_packet(CLASS_READ, TYPE_SENSORS, a)).grid(row=i, column=2, padx=2, pady=1)

    def _build_control_tab(self):
        tab = ttk.Frame(self.nb, padding=8)
        self.nb.add(tab, text="Control & Program")

        left = ttk.LabelFrame(tab, text="Relays, Valves & Modes", padding=8)
        left.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 4))

        simple_ctrls = [
            ("Refrig Relay (0x01)",       0x01, True),
            ("Purge Relay (0x03)",        0x03, True),
            ("Shell Freezer (0x04)",      0x04, True),
            ("Defrost Relay (0x05)",      0x05, True),
            ("Vacuum Vent Valve (0x06)",  0x06, True),
            ("Auto Mode (0x09)",          0x09, False),
        ]
        for r, (lbl, act, can_read) in enumerate(simple_ctrls):
            var = tk.StringVar(value="---")
            self.ctrl_vars[act] = var
            ttk.Label(left, text=lbl, width=24).grid(row=r, column=0, sticky=tk.W, pady=2)
            ttk.Button(left, text="ON", width=5,
                       command=lambda a=act: self.send_packet(CLASS_WRITE, TYPE_CONTROL, a, b"\x01")).grid(row=r, column=1, padx=2)
            ttk.Button(left, text="OFF", width=5,
                       command=lambda a=act: self.send_packet(CLASS_WRITE, TYPE_CONTROL, a, b"\x00")).grid(row=r, column=2, padx=2)
            if can_read:
                ttk.Button(left, text="Read", width=5,
                           command=lambda a=act: self.send_packet(CLASS_READ, TYPE_CONTROL, a)).grid(row=r, column=3, padx=2)
            ttk.Label(left, textvariable=var, width=12).grid(row=r, column=4, padx=4)

        r = len(simple_ctrls)
        self.vac_sp_ent = ttk.Entry(left, width=8)
        self.vac_sp_ent.insert(0, "0.120")
        self.vac_prot_var = tk.BooleanVar(value=True)
        ttk.Label(left, text="Vacuum Rly (mbar):").grid(row=r, column=0, sticky=tk.W, pady=(10, 2))
        self.vac_sp_ent.grid(row=r, column=1, pady=(10, 2))
        ttk.Checkbutton(left, text="VacProt", variable=self.vac_prot_var).grid(row=r, column=2, pady=(10, 2))
        ttk.Button(left, text="ON", width=5, command=lambda: self._send_vac_rly(1)).grid(row=r, column=3, pady=(10, 2))
        ttk.Button(left, text="OFF", width=5, command=lambda: self._send_vac_rly(0)).grid(row=r, column=4, pady=(10, 2))

        r += 1
        self.mc_temp_ent = ttk.Entry(left, width=8)
        self.mc_temp_ent.insert(0, "25.0")
        ttk.Label(left, text="Mini Chamber (°C):").grid(row=r, column=0, sticky=tk.W, pady=2)
        self.mc_temp_ent.grid(row=r, column=1)
        ttk.Button(left, text="ON", width=5, command=lambda: self._send_mini_chamber(1)).grid(row=r, column=2)
        ttk.Button(left, text="OFF", width=5, command=lambda: self._send_mini_chamber(0)).grid(row=r, column=3)

        r += 1
        self.sh_temp_ent = ttk.Entry(left, width=8)
        self.sh_temp_ent.insert(0, "-20.0")
        self.sh_maxcold_var = tk.BooleanVar()
        self.sh_inflask_var = tk.BooleanVar()
        ttk.Label(left, text="Manual Shelf (°C):").grid(row=r, column=0, sticky=tk.W, pady=2)
        self.sh_temp_ent.grid(row=r, column=1)
        ttk.Checkbutton(left, text="MaxCold", variable=self.sh_maxcold_var).grid(row=r, column=2)
        ttk.Checkbutton(left, text="InFlask", variable=self.sh_inflask_var).grid(row=r, column=3)
        ttk.Button(left, text="ON", width=5, command=lambda: self._send_manual_shelf(1)).grid(row=r, column=4)
        ttk.Button(left, text="OFF", width=5, command=lambda: self._send_manual_shelf(0)).grid(row=r, column=5)

        right = ttk.LabelFrame(tab, text="Program Segments & Run (0x0A - 0x0C)", padding=8)
        right.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(4, 0))

        fields = [
            ("Segment No:",          "1"),
            ("Shelf Temp (°C):",     "-30.0"),
            ("Ramp Rate (°C/min):",  "1.50"),
            ("Vacuum (mbar):",       "0.100"),
            ("Hold Time (mins):",    "60"),
        ]
        self.pgm_entries = {}
        for idx, (lbl, default) in enumerate(fields):
            ttk.Label(right, text=lbl).grid(row=idx, column=0, sticky=tk.W, pady=2)
            ent = ttk.Entry(right, width=12)
            ent.insert(0, default)
            ent.grid(row=idx, column=1, sticky=tk.W, pady=2)
            self.pgm_entries[lbl] = ent

        self.pgm_vac_on = tk.BooleanVar(value=True)
        self.pgm_vac_prot = tk.BooleanVar(value=True)
        self.pgm_max_cold = tk.BooleanVar(value=False)
        self.pgm_inf_hold = tk.BooleanVar(value=False)
        ttk.Checkbutton(right, text="Vac ON", variable=self.pgm_vac_on).grid(row=5, column=0, sticky=tk.W)
        ttk.Checkbutton(right, text="Vac Protect", variable=self.pgm_vac_prot).grid(row=5, column=1, sticky=tk.W)
        ttk.Checkbutton(right, text="Max Cold", variable=self.pgm_max_cold).grid(row=6, column=0, sticky=tk.W)
        ttk.Checkbutton(right, text="Infinite Hold", variable=self.pgm_inf_hold).grid(row=6, column=1, sticky=tk.W)

        btns = ttk.Frame(right)
        btns.grid(row=7, column=0, columnspan=2, pady=10, sticky=tk.W)
        ttk.Button(btns, text="Send Segment", command=self._send_program_segment).pack(side=tk.LEFT, padx=2)
        ttk.Button(btns, text="Start Program", command=lambda: self.send_packet(CLASS_WRITE, TYPE_CONTROL, 0x0B, b"\x01")).pack(side=tk.LEFT, padx=2)
        ttk.Button(btns, text="Stop Program", command=lambda: self.send_packet(CLASS_WRITE, TYPE_CONTROL, 0x0B, b"\x00")).pack(side=tk.LEFT, padx=2)
        ttk.Button(btns, text="Read Status", command=lambda: self.send_packet(CLASS_READ, TYPE_CONTROL, 0x0C)).pack(side=tk.LEFT, padx=2)

        self.prog_status_var = tk.StringVar(value="Status: ---")
        ttk.Label(right, textvariable=self.prog_status_var, font=("Segoe UI", 9, "bold")).grid(row=8, column=0, columnspan=2, sticky=tk.W)

    def _build_system_tab(self):
        tab = ttk.Frame(self.nb, padding=8)
        self.nb.add(tab, text="System")

        self.fw_iob_var = tk.StringVar(value="---")
        self.fw_accy_var = tk.StringVar(value="---")
        self.model_iob_var = tk.StringVar(value="---")
        self.model_accy_var = tk.StringVar(value="---")
        self.date_var = tk.StringVar(value="---")
        self.time_var = tk.StringVar(value="---")

        rows = [
            ("IOB FW Version (0x01):",  self.fw_iob_var,    0x01),
            ("ACCY FW Version (0x0A):", self.fw_accy_var,   0x0A),
            ("IOB Model (0x02):",       self.model_iob_var, 0x02),
            ("ACCY Model (0x0B):",      self.model_accy_var,0x0B),
            ("System Date (0x04):",     self.date_var,      0x04),
            ("System Time (0x05):",     self.time_var,      0x05),
        ]
        for r, (lbl, var, act) in enumerate(rows):
            ttk.Label(tab, text=lbl, width=24).grid(row=r, column=0, sticky=tk.W, pady=3)
            ttk.Label(tab, textvariable=var, width=38).grid(row=r, column=1, sticky=tk.W)
            ttk.Button(tab, text="Read", command=lambda a=act: self.send_packet(CLASS_READ, TYPE_SYSTEM, a)).grid(row=r, column=2, padx=4)

        act_frame = ttk.Frame(tab)
        act_frame.grid(row=6, column=0, columnspan=3, sticky=tk.W, pady=12)
        ttk.Button(act_frame, text="Sync PC Date & Time", command=self._sync_datetime).pack(side=tk.LEFT, padx=4)
        ttk.Button(act_frame, text="Send I_AM_ALIVE (0x06)", command=lambda: self.send_packet(CLASS_WRITE, TYPE_SYSTEM, 0x06)).pack(side=tk.LEFT, padx=4)
        ttk.Button(act_frame, text="Req ACCY FW Update (0x0C)", command=lambda: self.send_packet(CLASS_WRITE, TYPE_SYSTEM, 0x0C)).pack(side=tk.LEFT, padx=4)

    def _build_settings_tab(self):
        tab = ttk.Frame(self.nb, padding=8)
        self.nb.add(tab, text="Settings (Calibration)")

        tf = ttk.LabelFrame(tab, text="Temperature Calibration Offset (0x01)", padding=8)
        tf.pack(fill=tk.X, pady=4)
        self.calib_t_cb = ttk.Combobox(tf, values=list(CALIB_TEMP_MAP.keys()), width=24, state="readonly")
        self.calib_t_cb.current(0)
        self.calib_t_cb.pack(side=tk.LEFT, padx=4)
        ttk.Label(tf, text="Offset (°C):").pack(side=tk.LEFT)
        self.calib_t_ent = ttk.Entry(tf, width=8)
        self.calib_t_ent.insert(0, "0.0")
        self.calib_t_ent.pack(side=tk.LEFT, padx=4)
        ttk.Button(tf, text="Write", command=self._write_temp_calib).pack(side=tk.LEFT, padx=2)
        ttk.Button(tf, text="Read", command=self._read_temp_calib).pack(side=tk.LEFT, padx=2)
        self.calib_temp_read_var = tk.StringVar(value="---")
        ttk.Label(tf, textvariable=self.calib_temp_read_var).pack(side=tk.LEFT, padx=10)

        vf = ttk.LabelFrame(tab, text="Vacuum Calibration Offset (0x02)", padding=8)
        vf.pack(fill=tk.X, pady=4)
        self.calib_v_cb = ttk.Combobox(vf, values=list(CALIB_VAC_MAP.keys()), width=24, state="readonly")
        self.calib_v_cb.current(0)
        self.calib_v_cb.pack(side=tk.LEFT, padx=4)
        ttk.Label(vf, text="Offset (mbar):").pack(side=tk.LEFT)
        self.calib_v_ent = ttk.Entry(vf, width=8)
        self.calib_v_ent.insert(0, "0.000")
        self.calib_v_ent.pack(side=tk.LEFT, padx=4)
        ttk.Button(vf, text="Write", command=self._write_vac_calib).pack(side=tk.LEFT, padx=2)
        ttk.Button(vf, text="Read", command=self._read_vac_calib).pack(side=tk.LEFT, padx=2)
        self.calib_vac_read_var = tk.StringVar(value="---")
        ttk.Label(vf, textvariable=self.calib_vac_read_var).pack(side=tk.LEFT, padx=10)

    def _build_alerts_tab(self):
        tab = ttk.Frame(self.nb, padding=8)
        self.nb.add(tab, text="Alerts")

        ctrl = ttk.Frame(tab)
        ctrl.pack(fill=tk.X, pady=(0, 6))
        ttk.Button(ctrl, text="Clear All Alarms (0x01)",
                   command=lambda: self.send_packet(CLASS_PUSH, TYPE_ALERT, 0x01)).pack(side=tk.LEFT, padx=4)
        self.alert_id_cb = ttk.Combobox(ctrl, values=[f"0x{k:02X}: {v}" for k, v in ALERT_NAMES.items() if k >= 3],
                                        width=44, state="readonly")
        self.alert_id_cb.current(0)
        self.alert_id_cb.pack(side=tk.LEFT, padx=6)
        ttk.Button(ctrl, text="Clear Specific (0x02)", command=self._clear_specific_alert).pack(side=tk.LEFT, padx=2)
        ttk.Button(ctrl, text="Clear Log", command=lambda: self.alert_log.delete("1.0", tk.END)).pack(side=tk.RIGHT)

        self.alert_log = scrolledtext.ScrolledText(tab, height=12)
        self.alert_log.pack(fill=tk.BOTH, expand=True)

    def _build_diag_tab(self):
        tab = ttk.Frame(self.nb, padding=8)
        self.nb.add(tab, text="Diagnostics")

        ctrl = ttk.Frame(tab)
        ctrl.pack(fill=tk.X, pady=(0, 6))
        self.diag_cb = ttk.Combobox(ctrl, values=list(DIAG_TESTS.keys()), width=30, state="readonly")
        self.diag_cb.current(0)
        self.diag_cb.pack(side=tk.LEFT, padx=4)
        ttk.Button(ctrl, text="Start Test (0x02)",
                   command=lambda: self.send_packet(CLASS_WRITE, TYPE_DIAGNOSTIC, 0x02, bytes([DIAG_TESTS[self.diag_cb.get()]]))).pack(side=tk.LEFT, padx=4)
        ttk.Button(ctrl, text="Stop Test (0x03)",
                   command=lambda: self.send_packet(CLASS_WRITE, TYPE_DIAGNOSTIC, 0x03, bytes([DIAG_TESTS[self.diag_cb.get()]]))).pack(side=tk.LEFT, padx=4)

        self.diag_log = scrolledtext.ScrolledText(tab, height=12)
        self.diag_log.pack(fill=tk.BOTH, expand=True)

    def _build_log_panel(self):
        lf = ttk.LabelFrame(self.paned, text="Raw TX / RX Packet Log (drag divider above to resize)", padding=4)
        self.paned.add(lf, weight=2)

        bar = ttk.Frame(lf)
        bar.pack(fill=tk.X)
        ttk.Checkbutton(bar, text="Pause Log", variable=self.pause_log_var).pack(side=tk.LEFT)
        ttk.Button(bar, text="Clear", command=lambda: self.raw_log.delete("1.0", tk.END)).pack(side=tk.RIGHT)

        self.raw_log = scrolledtext.ScrolledText(lf, height=16, font=("Consolas", 9))
        self.raw_log.pack(fill=tk.BOTH, expand=True)

    # ---------------- Actions ----------------
    def _refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_cb["values"] = ports
        if ports and not self.port_cb.get():
            self.port_cb.current(0)

    def _toggle_connect(self):
        if self.ser and self.ser.is_open:
            self.rx_running = False
            self.ser.close()
            self.conn_btn.config(text="Connect")
            self.status_lbl.config(text="Disconnected", foreground="red")
            return
        port = self.port_cb.get()
        baud = int(self.baud_cb.get())
        try:
            self.ser = serial.Serial(port, baud, timeout=0.1)
            self.rx_running = True
            threading.Thread(target=self._rx_worker, daemon=True).start()
            self.conn_btn.config(text="Disconnect")
            self.status_lbl.config(text=f"Connected ({port} @ {baud})", foreground="green")
        except Exception as e:
            self._log(f"[CONN ERR] {e}")

    def _read_all_sensors(self):
        for _, act in TEMP_SENSORS[:9] + VAC_SENSORS[:7] + [("HW", 0x1F)]:
            self.send_packet(CLASS_READ, TYPE_SENSORS, act)
            time.sleep(0.02)

    def _send_vac_rly(self, on: int):
        sp = int(round(float(self.vac_sp_ent.get()) * 1000))
        flags = (on & 0x0F) | (0x10 if self.vac_prot_var.get() else 0x00)
        self.send_packet(CLASS_WRITE, TYPE_CONTROL, 0x02, struct.pack("<BH", flags, sp))

    def _send_mini_chamber(self, on: int):
        temp = int(round(float(self.mc_temp_ent.get()) * 10))
        self.send_packet(CLASS_WRITE, TYPE_CONTROL, 0x07, struct.pack("<Bh", on, temp))

    def _send_manual_shelf(self, on: int):
        temp = int(round(float(self.sh_temp_ent.get()) * 10))
        flags = (on & 0x0F) | (0x10 if self.sh_maxcold_var.get() else 0) | (0x20 if self.sh_inflask_var.get() else 0)
        self.send_packet(CLASS_WRITE, TYPE_CONTROL, 0x08, struct.pack("<Bh", flags, temp))

    def _send_program_segment(self):
        seg  = int(self.pgm_entries["Segment No:"].get())
        temp = int(round(float(self.pgm_entries["Shelf Temp (°C):"].get()) * 10))
        ramp = int(round(float(self.pgm_entries["Ramp Rate (°C/min):"].get()) * 100))
        vac  = int(round(float(self.pgm_entries["Vacuum (mbar):"].get()) * 1000))
        hold = int(self.pgm_entries["Hold Time (mins):"].get())
        flags = ((1 if self.pgm_vac_on.get() else 0)
                 | (2 if self.pgm_vac_prot.get() else 0)
                 | (4 if self.pgm_max_cold.get() else 0)
                 | (8 if self.pgm_inf_hold.get() else 0))
        self.send_packet(CLASS_WRITE, TYPE_CONTROL, 0x0A, struct.pack("<BBhHHH", seg, flags, temp, ramp, vac, hold))

    def _sync_datetime(self):
        now = datetime.datetime.now()
        self.send_packet(CLASS_WRITE, TYPE_SYSTEM, 0x04, bytes([now.day, now.month, now.year % 100]))
        self.send_packet(CLASS_WRITE, TYPE_SYSTEM, 0x05, bytes([now.second, now.minute, now.hour]))

    def _write_temp_calib(self):
        sid = CALIB_TEMP_MAP[self.calib_t_cb.get()]
        off = int(round(float(self.calib_t_ent.get()) * 10))
        self.send_packet(CLASS_WRITE, TYPE_SETTINGS, 0x01, struct.pack("<Bh", sid, off))

    def _read_temp_calib(self):
        sid = CALIB_TEMP_MAP[self.calib_t_cb.get()]
        self.send_packet(CLASS_READ, TYPE_SETTINGS, 0x01, bytes([sid]))

    def _write_vac_calib(self):
        sid = CALIB_VAC_MAP[self.calib_v_cb.get()]
        off = int(round(float(self.calib_v_ent.get()) * 1000))
        self.send_packet(CLASS_WRITE, TYPE_SETTINGS, 0x02, struct.pack("<Bh", sid, off))

    def _read_vac_calib(self):
        sid = CALIB_VAC_MAP[self.calib_v_cb.get()]
        self.send_packet(CLASS_READ, TYPE_SETTINGS, 0x02, bytes([sid]))

    def _clear_specific_alert(self):
        aid = int(self.alert_id_cb.get().split(":")[0], 16)
        self.send_packet(CLASS_PUSH, TYPE_ALERT, 0x02, bytes([aid]))

    def _log(self, msg: str):
        if self.pause_log_var.get():
            return
        ts = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
        self.raw_log.insert(tk.END, f"[{ts}] {msg}\n")
        self.raw_log.see(tk.END)


if __name__ == "__main__":
    FD3Dashboard().mainloop()