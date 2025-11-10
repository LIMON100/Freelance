import tkinter as tk
from tkinter import ttk, scrolledtext
import socket
import threading
import struct
import time
import queue
import subprocess
import os
import signal  # NEW: Import the signal module
from datetime import datetime

# --- Configuration ---
HOST_IP = '0.0.0.0'
COMMAND_TCP_PORT = 65432
TOUCH_UDP_PORT = 65433
DRIVING_UDP_PORT = 65434
STATUS_TCP_PORT = 65435
RTSP_PUSH_PORT = 8554

class RobotSimulatorGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Robot Server Simulator")
        self.root.geometry("800x600")

        # --- Theming (Same as before) ---
        self.style = ttk.Style(self.root)
        self.style.theme_use('clam')
        self.style.configure('.', background='#2E2E2E', foreground='white', fieldbackground='#3E3E3E')
        self.style.configure('TLabel', background='#2E2E2E', foreground='white')
        self.style.configure('TButton', background='#4A4A4A', foreground='white')
        self.style.configure('TLabelframe', background='#2E2E2E', bordercolor='#555')
        self.style.configure('TLabelframe.Label', background='#2E2E2E', foreground='cyan')
        self.style.map('TButton', background=[('active', '#5A5A5A')])
        self.style.configure('Active.TButton', background='green', foreground='white')
        self.style.map('Active.TButton', background=[('active', 'dark green')])

        # --- Shared State ---
        self.robot_state = { "rtsp_running": 1, "current_mode_id": 0, "permission_request_active": 0, "crosshair_x": -1.0, "crosshair_y": -1.0, "move_speed": 0, "turn_angle": 0, "pan_speed": 0, "tilt_speed": 0, "zoom_command": 0, "attack_permission": 0, "lateral_wind_speed": 0.0, "touch_x": -1.0, "touch_y": -1.0 }
        self.running = True
        self.gui_queue = queue.Queue()
        self.sockets_to_close = []
        self.gstreamer_processes = []

        self.create_widgets()
        self.start_threads()

        # --- MODIFIED: Handle both window close and Ctrl+C ---
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        signal.signal(signal.SIGINT, self.signal_handler)

        self.process_gui_queue()
        self.log_message("Simulator GUI Initialized. Close with 'X' or Ctrl+C.")

    # --- NEW: Custom signal handler for Ctrl+C ---
    def signal_handler(self, sig, frame):
        print("\nCtrl+C detected!")
        self.on_closing()

    def create_widgets(self):
        # ... (This function is unchanged)
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky="nsew")
        self.root.grid_rowconfigure(0, weight=1)
        self.root.grid_columnconfigure(0, weight=1)
        top_frame = ttk.Frame(main_frame)
        top_frame.grid(row=0, column=0, sticky="ew")
        main_frame.grid_columnconfigure(0, weight=1)
        app_data_frame = ttk.LabelFrame(top_frame, text="Data From App", padding="10")
        app_data_frame.grid(row=0, column=0, padx=5, pady=5, sticky="nsw")
        top_frame.grid_columnconfigure(0, weight=1)
        control_frame = ttk.LabelFrame(top_frame, text="Simulator Controls (Send to App)", padding="10")
        control_frame.grid(row=0, column=1, padx=5, pady=5, sticky="nsew")
        top_frame.grid_columnconfigure(1, weight=1)
        log_frame = ttk.LabelFrame(main_frame, text="Live Log", padding="10")
        log_frame.grid(row=1, column=0, sticky="nsew", pady=10)
        main_frame.grid_rowconfigure(1, weight=1)
        self.log_text = scrolledtext.ScrolledText(log_frame, wrap=tk.WORD, height=10, bg="#1E1E1E", fg="light grey")
        self.log_text.pack(fill="both", expand=True)
        self.labels = {}
        row = 0
        for key in ["current_mode_id", "move_speed", "turn_angle", "pan_speed", "tilt_speed", "zoom_command", "attack_permission", "lateral_wind_speed", "touch_x", "touch_y"]:
            ttk.Label(app_data_frame, text=f"{key.replace('_', ' ').title()}:").grid(row=row, column=0, sticky="w", pady=2)
            self.labels[key] = ttk.Label(app_data_frame, text="-", width=10)
            self.labels[key].grid(row=row, column=1, sticky="w", padx=10)
            row += 1
        self.permission_button = ttk.Button(control_frame, text="Request Attack Permission", command=self.toggle_permission_request)
        self.permission_button.grid(row=0, column=0, columnspan=2, pady=10)
        ttk.Label(control_frame, text="Crosshair X:").grid(row=1, column=0, sticky="w")
        self.crosshair_x_label = ttk.Label(control_frame, text="-1.00", width=5)
        self.crosshair_x_label.grid(row=1, column=1, sticky="e")
        self.crosshair_x_var = tk.DoubleVar(value=-1.0)
        self.crosshair_x_slider = ttk.Scale(control_frame, from_=-1.0, to=1.0, orient="horizontal", variable=self.crosshair_x_var, command=self.update_crosshair)
        self.crosshair_x_slider.grid(row=2, column=0, columnspan=2, pady=5, sticky="ew")
        ttk.Label(control_frame, text="Crosshair Y:").grid(row=3, column=0, sticky="w")
        self.crosshair_y_label = ttk.Label(control_frame, text="-1.00", width=5)
        self.crosshair_y_label.grid(row=3, column=1, sticky="e")
        self.crosshair_y_var = tk.DoubleVar(value=-1.0)
        self.crosshair_y_slider = ttk.Scale(control_frame, from_=-1.0, to=1.0, orient="horizontal", variable=self.crosshair_y_var, command=self.update_crosshair)
        self.crosshair_y_slider.grid(row=4, column=0, columnspan=2, pady=5, sticky="ew")

    def on_closing(self):
        # This is now the single point of truth for shutdown
        if not self.running: return # Prevent double-calls
        
        self.log_message("Shutdown signal received. Terminating threads and processes...")
        self.running = False
        
        # Terminate GStreamer subprocesses
        for p in self.gstreamer_processes:
            p.terminate()

        # Close all network sockets to unblock threads
        for s in self.sockets_to_close:
            s.close()

        # Give threads a moment to exit their loops
        self.root.after(200, self.root.destroy)
        print("Simulator shutting down...")

    # ... All other GUI methods like process_gui_queue, log_message, update_state, etc. are unchanged ...
    def log_message(self, message):
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.gui_queue.put(('log', f"[{timestamp}] {message}"))

    def process_gui_queue(self):
        try:
            while not self.gui_queue.empty():
                msg_type, value = self.gui_queue.get_nowait()
                if msg_type == 'log':
                    self.log_text.insert(tk.END, value + "\n")
                    self.log_text.see(tk.END)
                elif msg_type in self.labels:
                    if isinstance(value, float):
                        self.labels[msg_type].config(text=f"{value:.2f}")
                    else:
                        self.labels[msg_type].config(text=str(value))
        finally:
            self.root.after(100, self.process_gui_queue)

    def toggle_permission_request(self):
        new_state = 1 - self.robot_state["permission_request_active"]
        self.robot_state["permission_request_active"] = new_state
        self.log_message(f"Toggled permission request to: {'ACTIVE' if new_state else 'INACTIVE'}")
        if new_state:
            self.permission_button.config(text="Cancel Permission Request", style="Active.TButton")
        else:
            self.permission_button.config(text="Request Attack Permission", style="TButton")
            self.gui_queue.put(("attack_permission", 0))

    def update_crosshair(self, _):
        self.robot_state["crosshair_x"] = self.crosshair_x_var.get()
        self.robot_state["crosshair_y"] = self.crosshair_y_var.get()
        self.crosshair_x_label.config(text=f"{self.robot_state['crosshair_x']:.2f}")
        self.crosshair_y_label.config(text=f"{self.robot_state['crosshair_y']:.2f}")

    def start_threads(self):
        self.threads = [ threading.Thread(target=self.tcp_command_thread), threading.Thread(target=self.udp_driving_thread), threading.Thread(target=self.udp_touch_thread), threading.Thread(target=self.tcp_status_thread), threading.Thread(target=self.rtsp_stream_thread) ]
        for t in self.threads:
            t.start()
        
    # --- NETWORK THREADS (Slightly modified for clean shutdown) ---
    def tcp_command_thread(self):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((HOST_IP, COMMAND_TCP_PORT))
                self.sockets_to_close.append(s)
                s.listen()
                self.log_message(f"Listening for Commands on TCP {HOST_IP}:{COMMAND_TCP_PORT}")
                while self.running:
                    conn, addr = s.accept()
                    with conn:
                        data = conn.recv(9)
                        if data and len(data) == 9:
                            cmd_id, atk_perm, pan, tilt, zoom, wind = struct.unpack('<BBbbbf', data)
                            self.gui_queue.put(("current_mode_id", cmd_id))
                            self.gui_queue.put(("attack_permission", atk_perm))
                            self.gui_queue.put(("pan_speed", pan))
                            self.gui_queue.put(("tilt_speed", tilt))
                            self.gui_queue.put(("zoom_command", zoom))
                            self.gui_queue.put(("lateral_wind_speed", wind))
        except OSError:
            if self.running: self.log_message("Command TCP socket error or closed.")
        finally:
            self.log_message("Command TCP thread terminated.")


    def _generic_udp_thread(self, port, name, unpack_format, keys, log_prefix):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((HOST_IP, port))
                self.sockets_to_close.append(s)
                self.log_message(f"Listening for {name} on UDP {HOST_IP}:{port}")
                while self.running:
                    data, _ = s.recvfrom(1024)
                    if data:
                        values = struct.unpack(unpack_format, data)
                        log_parts = []
                        for i, key in enumerate(keys):
                            self.gui_queue.put((key, values[i]))
                            log_parts.append(f"{key}={values[i]:.2f}" if isinstance(values[i], float) else f"{key}={values[i]}")
                        self.log_message(f"Rcvd {log_prefix}: {', '.join(log_parts)}")
        except OSError:
            if self.running: self.log_message(f"{name} UDP socket error or closed.")
        finally:
            self.log_message(f"{name} UDP thread terminated.")

    def udp_driving_thread(self):
        self._generic_udp_thread(DRIVING_UDP_PORT, "Driving", '<bb', ["move_speed", "turn_angle"], "DRV")

    def udp_touch_thread(self):
        self._generic_udp_thread(TOUCH_UDP_PORT, "Touch", '<ff', ["touch_x", "touch_y"], "TCH")

    def tcp_status_thread(self):
        clients = []
        def accept_clients(server_socket):
            while self.running:
                try:
                    conn, addr = server_socket.accept()
                    self.log_message(f"Status client connected: {addr}")
                    clients.append(conn)
                except OSError:
                    break
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((HOST_IP, STATUS_TCP_PORT))
                self.sockets_to_close.append(s)
                s.listen()
                self.log_message(f"Listening for Status clients on TCP {HOST_IP}:{STATUS_TCP_PORT}")
                
                accept_thread = threading.Thread(target=accept_clients, args=(s,))
                accept_thread.start()

                while self.running:
                    try:
                        mode_id = int(self.labels["current_mode_id"].cget("text"))
                    except ValueError:
                        mode_id = 0 # Default to IDLE if text is not a valid int yet
                    
                    packet = struct.pack('<BBBff', self.robot_state["rtsp_running"], mode_id, self.robot_state["permission_request_active"], self.robot_state["crosshair_x"], self.robot_state["crosshair_y"])
                    
                    disconnected_clients = []
                    for client in clients:
                        try: client.sendall(packet)
                        except (ConnectionResetError, BrokenPipeError, OSError): disconnected_clients.append(client)
                    
                    for client in disconnected_clients:
                        self.log_message(f"Status client disconnected.")
                        clients.remove(client)
                    time.sleep(0.1)
        except OSError:
            if self.running: self.log_message("Status TCP socket error or closed.")
        finally:
            self.log_message("Status TCP thread terminated.")
    
    def rtsp_stream_thread(self):
        # ... (This function is unchanged)
        self.log_message("Checking for GStreamer and rtsp-simple-server...")
        if not os.path.exists('./rtsp-simple-server'):
            self.log_message("ERROR: rtsp-simple-server executable not found in the same directory.")
            self.log_message("Please download it from GitHub and place it here.")
            return
        try:
            subprocess.run(['gst-launch-1.0', '--version'], check=True, capture_output=True)
        except (FileNotFoundError, subprocess.CalledProcessError):
            self.log_message("ERROR: 'gst-launch-1.0' not found or failed.")
            self.log_message("Please ensure GStreamer is installed correctly.")
            return
        self.log_message("Starting rtsp-simple-server...")
        rtsp_server_process = subprocess.Popen(['./rtsp-simple-server'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self.gstreamer_processes.append(rtsp_server_process)
        time.sleep(1)
        gst_command = f'gst-launch-1.0 videotestsrc pattern=ball ! video/x-raw,width=640,height=480,framerate=30/1 ! videoconvert ! x264enc tune=zerolatency bitrate=500 speed-preset=superfast ! rtph264pay ! udpsink host=127.0.0.1 port={RTSP_PUSH_PORT}'
        self.log_message("Starting GStreamer pipeline...")
        gst_process = subprocess.Popen(gst_command, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self.gstreamer_processes.append(gst_process)
        while self.running:
            time.sleep(1)
        self.log_message("RTSP Stream thread terminated.")

if __name__ == "__main__":
    root = tk.Tk()
    app = RobotSimulatorGUI(root)
    root.mainloop()