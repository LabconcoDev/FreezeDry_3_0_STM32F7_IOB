"""
Freeze Dry 3.0 - Accessory IOB UART4 Simulator GUI
Simulates Stoppering Tray (ST), Bulk Tray (BT), Mini Bulk Tray (mBT),
and Heated Product Shelf (HPS) over a spare USB-UART adapter connected
to the STM32F723 Main IOB UART4 (PC10 TX / PC11 RX) at 115200 baud.
"""

import struct
import threading
import time
import tkinter as tk
from tkinter import scrolledtext, ttk

import serial
import serial.tools.list_ports

# =========================================================
# ICD 3.0 PROTOCOL CONSTANTS
# =========================================================
FD_SOF_L = 0x4C  # 'L'
FD_SOF_C = 0x43  # 'C'

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

CLASS_NAMES = {
    CLASS_READ: "READ",
    CLASS_WRITE: "WRITE",
    CLASS_PUSH: "PUSH",
    CLASS_ACK: "ACK",
}

TYPE_NAMES = {
    TYPE_SENSORS: "SENSORS",
    TYPE_CONTROL: "CONTROL",
    TYPE_ALERT: "ALERT",
    TYPE_SYSTEM: "SYSTEM",
    TYPE_SETTINGS: "SETTINGS",
    TYPE_DIAGNOSTIC: "DIAG",
}

# Control Actions
CONTROL_MANUAL_SHELF_HEAT = 0x08
CONTROL_PROGRAM_SEGMENTS  = 0x0A
CONTROL_PROGRAM_RUN       = 0x0B
CONTROL_PROGRAM_STATUS    = 0x0C

# Sensor Actions
SENSOR_LEXSOL_TEMP                    = 0x03
SENSOR_SHELF1_TEMP                    = 0x0A
SENSOR_SHELF5_TEMP                    = 0x0E
SENSOR_SHELF1_SAMPLE_TEMP             = 0x0F
SENSOR_SHELF5_SAMPLE_TEMP             = 0x13
SENSOR_STOPPERING_TRAY_VACUUM         = 0x1B
SENSOR_DRYING_ACCESSORY_AC_POWER      = 0x1E
SENSOR_HARDWARE_SENSOR_DETECTION_ACCY = 0x20

# Alert Actions
ALERT_SHELF_TEMP_VAR                   = 0x03
ALERT_LEXSOL_TEMP_VAR                  = 0x06
ALERT_TEMP_PROBE_ERROR                 = 0x0A
ALERT_VACUUM_VAR_SET_POINT_STOPPERING  = 0x0F
ALERT_PROGRAM_COMPLETE                 = 0x17
ALERT_PROGRAM_SEGMENT_COMPLETE         = 0x18
ALERT_DOOR_STATUS                      = 0x19
ALERT_POWER_FAIL_ACCESSORY             = 0x1B

# System Actions
SYSTEM_FIRMWARE_VERSION_ACCY = 0x0A
SYSTEM_MODEL_DETAILS_ACCY    = 0x0B

# Accessory Profiles
PROFILES = {
    "Stoppering Tray (ST)": {
        "model": b"ST30", "special": b"0001", "serial": b"ST01",
        "fw": (3, 0, 1),
        "shelves": 3, "samples": 3,
        "has_lexsol": True, "has_vacuum": True, "has_door": True,
    },
    "Bulk Tray (BT)": {
        "model": b"BT30", "special": b"0001", "serial": b"BT01",
        "fw": (3, 0, 1),
        "shelves": 5, "samples": 5,
        "has_lexsol": False, "has_vacuum": False, "has_door": True,
    },
    "Mini Bulk Tray (mBT)": {
        "model": b"MBT3", "special": b"0001", "serial": b"MB01",
        "fw": (3, 0, 1),
        "shelves": 3, "samples": 3,
        "has_lexsol": False, "has_vacuum": False, "has_door": True,
    },
    "Heated Product Shelf (HPS)": {
        "model": b"HPS3", "special": b"0001", "serial": b"HP01",
        "fw": (3, 0, 1),
        "shelves": 5, "samples": 5,
        "has_lexsol": False, "has_vacuum": False, "has_door": False,
    },
}


