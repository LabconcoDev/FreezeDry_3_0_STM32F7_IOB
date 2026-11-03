import serial
import serial.tools.list_ports
import threading
import struct
import tkinter as tk
from tkinter import ttk

# --- Protocol Definitions ---
CLASS_PUSH = 0x03
CLASS_WRITE = 0x02
TYPE_SENSORS = 0x01
TYPE_CONTROL = 0x02

# Map of the Triad Control Actions you want to test
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
    0x18: ("Shell/Dry Chamber Temp", "<h", "°C/10")
}

class TriadSimulatorDashboard(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Triad Machine Telemetry Simulator")
        self.geometry("800x600")
        self.configure(bg="#2b2b2b")
        
        self.header = ttk.Label(self, text="Waiting for FTDI connection...", font=("Arial", 14, "bold"), background="#2b2b2b", foreground="white")
        self.header.pack(pady=10)
        
        # Create a Canvas for drawing the Triad diagram
        self.canvas = tk.Canvas(self, bg="#333333", highlightthickness=0)
        self.canvas.pack(fill=tk.BOTH, expand=True, padx=20, pady=10)
        
        self.sensor_labels = {}
        self.sensor_text_ids = {}

        self.draw_triad_diagram()
        self.draw_control_panel()

        self.serial_port = None
        self.running = True
        
        # Connect to serial
        self.connect_to_serial()

    def draw_triad_diagram(self):
        # Modern Dark Theme Colors (Catppuccin Mocha inspired)
        bg_color = "#1E1E2E"
        panel_color = "#313244"
        accent_color = "#89B4FA"
        pipe_color = "#7F849C"
        text_val = "#A6E3A1"
        shadow_color = "#11111B"
        font_title = ("Segoe UI", 14, "bold")
        font_sub = ("Segoe UI", 9, "bold")
        
        self.configure(bg=bg_color)
        self.header.configure(background=bg_color, foreground=accent_color, font=("Segoe UI", 16, "bold"))
        self.canvas.configure(bg=bg_color)
        
        # Helper to draw a shadow + box
        def draw_box(x1, y1, x2, y2, fill, outline="", width=1, radius=0):
            self.canvas.create_rectangle(x1+6, y1+6, x2+6, y2+6, fill=shadow_color, outline="")
            self.canvas.create_rectangle(x1, y1, x2, y2, fill=fill, outline=outline, width=width)

        # Draw Pipes (Back layer)
        # Pipe: Sample to Collector (Moved down so it perfectly connects to the side of the Collector)
        draw_box(300, 310, 450, 340, pipe_color)
        # Flanges/Joints for industrial look
        draw_box(310, 300, 330, 350, "#585B70") # Left flange
        draw_box(430, 300, 450, 350, "#585B70") # Right flange

        # Pipe: Collector to Pump
        draw_box(530, 150, 560, 280, pipe_color)
        # Flanges
        draw_box(520, 150, 570, 170, "#585B70") # Top flange
        draw_box(520, 270, 570, 290, "#585B70") # Bottom flange

        # 1. Sample Chamber (Left)
        draw_box(60, 60, 320, 360, panel_color, outline=accent_color, width=2)
        # Glass reflection effect
        self.canvas.create_polygon(62, 62, 100, 62, 70, 358, 62, 358, fill="#45475A", outline="")
        self.canvas.create_text(190, 90, text="SAMPLE CHAMBER", fill=accent_color, font=font_title)
        
        # Shelf inside Sample Chamber
        draw_box(100, 210, 280, 230, "#585B70", outline="#A6ADC8")
        # Shelf brackets
        self.canvas.create_rectangle(120, 230, 130, 250, fill="#45475A", outline="")
        self.canvas.create_rectangle(250, 230, 260, 250, fill="#45475A", outline="")
        
        # Sample (A beaker shape on the shelf)
        self.canvas.create_oval(170, 160, 210, 210, fill="#F9E2AF", outline="#F38BA8", width=2)
        self.canvas.create_rectangle(180, 130, 200, 165, fill="#F9E2AF", outline="")
        self.canvas.create_line(180, 130, 180, 165, fill="#F38BA8", width=2)
        self.canvas.create_line(200, 130, 200, 165, fill="#F38BA8", width=2)
        self.canvas.create_oval(180, 125, 200, 135, fill="#F38BA8", outline="") # Beaker lip

        # 2. Collector Chamber (Bottom Right)
        draw_box(430, 290, 680, 530, panel_color, outline=accent_color, width=2)
        self.canvas.create_polygon(432, 292, 470, 292, 440, 528, 432, 528, fill="#45475A", outline="")
        self.canvas.create_text(555, 320, text="COLLECTOR CHAMBER\n(Cold Trap)", fill=accent_color, font=font_title, justify="center")
        
        # Ice coils inside collector
        self.canvas.create_line(460, 370, 650, 370, 460, 420, 650, 420, 460, 470, 650, 470, fill="#89DCEB", width=6, capstyle=tk.ROUND, joinstyle=tk.ROUND)

        # 3. Vacuum Pump (Top Right)
        draw_box(480, 60, 610, 150, "#F38BA8", outline="#eba0ac", width=2)
        self.canvas.create_text(545, 90, text="VACUUM PUMP", fill="#1E1E2E", font=("Segoe UI", 13, "bold"))

        # Add Sensor Displays inside sleek dark panels
        def add_sensor(action_id, title, x, y, width_pad=75):
            draw_box(x-width_pad, y-25, x+width_pad, y+25, "#181825", outline="#585B70", width=1)
            self.canvas.create_text(x, y-10, text=title.upper(), fill="#A6ADC8", font=font_sub)
            text_id = self.canvas.create_text(x, y+10, text="WAITING", fill=text_val, font=("Segoe UI", 13, "bold"))
            self.sensor_text_ids[action_id] = text_id
            self.sensor_labels[action_id] = tk.StringVar(value="---")

        # Overlay the sensors onto the diagram
        add_sensor(0x0C, "Shelf 1 Temp", 190, 260)
        add_sensor(0x11, "Sample 1 Temp", 190, 130)
        add_sensor(0x18, "Shell / Dry Chamber Temp", 190, 385, width_pad=100)
        
        add_sensor(0x01, "Collector Temp", 555, 490)
        add_sensor(0x03, "System Vacuum", 545, 200)
        
        # Tilt Switch
        draw_box(60, 450, 180, 510, panel_color, outline=accent_color, width=2)
        self.canvas.create_text(120, 470, text="TILT SWITCH", fill="#A6ADC8", font=font_sub)
        text_id = self.canvas.create_text(120, 490, text="WAITING", fill="#F38BA8", font=("Segoe UI", 12, "bold"))
        self.sensor_text_ids[0x17] = text_id
        self.sensor_labels[0x17] = tk.StringVar(value="---")

    def send_control_command(self, action_id, state):
        if not self.serial_port or not self.serial_port.is_open:
            print("Cannot send: Serial port not connected.")
            return

        data_byte = state # 1 for ON, 0 for OFF
        payload_len = 4   # cls(1) + type(1) + action(1) + data(1)
        
        # Calculate Checksum: Length + Payload Bytes
        csum = payload_len + CLASS_WRITE + TYPE_CONTROL + action_id + data_byte
        
        frame = bytearray([
            0x4C,  # 'L'
            0x43,  # 'C'
            payload_len,
            CLASS_WRITE,
            TYPE_CONTROL,
            action_id,
            data_byte,
            csum & 0xFF,        # Checksum LSB
            (csum >> 8) & 0xFF  # Checksum MSB
        ])
        
        self.serial_port.write(frame)
        print(f"Sent frame: {[hex(x) for x in frame]}")

    def draw_control_panel(self):
        # Create a frame for the buttons at the bottom of the window
        control_frame = tk.Frame(self, bg="#1E1E2E")
        control_frame.pack(fill=tk.X, padx=20, pady=10)
        
        tk.Label(control_frame, text="Triad Control Panel", font=("Arial", 12, "bold"), bg="#1E1E2E", fg="white").grid(row=0, column=0, columnspan=3, pady=5)
        
        row_idx = 1
        col_idx = 0
        
        for name, action_id in TRIAD_CONTROLS.items():
            # Label
            tk.Label(control_frame, text=name, bg="#1E1E2E", fg="#A6ACCD", width=20, anchor="e").grid(row=row_idx, column=col_idx, padx=5, pady=2)
            
            # ON Button (Sends 1)
            tk.Button(control_frame, text="ON", bg="#A6E3A1", fg="black", width=5, 
                      command=lambda a=action_id: self.send_control_command(a, 1)).grid(row=row_idx, column=col_idx+1, padx=2, pady=2)
            
            # OFF Button (Sends 0)
            tk.Button(control_frame, text="OFF", bg="#F38BA8", fg="black", width=5, 
                      command=lambda a=action_id: self.send_control_command(a, 0)).grid(row=row_idx, column=col_idx+2, padx=2, pady=2)
            
            row_idx += 1
            if row_idx > 4:  # Wrap to next column after 4 buttons
                row_idx = 1
                col_idx += 3

    def connect_to_serial(self):
        ports = serial.tools.list_ports.comports()
        if not ports:
            self.header.config(text="No COM Ports Found! (Waiting...)")
            self.after(2000, self.connect_to_serial)
            return
            
        try:
            port_name = ports[0].device
            self.serial_port = serial.Serial(port_name, 9600, timeout=1)
            self.header.config(text=f"Connected to TRIAD on {port_name}", foreground="#00FFCC")
            
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
                elif state == 4:
                    state = 0 
                    if len(payload) >= 3:
                        cls = payload[0]
                        typ = payload[1]
                        action = payload[2]
                        data = payload[3:]
                        
                        if cls == CLASS_PUSH and typ == TYPE_SENSORS:
                            self.update_gui(action, data)

            except Exception as e:
                print("Serial Error:", e)
                break

    def update_gui(self, action, data):
        if action in SENSOR_MAP:
            name, fmt, unit = SENSOR_MAP[action]
            try:
                val = struct.unpack(fmt, data)[0]
                
                if "Temp" in name:
                    display_str = f"{val / 10.0:.1f} {unit}"
                else:
                    display_str = f"{val} {unit}"
                    
                # Schedule Canvas update in main thread
                self.after(0, lambda a=action, v=display_str: self._update_canvas_text(a, v))
            except Exception as e:
                print(f"Failed to unpack action {action}: {e}")

    def _update_canvas_text(self, action_id, text):
        if action_id in self.sensor_text_ids:
            self.canvas.itemconfigure(self.sensor_text_ids[action_id], text=text)

if __name__ == "__main__":
    app = TriadSimulatorDashboard()
    app.mainloop()
