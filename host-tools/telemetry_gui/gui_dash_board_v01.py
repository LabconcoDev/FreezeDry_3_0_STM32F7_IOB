import serial
import serial.tools.list_ports
import threading
import struct
import tkinter as tk
from tkinter import ttk

# --- Protocol Definitions ---
CLASS_READ  = 0x01
CLASS_WRITE = 0x02
CLASS_PUSH  = 0x03
CLASS_ACK   = 0x04

TYPE_SENSORS = 0x01
TYPE_CONTROL = 0x02
TYPE_ALERT   = 0x03

# Program Actions
ACTION_PGM_START  = 0x30
ACTION_PGM_STOP   = 0x31
ACTION_PGM_UPDATE = 0x32
ACTION_PGM_STATUS = 0x30  # SENSOR_PROGRAM_STATUS (Type 0x01)

ACTION_ALERT_PGM_COMPLETE = 0x30
ACTION_ALERT_SEG_COMPLETE = 0x31

# Map of the Triad Manual Controls
TRIAD_CONTROLS = {
    "Tilt Valve (0x12)": 0x12,
    "Ref Valve (0x11)": 0x11,
    "Lexsol Pump (0x0F)": 0x0F,
    "Lexsol Heater (0x10)": 0x10,
    "5V Fan (0x13)": 0x13,
    "Refrig Delay (0x05)": 0x05,
    "Defrost Relay (0x07)": 0x07,
    "Vacuum Relay (0x02)": 0x02,
}

SENSOR_MAP = {
    0x01: ("Collector Temp", "<h", "°C/10"), 
    0x03: ("System Vacuum", "<i", "mBar"),
    0x0A: ("Moisture", "<B", "Status"),
    0x0C: ("Shelf 1 Temp", "<h", "°C/10"),
    0x11: ("Sample 1 Temp", "<h", "°C/10"),
    0x17: ("Tilt Switch", "<B", "Status"),
    0x18: ("Shell/Dry Chamber Temp", "<h", "°C/10"),
    0x19: ("Lexsol Temp", "<h", "°C/10")
}