# =========================================================
# FRAMING & SERIALIZATION
# =========================================================
def calc_checksum(payload_len: int, payload: bytes) -> int:
    return (payload_len + sum(payload)) & 0xFFFF


def build_frame(cls: int, ptype: int, action: int, data: bytes = b"") -> bytes:
    payload = bytes([cls, ptype, action]) + data
    plen = len(payload)
    csum = calc_checksum(plen, payload)
    return bytes([FD_SOF_L, FD_SOF_C, plen]) + payload + struct.pack("<H", csum)


class FrameParser:
    def __init__(self, on_packet):
        self.state = 0
        self.expected_len = 0
        self.buf = bytearray()
        self.csum_lsb = 0
        self.on_packet = on_packet

    def feed(self, raw: bytes):
        for b in raw:
            if self.state == 0:
                if b == FD_SOF_L:
                    self.state = 1
            elif self.state == 1:
                if b == FD_SOF_C:
                    self.state = 2
                elif b != FD_SOF_L:
                    self.state = 0
            elif self.state == 2:
                if 3 <= b <= 128:
                    self.expected_len = b
                    self.buf.clear()
                    self.state = 3
                else:
                    self.state = 0
            elif self.state == 3:
                self.buf.append(b)
                if len(self.buf) >= self.expected_len:
                    self.state = 4
            elif self.state == 4:
                self.csum_lsb = b
                self.state = 5
            elif self.state == 5:
                csum = self.csum_lsb | (b << 8)
                if calc_checksum(self.expected_len, self.buf) == csum:
                    cls = self.buf[0]
                    ptype = self.buf[1]
                    action = self.buf[2]
                    data = bytes(self.buf[3:])
                    self.on_packet(cls, ptype, action, data)
                self.state = 0