class TriadSimulatorDashboard(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Triad Machine Telemetry & Program Manager")
        self.geometry("900x750")
        
        bg_color = "#1E1E2E"
        self.configure(bg=bg_color)

        self.header = ttk.Label(self, text="Waiting for FTDI connection...", font=("Segoe UI", 14, "bold"), background=bg_color, foreground="white")
        self.header.pack(pady=8)

        # Style the Notebook Tabs
        style = ttk.Style()
        style.theme_use('default')
        style.configure('TNotebook', background=bg_color, borderwidth=0)
        style.configure('TNotebook.Tab', background="#313244", foreground="#CDD6F4", padding=[15, 5], font=("Segoe UI", 10, "bold"))
        style.map('TNotebook.Tab', background=[('selected', '#89B4FA')], foreground=[('selected', '#11111B')])

        self.notebook = ttk.Notebook(self)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=15, pady=5)

        # Tab 1: Telemetry Chamber View & Manual Controls
        self.tab_diagram = tk.Frame(self.notebook, bg=bg_color)
        self.notebook.add(self.tab_diagram, text="Telemetry & Manual Controls")

        # Tab 2: Program & Recipe Manager
        self.tab_program = tk.Frame(self.notebook, bg=bg_color)
        self.notebook.add(self.tab_program, text="Program & Recipe Control")

        # Build UI Elements
        self.sensor_labels = {}
        self.sensor_text_ids = {}

        self.build_tab_diagram()
        self.build_tab_program()

        self.serial_port = None
        self.running = True
        
        # Connect to serial at 9600 BAUD
        self.connect_to_serial()

    # =========================================================================
    # TAB 1: TELEMETRY & CHAMBER DIAGRAM + CONTROLS
    # =========================================================================
    def build_tab_diagram(self):
        bg_color = "#1E1E2E"
        panel_color = "#313244"
        accent_color = "#89B4FA"
        pipe_color = "#7F849C"
        text_val = "#A6E3A1"
        shadow_color = "#11111B"
        font_title = ("Segoe UI", 13, "bold")
        font_sub = ("Segoe UI", 9, "bold")

        self.canvas = tk.Canvas(self.tab_diagram, bg=bg_color, highlightthickness=0, height=480)
        self.canvas.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        def draw_box(x1, y1, x2, y2, fill, outline="", width=1):
            self.canvas.create_rectangle(x1+4, y1+4, x2+4, y2+4, fill=shadow_color, outline="")
            self.canvas.create_rectangle(x1, y1, x2, y2, fill=fill, outline=outline, width=width)

        # Pipes
        draw_box(290, 240, 430, 265, pipe_color)
        draw_box(300, 230, 315, 275, "#585B70")
        draw_box(405, 230, 420, 275, "#585B70")

        draw_box(490, 110, 515, 210, pipe_color)
        draw_box(480, 110, 525, 125, "#585B70")
        draw_box(480, 195, 525, 210, "#585B70")

        # 1. Sample Chamber
        draw_box(40, 40, 290, 360, panel_color, outline=accent_color, width=2)
        self.canvas.create_polygon(42, 42, 80, 42, 50, 358, 42, 358, fill="#45475A", outline="")
        self.canvas.create_text(165, 65, text="SAMPLE CHAMBER", fill=accent_color, font=font_title)
        
        # Shelf inside Sample Chamber
        draw_box(80, 190, 250, 205, "#585B70", outline="#A6ADC8")
        self.canvas.create_rectangle(100, 205, 110, 220, fill="#45475A", outline="")
        self.canvas.create_rectangle(220, 205, 230, 220, fill="#45475A", outline="")
        
        # Sample Beaker
        self.canvas.create_oval(145, 140, 185, 190, fill="#F9E2AF", outline="#F38BA8", width=2)
        self.canvas.create_rectangle(155, 115, 175, 145, fill="#F9E2AF", outline="")
        self.canvas.create_line(155, 115, 155, 145, fill="#F38BA8", width=2)
        self.canvas.create_line(175, 115, 175, 145, fill="#F38BA8", width=2)
        self.canvas.create_oval(155, 110, 175, 120, fill="#F38BA8", outline="")

        # 2. Collector Chamber
        draw_box(410, 210, 660, 440, panel_color, outline=accent_color, width=2)
        self.canvas.create_polygon(412, 212, 450, 212, 420, 438, 412, 438, fill="#45475A", outline="")
        self.canvas.create_text(535, 235, text="COLLECTOR CHAMBER\n(Cold Trap)", fill=accent_color, font=font_title, justify="center")
        
        # Ice Coils
        self.canvas.create_line(440, 290, 630, 290, 440, 340, 630, 340, 440, 390, 630, 390, fill="#89DCEB", width=5, capstyle=tk.ROUND, joinstyle=tk.ROUND)

        # 3. Vacuum Pump
        draw_box(440, 40, 570, 110, "#F38BA8", outline="#EBA0AC", width=2)
        self.canvas.create_text(505, 65, text="VACUUM PUMP", fill="#1E1E2E", font=("Segoe UI", 11, "bold"))

        # Sensor Panels
        def add_sensor(action_id, title, x, y, width_pad=68):
            draw_box(x-width_pad, y-20, x+width_pad, y+20, "#181825", outline="#585B70", width=1)
            self.canvas.create_text(x, y-8, text=title.upper(), fill="#A6ADC8", font=font_sub)
            text_id = self.canvas.create_text(x, y+8, text="WAITING", fill=text_val, font=("Segoe UI", 11, "bold"))
            self.sensor_text_ids[action_id] = text_id
            self.sensor_labels[action_id] = tk.StringVar(value="---")

        add_sensor(0x0C, "Shelf 1 Temp", 165, 240)
        add_sensor(0x11, "Sample 1 Temp", 165, 125)
        add_sensor(0x18, "Shell / Chamber", 165, 330, width_pad=75)
        add_sensor(0x01, "Collector Temp", 535, 410)
        add_sensor(0x03, "System Vacuum", 505, 155)
        
        # Tilt Switch
        draw_box(40, 390, 150, 440, panel_color, outline=accent_color, width=2)
        self.canvas.create_text(95, 405, text="TILT SWITCH", fill="#A6ADC8", font=font_sub)
        text_id = self.canvas.create_text(95, 423, text="WAITING", fill="#F38BA8", font=("Segoe UI", 10, "bold"))
        self.sensor_text_ids[0x17] = text_id
        self.sensor_labels[0x17] = tk.StringVar(value="---")

        # Bottom Control Panel
        control_frame = tk.LabelFrame(self.tab_diagram, text=" Triad Manual Override Controls ", bg=panel_color, fg=accent_color, font=("Segoe UI", 10, "bold"), padx=10, pady=5)
        control_frame.pack(fill=tk.X, padx=10, pady=5)
        
        row_idx = 0
        col_idx = 0
        for name, action_id in TRIAD_CONTROLS.items():
            tk.Label(control_frame, text=name, bg=panel_color, fg="#CDD6F4", width=18, anchor="e", font=("Segoe UI", 9)).grid(row=row_idx, column=col_idx, padx=4, pady=2)
            tk.Button(control_frame, text="ON", bg="#A6E3A1", fg="black", width=4, relief="flat", font=("Segoe UI", 8, "bold"),
                      command=lambda a=action_id: self.send_control_command(a, 1)).grid(row=row_idx, column=col_idx+1, padx=2, pady=2)
            tk.Button(control_frame, text="OFF", bg="#F38BA8", fg="black", width=4, relief="flat", font=("Segoe UI", 8, "bold"),
                      command=lambda a=action_id: self.send_control_command(a, 0)).grid(row=row_idx, column=col_idx+2, padx=2, pady=2)
            
            row_idx += 1
            if row_idx > 3:
                row_idx = 0
                col_idx += 3

    # =========================================================================
    # TAB 2: PROGRAM & RECIPE MANAGER
    # =========================================================================
    def build_tab_program(self):
        panel = "#313244"
        accent = "#89B4FA"
        fg_text = "#CDD6F4"

        main_frame = tk.Frame(self.tab_program, bg="#1E1E2E")
        main_frame.pack(fill=tk.BOTH, expand=True, padx=15, pady=10)

        # --- Left Column: Settings ---
        left_box = tk.LabelFrame(main_frame, text=" Segment Settings (11-Byte Frame) ", bg=panel, fg=accent, font=("Segoe UI", 11, "bold"), padx=15, pady=10)
        left_box.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=10)

        def make_entry(parent, label_text, default_val, row):
            tk.Label(parent, text=label_text, bg=panel, fg=fg_text, font=("Segoe UI", 10)).grid(row=row, column=0, sticky="w", pady=4)
            ent = tk.Entry(parent, width=16, font=("Segoe UI", 10), bg="#181825", fg="white", insertbackground="white")
            ent.insert(0, default_val)
            ent.grid(row=row, column=1, sticky="e", pady=4, padx=5)
            return ent

        self.ent_seg  = make_entry(left_box, "Segment Number (1-10):", "1", 0)
        
        tk.Label(left_box, text="Operation Mode:", bg=panel, fg=fg_text, font=("Segoe UI", 10)).grid(row=1, column=0, sticky="w", pady=4)
        self.var_mode = tk.StringVar(value="3: Program")
        mode_cb = ttk.Combobox(left_box, textvariable=self.var_mode, values=["0: Off", "1: Manual", "2: Auto", "3: Program"], width=14, state="readonly")
        mode_cb.grid(row=1, column=1, sticky="e", pady=4, padx=5)

        self.ent_temp = make_entry(left_box, "Shelf Target Temp (°C):", "-10.0", 2)
        self.ent_ramp = make_entry(left_box, "Ramp Rate (°C/min):", "0.50", 3)
        self.ent_vac  = make_entry(left_box, "Vacuum Setpoint (mBar):", "0.200", 4)
        self.ent_time = make_entry(left_box, "Hold Time (minutes):", "10", 5)

        # Control Bit Checkboxes
        chk_box = tk.LabelFrame(left_box, text=" Control Flags (Byte 10) ", bg=panel, fg=accent, font=("Segoe UI", 9, "bold"), padx=10, pady=5)
        chk_box.grid(row=6, column=0, columnspan=2, sticky="ew", pady=10)

        self.chk_vac_on       = tk.BooleanVar(value=True)
        self.chk_vac_protect  = tk.BooleanVar(value=True)
        self.chk_max_cold     = tk.BooleanVar(value=False)
        self.chk_inf_hold     = tk.BooleanVar(value=False)
        self.chk_in_flask     = tk.BooleanVar(value=False)
        self.chk_ac_pause     = tk.BooleanVar(value=False)
        self.chk_warm_lexsol  = tk.BooleanVar(value=False)

        def make_chk(parent, text, var, r, c):
            tk.Checkbutton(parent, text=text, variable=var, bg=panel, fg=fg_text, selectcolor="#181825", activebackground=panel, activeforeground=accent).grid(row=r, column=c, sticky="w", pady=2)

        make_chk(chk_box, "Vac ON/OFF", self.chk_vac_on, 0, 0)
        make_chk(chk_box, "Vac Protect", self.chk_vac_protect, 0, 1)
        make_chk(chk_box, "Max Cold", self.chk_max_cold, 1, 0)
        make_chk(chk_box, "Infinite Hold", self.chk_inf_hold, 1, 1)
        make_chk(chk_box, "In Flask", self.chk_in_flask, 2, 0)
        make_chk(chk_box, "AC Power Pause", self.chk_ac_pause, 2, 1)
        make_chk(chk_box, "Warm Lexsol", self.chk_warm_lexsol, 3, 0)

        # Action Buttons
        btn_box = tk.Frame(left_box, bg=panel)
        btn_box.grid(row=7, column=0, columnspan=2, pady=10)

        tk.Button(btn_box, text="▶ START PROGRAM (0x30)", bg="#A6E3A1", fg="black", font=("Segoe UI", 10, "bold"), width=24, relief="flat",
                  command=lambda: self.send_program_frame(ACTION_PGM_START)).pack(pady=3)
        tk.Button(btn_box, text="⚡ UPDATE SEGMENT (0x32)", bg="#89B4FA", fg="black", font=("Segoe UI", 10, "bold"), width=24, relief="flat",
                  command=lambda: self.send_program_frame(ACTION_PGM_UPDATE)).pack(pady=3)
        tk.Button(btn_box, text="⏹ STOP PROGRAM (0x31)", bg="#F38BA8", fg="black", font=("Segoe UI", 10, "bold"), width=24, relief="flat",
                  command=lambda: self.send_program_stop()).pack(pady=3)

        # --- Right Column: Live Status & Alerts ---
        right_box = tk.LabelFrame(main_frame, text=" Live Program Status (From STM32) ", bg=panel, fg=accent, font=("Segoe UI", 11, "bold"), padx=15, pady=10)
        right_box.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=10)

        self.lbl_status_seg   = tk.Label(right_box, text="Active Segment: --", font=("Segoe UI", 12), bg=panel, fg=fg_text)
        self.lbl_status_seg.pack(anchor="w", pady=5)

        self.lbl_status_mode  = tk.Label(right_box, text="Current Mode: --", font=("Segoe UI", 12), bg=panel, fg=fg_text)
        self.lbl_status_mode.pack(anchor="w", pady=5)

        self.lbl_status_phase = tk.Label(right_box, text="Phase: --", font=("Segoe UI", 12), bg=panel, fg=fg_text)
        self.lbl_status_phase.pack(anchor="w", pady=5)

        self.lbl_status_time  = tk.Label(right_box, text="Time Remaining: --:--", font=("Segoe UI", 16, "bold"), bg=panel, fg="#F9E2AF")
        self.lbl_status_time.pack(anchor="w", pady=10)

        # Event & Alert Log Box
        tk.Label(right_box, text="Program Alert Log:", font=("Segoe UI", 10, "bold"), bg=panel, fg=accent).pack(anchor="w", pady=(10, 2))
        self.alert_log = tk.Text(right_box, height=12, bg="#181825", fg="#A6ADC8", font=("Consolas", 9), relief="flat")
        self.alert_log.pack(fill=tk.BOTH, expand=True)

    # =========================================================================
    # PROTOCOL SERIALIZATION & SENDING
    # =========================================================================
    def build_frame(self, cls, typ, action, data):
        payload_len = 3 + len(data) # cls(1) + type(1) + action(1) + data(N)
        csum = payload_len + cls + typ + action + sum(data)
        
        frame = bytearray([0x4C, 0x43, payload_len, cls, typ, action])
        frame.extend(data)
        frame.append(csum & 0xFF)
        frame.append((csum >> 8) & 0xFF)
        return frame

    def send_control_command(self, action_id, state):
        if not self.serial_port or not self.serial_port.is_open:
            print("Cannot send: Serial not connected.")
            return

        frame = self.build_frame(CLASS_WRITE, TYPE_CONTROL, action_id, bytearray([state]))
        self.serial_port.write(frame)
        print(f"Sent Control Frame: {[hex(x) for x in frame]}")

    def send_program_frame(self, action_code):
        if not self.serial_port or not self.serial_port.is_open:
            print("Cannot send: Serial not connected.")
            return

        try:
            seg = int(self.ent_seg.get())
            mode = int(self.var_mode.get().split(":")[0])
            temp_x10 = int(float(self.ent_temp.get()) * 10)
            ramp_x100 = int(float(self.ent_ramp.get()) * 100)
            vac_x1000 = int(float(self.ent_vac.get()) * 1000)
            hold_mins = int(self.ent_time.get())

            flags = 0
            if self.chk_vac_on.get():      flags |= (1 << 0)
            if self.chk_vac_protect.get(): flags |= (1 << 1)
            if self.chk_max_cold.get():    flags |= (1 << 2)
            if self.chk_inf_hold.get():    flags |= (1 << 3)
            if self.chk_in_flask.get():    flags |= (1 << 4)
            if self.chk_ac_pause.get():    flags |= (1 << 5)
            if self.chk_warm_lexsol.get(): flags |= (1 << 6)

            # Pack 11 Bytes Big-Endian (> = Big-Endian, B=uint8, h=int16, H=uint16)
            payload_data = struct.pack(">BBhhHHB", seg, mode, temp_x10, ramp_x100, vac_x1000, hold_mins, flags)
            
            frame = self.build_frame(CLASS_WRITE, TYPE_CONTROL, action_code, payload_data)
            self.serial_port.write(frame)
            
            action_name = "START" if action_code == ACTION_PGM_START else "UPDATE"
            self.log_alert(f"[TX] Sent {action_name} for Segment {seg} ({len(frame)} bytes)")
            print(f"Sent Program Frame: {[hex(x) for x in frame]}")
        except Exception as e:
            self.log_alert(f"[ERROR] Failed to pack program: {e}")

    def send_program_stop(self):
        if not self.serial_port or not self.serial_port.is_open: return
        try:
            seg = int(self.ent_seg.get())
            frame = self.build_frame(CLASS_WRITE, TYPE_CONTROL, ACTION_PGM_STOP, bytearray([seg]))
            self.serial_port.write(frame)
            self.log_alert(f"[TX] Sent STOP for Segment {seg}")
        except Exception as e:
            self.log_alert(f"[ERROR] Failed to send STOP: {e}")

    def log_alert(self, msg):
        self.alert_log.insert(tk.END, msg + "\n")
        self.alert_log.see(tk.END)

    # =========================================================================
    # SERIAL COMMUNICATION THREAD (9600 BAUD)
    # =========================================================================
    def connect_to_serial(self):
        ports = serial.tools.list_ports.comports()
        if not ports:
            self.header.config(text="No COM Ports Found! (Waiting...)")
            self.after(2000, self.connect_to_serial)
            return
            
        try:
            port_name = ports[0].device
            # 9600 BAUD MATCHES STM32 UART7 CONFIGURATION
            self.serial_port = serial.Serial(port_name, 9600, timeout=1)
            self.header.config(text=f"Connected to TRIAD on {port_name} (9600 baud)", foreground="#00FFCC")
            threading.Thread(target=self.serial_reader_thread, daemon=True).start()
        except Exception as e:
            self.header.config(text=f"Connection Error: {str(e)}", foreground="red")
            self.after(2000, self.connect_to_serial)

    def serial_reader_thread(self):
        state = 0
        payload_len = 0
        payload = bytearray()
        
        while self.running:
            try:
                byte = self.serial_port.read(1)
                if not byte: continue
                b = byte[0]
                
                if state == 0:
                    if b == 0x4C: state = 1 
                elif state == 1:
                    if b == 0x43: state = 2 
                    else: state = 0
                elif state == 2:
                    payload_len = b
                    payload = bytearray()
                    state = 3
                elif state == 3:
                    payload.append(b)
                    if len(payload) == payload_len:
                        state = 4 
                elif state == 4: # First checksum byte (LSB)
                    state = 5
                elif state == 5: # Second checksum byte (MSB)
                    state = 0 
                    if len(payload) >= 3:
                        cls = payload[0]
                        typ = payload[1]
                        action = payload[2]
                        data = payload[3:]
                        
                        # Route incoming messages
                        if cls == CLASS_PUSH:
                            if typ == TYPE_SENSORS:
                                if action == ACTION_PGM_STATUS and len(data) >= 4:
                                    self.after(0, lambda d=data: self.update_program_status(d))
                                else:
                                    self.update_gui(action, data)
                            elif typ == TYPE_ALERT:
                                self.after(0, lambda a=action, d=data: self.handle_incoming_alert(a, d))

            except Exception as e:
                print("Serial Error:", e)
                break

    def update_program_status(self, data):
        seg = data[0]
        mode = data[1] & 0x0F
        phase = (data[1] >> 4) & 0x0F
        time_rem = data[2] | (data[3] << 8)

        modes = ["OFF", "MANUAL", "AUTO", "PROGRAM"]
        mode_str = modes[mode] if mode < len(modes) else f"Unknown ({mode})"
        phase_str = "HOLD Phase" if phase == 0 else "RAMP Phase"
        hours, mins = divmod(time_rem, 60)

        self.lbl_status_seg.config(text=f"Active Segment: {seg}")
        self.lbl_status_mode.config(text=f"Current Mode: {mode_str}")
        self.lbl_status_phase.config(text=f"Phase: {phase_str}")
        self.lbl_status_time.config(text=f"Time Remaining: {hours:02d}:{mins:02d} ({time_rem} mins)")

    def handle_incoming_alert(self, action, data):
        seg = data[0] if len(data) > 0 else 0
        if action == ACTION_ALERT_SEG_COMPLETE:
            self.log_alert(f"[ALERT] Segment {seg} COMPLETE! Sending next segment...")
        elif action == ACTION_ALERT_PGM_COMPLETE:
            self.log_alert(f"[ALERT] Entire Program COMPLETE at Segment {seg}!")
        else:
            self.log_alert(f"[ALERT] Unknown Alert: Action=0x{action:02X}, Data={list(data)}")

    def update_gui(self, action, data):
        if action in SENSOR_MAP:
            name, fmt, unit = SENSOR_MAP[action]
            try:
                val = struct.unpack(fmt, data)[0]
                if "Temp" in name:
                    display_str = f"{val / 10.0:.1f} {unit}"
                else:
                    display_str = f"{val} {unit}"
                self.after(0, lambda a=action, v=display_str: self._update_canvas_text(a, v))
            except Exception as e:
                pass

    def _update_canvas_text(self, action_id, text):
        if action_id in self.sensor_text_ids:
            self.canvas.itemconfigure(self.sensor_text_ids[action_id], text=text)

if __name__ == "__main__":
    app = TriadSimulatorDashboard()
    app.mainloop()