# =========================================================
# SIMULATOR GUI
# =========================================================
class AccessorySimulatorGUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Freeze Dry 3.0 - Accessory IOB Simulator (UART4)")
        self.geometry("1120x840")

        self.ser = None
        self.rx_running = False
        self.parser = FrameParser(self._on_rx_packet_thread)
        self.program_segments = {}

        self._build_top_bar()
        self._build_paned_layout()
        self.refresh_ports()
        self.on_profile_changed()

        self.after(1000, self._periodic_tick)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    # -----------------------------------------------------
    # UI BUILDERS
    # -----------------------------------------------------
    def _build_top_bar(self):
        top = ttk.Frame(self, padding=8)
        top.pack(fill=tk.X)

        ttk.Label(top, text="UART4 Port:").pack(side=tk.LEFT)
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(top, textvariable=self.port_var, width=12, state="readonly")
        self.port_combo.pack(side=tk.LEFT, padx=4)
        ttk.Button(top, text="Refresh", command=self.refresh_ports).pack(side=tk.LEFT, padx=2)

        ttk.Label(top, text="Baud:").pack(side=tk.LEFT, padx=(10, 2))
        self.baud_var = tk.StringVar(value="115200")
        ttk.Combobox(top, textvariable=self.baud_var, values=["9600", "115200"], width=8, state="readonly").pack(side=tk.LEFT)

        self.conn_btn = ttk.Button(top, text="Connect", command=self.toggle_connect)
        self.conn_btn.pack(side=tk.LEFT, padx=8)

        self.status_lbl = ttk.Label(top, text="Disconnected", foreground="red", font=("Segoe UI", 9, "bold"))
        self.status_lbl.pack(side=tk.LEFT, padx=4)

        ttk.Separator(top, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=10)

        ttk.Label(top, text="Accessory:").pack(side=tk.LEFT)
        self.profile_var = tk.StringVar(value="Stoppering Tray (ST)")
        prof_combo = ttk.Combobox(top, textvariable=self.profile_var, values=list(PROFILES.keys()), width=26, state="readonly")
        prof_combo.pack(side=tk.LEFT, padx=4)
        prof_combo.bind("<<ComboboxSelected>>", lambda _e: self.on_profile_changed())

        self.auto_stream_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(top, text="Auto-Stream Sensors (1s)", variable=self.auto_stream_var).pack(side=tk.LEFT, padx=8)

        self.auto_reply_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(top, text="Auto-Reply Main IOB", variable=self.auto_reply_var).pack(side=tk.LEFT, padx=4)

    def _build_paned_layout(self):
        paned = ttk.PanedWindow(self, orient=tk.VERTICAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=8, pady=(0, 8))

        notebook = ttk.Notebook(paned)
        paned.add(notebook, weight=3)

        self._build_control_tab(notebook)
        self._build_sensors_tab(notebook)
        self._build_system_alerts_tab(notebook)

        # Bottom resizable TX/RX Log
        log_frame = ttk.LabelFrame(paned, text="Accessory UART4 TX / RX Packet Log", padding=6)
        paned.add(log_frame, weight=2)

        log_bar = ttk.Frame(log_frame)
        log_bar.pack(fill=tk.X, pady=(0, 4))
        self.pause_log_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(log_bar, text="Pause Log", variable=self.pause_log_var).pack(side=tk.LEFT)
        ttk.Button(log_bar, text="Clear", command=self.clear_log).pack(side=tk.RIGHT)

        self.log_box = scrolledtext.ScrolledText(log_frame, height=12, font=("Consolas", 9), state=tk.DISABLED)
        self.log_box.pack(fill=tk.BOTH, expand=True)

    def _build_control_tab(self, nb):
        tab = ttk.Frame(nb, padding=10)
        nb.add(tab, text="Control & Program")

        # Left: Simulated Outputs (PUSH to Main IOB)
        left = ttk.LabelFrame(tab, text="Simulate Accessory -> Main IOB (CLASS_PUSH)", padding=10)
        left.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 8))

        # Manual Shelf Heat (0x08)
        row0 = ttk.Frame(left)
        row0.pack(fill=tk.X, pady=4)
        ttk.Label(row0, text="Manual Shelf Heat (0x08):", width=24).pack(side=tk.LEFT)
        self.shelf_heat_on_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(row0, text="ON", variable=self.shelf_heat_on_var).pack(side=tk.LEFT, padx=4)
        ttk.Label(row0, text="Target (°C):").pack(side=tk.LEFT, padx=(8, 2))
        self.shelf_target_var = tk.DoubleVar(value=25.0)
        ttk.Spinbox(row0, from_=-60.0, to=65.0, increment=0.5, textvariable=self.shelf_target_var, width=7).pack(side=tk.LEFT)
        ttk.Button(row0, text="Push 0x08", command=self.push_shelf_heat).pack(side=tk.RIGHT)

        ttk.Separator(left, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=8)

        # Program Status (0x0C)
        p1 = ttk.Frame(left)
        p1.pack(fill=tk.X, pady=4)
        ttk.Label(p1, text="Program Seg No:", width=16).pack(side=tk.LEFT)
        self.prog_seg_var = tk.IntVar(value=1)
        ttk.Spinbox(p1, from_=1, to=16, textvariable=self.prog_seg_var, width=5).pack(side=tk.LEFT, padx=4)

        ttk.Label(p1, text="Time Left (min):").pack(side=tk.LEFT, padx=(10, 2))
        self.prog_time_var = tk.IntVar(value=30)
        ttk.Spinbox(p1, from_=0, to=9999, textvariable=self.prog_time_var, width=6).pack(side=tk.LEFT, padx=4)

        ttk.Label(p1, text="Phase:").pack(side=tk.LEFT, padx=(10, 2))
        self.prog_phase_var = tk.StringVar(value="0: Ramp")
        ttk.Combobox(p1, textvariable=self.prog_phase_var, values=["0: Ramp", "1: Hold"], width=9, state="readonly").pack(side=tk.LEFT)

        p2 = ttk.Frame(left)
        p2.pack(fill=tk.X, pady=6)
        self.prog_auto_tick_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(p2, text="Auto-Tick Program (1s = 1 min)", variable=self.prog_auto_tick_var).pack(side=tk.LEFT)
        ttk.Button(p2, text="Push Program Status (0x0C)", command=self.push_program_status).pack(side=tk.RIGHT)

        p3 = ttk.Frame(left)
        p3.pack(fill=tk.X, pady=4)
        ttk.Button(p3, text="Push Seg Complete (Alert 0x18)", command=self.push_seg_complete).pack(side=tk.LEFT, padx=(0, 6))
        ttk.Button(p3, text="Push Prog Complete (Alert 0x17)", command=self.push_prog_complete).pack(side=tk.LEFT)

        # Right: Commands Received FROM Main IOB
        right = ttk.LabelFrame(tab, text="Received Commands from Main IOB (UART4 RX)", padding=10)
        right.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        self.rx_shelf_lbl = ttk.Label(right, text="Last 0x08 Shelf Heat: (none)", font=("Consolas", 10))
        self.rx_shelf_lbl.pack(anchor=tk.W, pady=4)

        self.rx_run_lbl = ttk.Label(right, text="Last 0x0B Program Run: STOPPED", font=("Consolas", 10))
        self.rx_run_lbl.pack(anchor=tk.W, pady=4)

        ttk.Label(right, text="Loaded Program Segments (0x0A):").pack(anchor=tk.W, pady=(8, 2))
        self.seg_list = tk.Listbox(right, height=9, font=("Consolas", 9))
        self.seg_list.pack(fill=tk.BOTH, expand=True)

    def _build_sensors_tab(self, nb):
        tab = ttk.Frame(nb, padding=10)
        nb.add(tab, text="Sensors")

        top_row = ttk.Frame(tab)
        top_row.pack(fill=tk.X, pady=(0, 8))
        ttk.Button(top_row, text="Push All Active Sensors Now", command=self.push_all_sensors).pack(side=tk.LEFT)
        ttk.Button(top_row, text="Push HW Detect (0x20)", command=self.push_hw_detect).pack(side=tk.LEFT, padx=8)

        grid = ttk.Frame(tab)
        grid.pack(fill=tk.BOTH, expand=True)

        # Shelves 1-5 & Samples 1-5
        self.shelf_vars = []
        self.sample_vars = []
        for i in range(5):
            r = ttk.Frame(grid)
            r.pack(fill=tk.X, pady=2)

            sv = tk.DoubleVar(value=-20.0 + i * 1.5)
            self.shelf_vars.append(sv)
            ttk.Label(r, text=f"Shelf {i+1} Temp (0x{SENSOR_SHELF1_TEMP+i:02X}) °C:", width=26).pack(side=tk.LEFT)
            ttk.Spinbox(r, from_=-90.0, to=80.0, increment=0.5, textvariable=sv, width=7).pack(side=tk.LEFT, padx=4)
            ttk.Button(r, text="Push", command=lambda idx=i: self.push_temp(SENSOR_SHELF1_TEMP + idx, self.shelf_vars[idx].get())).pack(side=tk.LEFT, padx=(0, 16))

            smv = tk.DoubleVar(value=-18.0 + i * 1.5)
            self.sample_vars.append(smv)
            ttk.Label(r, text=f"Sample {i+1} (0x{SENSOR_SHELF1_SAMPLE_TEMP+i:02X}) °C:", width=22).pack(side=tk.LEFT)
            ttk.Spinbox(r, from_=-90.0, to=80.0, increment=0.5, textvariable=smv, width=7).pack(side=tk.LEFT, padx=4)
            ttk.Button(r, text="Push", command=lambda idx=i: self.push_temp(SENSOR_SHELF1_SAMPLE_TEMP + idx, self.sample_vars[idx].get())).pack(side=tk.LEFT)

        ttk.Separator(grid, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=8)

        # Lexsol, ST Vacuum, AC Power
        extra = ttk.Frame(grid)
        extra.pack(fill=tk.X, pady=4)

        self.lexsol_var = tk.DoubleVar(value=-35.0)
        ttk.Label(extra, text="ST Lexsol (0x03) °C:").pack(side=tk.LEFT)
        ttk.Spinbox(extra, from_=-90.0, to=65.0, increment=0.5, textvariable=self.lexsol_var, width=7).pack(side=tk.LEFT, padx=4)
        ttk.Button(extra, text="Push", command=lambda: self.push_temp(SENSOR_LEXSOL_TEMP, self.lexsol_var.get())).pack(side=tk.LEFT, padx=(0, 16))

        self.st_vac_var = tk.IntVar(value=120)
        ttk.Label(extra, text="ST Vacuum (0x1B) mbar*1000:").pack(side=tk.LEFT)
        ttk.Spinbox(extra, from_=0, to=65000, increment=10, textvariable=self.st_vac_var, width=8).pack(side=tk.LEFT, padx=4)
        ttk.Button(extra, text="Push", command=self.push_st_vacuum).pack(side=tk.LEFT, padx=(0, 16))

        self.ac_pwr_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(extra, text="AC Power ON (0x1E)", variable=self.ac_pwr_var).pack(side=tk.LEFT, padx=4)
        ttk.Button(extra, text="Push", command=self.push_ac_power).pack(side=tk.LEFT)

    def _build_system_alerts_tab(self, nb):
        tab = ttk.Frame(nb, padding=10)
        nb.add(tab, text="System & Alerts")

        sys_f = ttk.LabelFrame(tab, text="System Identity (TYPE_SYSTEM)", padding=10)
        sys_f.pack(fill=tk.X, pady=(0, 10))
        ttk.Button(sys_f, text="Push Accy FW Version (0x0A)", command=self.push_fw_version).pack(side=tk.LEFT, padx=4)
        ttk.Button(sys_f, text="Push Accy Model Details (0x0B)", command=self.push_model_details).pack(side=tk.LEFT, padx=4)

        al_f = ttk.LabelFrame(tab, text="Accessory Alerts (TYPE_ALERT)", padding=10)
        al_f.pack(fill=tk.X)

        self.door_open_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(al_f, text="Door Open", variable=self.door_open_var).pack(side=tk.LEFT, padx=4)
        ttk.Button(al_f, text="Push Door Status (0x19)", command=lambda: self.send_frame(CLASS_PUSH, TYPE_ALERT, ALERT_DOOR_STATUS, bytes([1 if self.door_open_var.get() else 0]))).pack(side=tk.LEFT, padx=6)
        ttk.Button(al_f, text="Push Power Fail Accy (0x1B)", command=lambda: self.send_frame(CLASS_PUSH, TYPE_ALERT, ALERT_POWER_FAIL_ACCESSORY, b"\x01")).pack(side=tk.LEFT, padx=6)
        ttk.Button(al_f, text="Push Shelf Temp Var (0x03)", command=lambda: self.send_frame(CLASS_PUSH, TYPE_ALERT, ALERT_SHELF_TEMP_VAR, b"\x01")).pack(side=tk.LEFT, padx=6)
        ttk.Button(al_f, text="Push Temp Probe Err (0x0A)", command=lambda: self.send_frame(CLASS_PUSH, TYPE_ALERT, ALERT_TEMP_PROBE_ERROR, b"\x01")).pack(side=tk.LEFT, padx=6)

    # -----------------------------------------------------
    # SERIAL CONNECTION & RX THREAD
    # -----------------------------------------------------
    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_combo["values"] = ports
        if ports and self.port_var.get() not in ports:
            self.port_var.set(ports[0])

    def toggle_connect(self):
        if self.ser and self.ser.is_open:
            self.rx_running = False
            self.ser.close()
            self.ser = None
            self.conn_btn.configure(text="Connect")
            self.status_lbl.configure(text="Disconnected", foreground="red")
            return

        port = self.port_var.get()
        if not port:
            return
        try:
            self.ser = serial.Serial(port, int(self.baud_var.get()), timeout=0.05)
            self.rx_running = True
            threading.Thread(target=self._rx_loop, daemon=True).start()
            self.conn_btn.configure(text="Disconnect")
            self.status_lbl.configure(text=f"Connected ({port})", foreground="green")
        except Exception as exc:
            self.status_lbl.configure(text=f"Error: {exc}", foreground="red")

    def _rx_loop(self):
        while self.rx_running and self.ser and self.ser.is_open:
            try:
                data = self.ser.read(256)
                if data:
                    self.parser.feed(data)
            except Exception:
                break

    def _on_rx_packet_thread(self, cls: int, ptype: int, action: int, data: bytes):
        self.after(0, lambda: self._handle_rx_packet(cls, ptype, action, data))

    def _handle_rx_packet(self, cls: int, ptype: int, action: int, data: bytes):
        cname = CLASS_NAMES.get(cls, f"0x{cls:02X}")
        tname = TYPE_NAMES.get(ptype, f"0x{ptype:02X}")
        self._log(f"RX <- [{cname}][{tname}] action=0x{action:02X} data={data.hex(' ')}")

        if ptype == TYPE_CONTROL and cls == CLASS_WRITE:
            if action == CONTROL_MANUAL_SHELF_HEAT and len(data) >= 3:
                flags, target_dC = struct.unpack("<Bh", data[:3])
                on = (flags & 0x0F) != 0
                self.shelf_heat_on_var.set(on)
                self.shelf_target_var.set(target_dC / 10.0)
                self.rx_shelf_lbl.configure(text=f"Last 0x08 Shelf Heat: {'ON' if on else 'OFF'} | target={target_dC/10.0:.1f} C")
                if self.auto_reply_var.get():
                    self.push_shelf_heat()

            elif action == CONTROL_PROGRAM_SEGMENTS and len(data) >= 10:
                seg_no, flags, temp_dC, ramp, vac, hold_m = struct.unpack("<BBhHHH", data[:10])
                self.program_segments[seg_no] = (flags, temp_dC, ramp, vac, hold_m)
                self._refresh_seg_list()
                if seg_no == self.prog_seg_var.get():
                    self.prog_time_var.set(hold_m)

            elif action == CONTROL_PROGRAM_RUN and len(data) >= 1:
                running = data[0] != 0
                self.rx_run_lbl.configure(text=f"Last 0x0B Program Run: {'RUNNING' if running else 'STOPPED'}")
                self.prog_auto_tick_var.set(running)
                if self.auto_reply_var.get():
                    self.push_program_status()

        elif ptype == TYPE_CONTROL and cls == CLASS_READ and self.auto_reply_var.get():
            if action == CONTROL_MANUAL_SHELF_HEAT:
                self.push_shelf_heat()
            elif action == CONTROL_PROGRAM_STATUS:
                self.push_program_status()

        elif ptype == TYPE_SYSTEM and cls == CLASS_READ and self.auto_reply_var.get():
            if action == SYSTEM_FIRMWARE_VERSION_ACCY:
                self.push_fw_version()
            elif action == SYSTEM_MODEL_DETAILS_ACCY:
                self.push_model_details()

        elif ptype == TYPE_SENSORS and cls == CLASS_READ and self.auto_reply_var.get():
            if action == SENSOR_HARDWARE_SENSOR_DETECTION_ACCY:
                self.push_hw_detect()

    def _refresh_seg_list(self):
        self.seg_list.delete(0, tk.END)
        for seg_no in sorted(self.program_segments.keys()):
            flags, temp_dC, ramp, vac, hold_m = self.program_segments[seg_no]
            self.seg_list.insert(
                tk.END,
                f"Seg {seg_no:02d} | flags=0x{flags:02X} | {temp_dC/10.0:5.1f}C | ramp={ramp/100.0:.2f} | vac={vac} | {hold_m}m",
            )

    # -----------------------------------------------------
    # TX HELPERS
    # -----------------------------------------------------
    def send_frame(self, cls: int, ptype: int, action: int, data: bytes = b""):
        frame = build_frame(cls, ptype, action, data)
        cname = CLASS_NAMES.get(cls, f"0x{cls:02X}")
        tname = TYPE_NAMES.get(ptype, f"0x{ptype:02X}")
        if self.ser and self.ser.is_open:
            self.ser.write(frame)
            self._log(f"TX -> [{cname}][{tname}] action=0x{action:02X} data={data.hex(' ')}")
        else:
            self._log(f"TX (NOT CONNECTED) -> [{cname}][{tname}] action=0x{action:02X} data={data.hex(' ')}")

    def push_shelf_heat(self):
        flags = 0x01 if self.shelf_heat_on_var.get() else 0x00
        target_dC = int(round(self.shelf_target_var.get() * 10.0))
        self.send_frame(CLASS_PUSH, TYPE_CONTROL, CONTROL_MANUAL_SHELF_HEAT, struct.pack("<Bh", flags, target_dC))

    def push_program_status(self):
        seg = int(self.prog_seg_var.get())
        tleft = int(self.prog_time_var.get())
        phase = 1 if self.prog_phase_var.get().startswith("1") else 0
        self.send_frame(CLASS_PUSH, TYPE_CONTROL, CONTROL_PROGRAM_STATUS, struct.pack("<BHB", seg, tleft, phase))

    def push_seg_complete(self):
        self.send_frame(CLASS_PUSH, TYPE_ALERT, ALERT_PROGRAM_SEGMENT_COMPLETE, bytes([int(self.prog_seg_var.get())]))

    def push_prog_complete(self):
        self.send_frame(CLASS_PUSH, TYPE_ALERT, ALERT_PROGRAM_COMPLETE, b"")

    def push_temp(self, action: int, temp_c: float):
        self.send_frame(CLASS_PUSH, TYPE_SENSORS, action, struct.pack("<h", int(round(temp_c * 10.0))))

    def push_st_vacuum(self):
        self.send_frame(CLASS_PUSH, TYPE_SENSORS, SENSOR_STOPPERING_TRAY_VACUUM, struct.pack("<H", int(self.st_vac_var.get())))

    def push_ac_power(self):
        self.send_frame(CLASS_PUSH, TYPE_SENSORS, SENSOR_DRYING_ACCESSORY_AC_POWER, bytes([1 if self.ac_pwr_var.get() else 0]))

    def push_hw_detect(self):
        prof = PROFILES[self.profile_var.get()]
        shelves_mask = (1 << prof["shelves"]) - 1
        samples_mask = (1 << prof["samples"]) - 1
        self.send_frame(CLASS_PUSH, TYPE_SENSORS, SENSOR_HARDWARE_SENSOR_DETECTION_ACCY, bytes([shelves_mask, samples_mask]))

    def push_fw_version(self):
        prof = PROFILES[self.profile_var.get()]
        self.send_frame(CLASS_PUSH, TYPE_SYSTEM, SYSTEM_FIRMWARE_VERSION_ACCY, bytes(prof["fw"]))

    def push_model_details(self):
        prof = PROFILES[self.profile_var.get()]
        self.send_frame(CLASS_PUSH, TYPE_SYSTEM, SYSTEM_MODEL_DETAILS_ACCY, prof["model"] + prof["special"] + prof["serial"])

    def push_all_sensors(self):
        prof = PROFILES[self.profile_var.get()]
        for i in range(prof["shelves"]):
            self.push_temp(SENSOR_SHELF1_TEMP + i, self.shelf_vars[i].get())
        for i in range(prof["samples"]):
            self.push_temp(SENSOR_SHELF1_SAMPLE_TEMP + i, self.sample_vars[i].get())
        if prof["has_lexsol"]:
            self.push_temp(SENSOR_LEXSOL_TEMP, self.lexsol_var.get())
        if prof["has_vacuum"]:
            self.push_st_vacuum()
        self.push_ac_power()

    def on_profile_changed(self):
        self._log(f"Profile switched -> {self.profile_var.get()}")

    # -----------------------------------------------------
    # TIMERS & LOGGING
    # -----------------------------------------------------
    def _periodic_tick(self):
        if self.ser and self.ser.is_open:
            if self.auto_stream_var.get():
                self.push_all_sensors()
            if self.prog_auto_tick_var.get():
                t = self.prog_time_var.get()
                if t > 0:
                    self.prog_time_var.set(t - 1)
                    self.push_program_status()
                else:
                    self.push_seg_complete()
                    self.prog_auto_tick_var.set(False)
                    self.push_prog_complete()
        self.after(1000, self._periodic_tick)

    def _log(self, msg: str):
        if self.pause_log_var.get():
            return
        ts = time.strftime("%H:%M:%S") + f".{int((time.time() % 1) * 1000):03d}"
        self.log_box.configure(state=tk.NORMAL)
        self.log_box.insert(tk.END, f"[{ts}] {msg}\n")
        self.log_box.see(tk.END)
        self.log_box.configure(state=tk.DISABLED)

    def clear_log(self):
        self.log_box.configure(state=tk.NORMAL)
        self.log_box.delete("1.0", tk.END)
        self.log_box.configure(state=tk.DISABLED)

    def _on_close(self):
        self.rx_running = False
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.destroy()


if __name__ == "__main__":
    app = AccessorySimulatorGUI()
    app.mainloop()
