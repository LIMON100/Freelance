# import sys
# import os
# import socket
# import struct
# import threading
# import time
# from datetime import datetime
# import signal


# # --- GUI Library Import ---
# try:
#     from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QLabel, 
#                                  QPushButton, QTextEdit, QTabWidget, QGridLayout, QSlider, QGroupBox)
#     from PyQt5.QtCore import Qt, QTimer, pyqtSignal, QObject
#     from PyQt5.QtGui import QFont, QPalette, QColor
#     GUI_AVAILABLE = True
#     print("PyQt5 found. Running with graphical interface.")
# except ImportError:
#     GUI_AVAILABLE = False
#     print("PyQt5 not found. Running in headless (terminal-only) mode.")

# # --- RTSP Server Library Import ---
# try:
#     import gi
#     gi.require_version('Gst', '1.0')
#     gi.require_version('GstRtspServer', '1.0')
#     from gi.repository import Gst, GstRtspServer, GLib
#     RTSP_AVAILABLE = True
#     print("GStreamer Python bindings found. RTSP server is available.")
# except (ImportError, ValueError):
#     RTSP_AVAILABLE = False
#     print("GStreamer Python bindings not found. RTSP server will be disabled.")


# # --- Constants matching Flutter app and C++ Server ---
# ROBOT_IP = '0.0.0.0'
# COMMAND_PORT = 65432
# DRIVING_PORT = 65434
# TOUCH_PORT = 65433
# STATUS_PORT = 65435
# RTSP_PORT = "8554"

# COMMAND_IDS = { 'IDLE': 0, 'DRIVING': 1, 'RECON': 2, 'DRONE': 3, 'MANUAL_ATTACK': 4, 'AUTO_ATTACK': 5 }
# ID_TO_NAME = {v: k for k, v in COMMAND_IDS.items()}

# # --- Data Structure Formats for struct ---
# STATE_COMMAND_FORMAT = "<B?bbbf"
# DRIVING_COMMAND_FORMAT = "<bb"
# TOUCH_COORD_FORMAT = "<ff"
# STATUS_PACKET_FORMAT = "<BBBff"

# # --- PyQt Signal Emitter for Thread-Safe GUI Updates ---
# class SignalEmitter(QObject):
#     log_message = pyqtSignal(str, str)
#     status_update = pyqtSignal(dict)

# # --- Main Simulator Class ---
# class RobotSimulator:
#     def __init__(self):
#         self.running = False
#         self.lock = threading.Lock()
#         self.active_mode_id = COMMAND_IDS['IDLE']
#         self.attack_permission = False
#         self.pan_speed, self.tilt_speed, self.zoom_command = 0, 0, 0
#         self.lateral_wind = 0.0
#         self.move_speed, self.turn_angle = 0, 0
#         self.permission_request_active = False
#         self.crosshair_x, self.crosshair_y = -1.0, -1.0
#         self.status_clients = []
#         if GUI_AVAILABLE:
#             self.emitter = SignalEmitter()

#     def start(self):
#         self.running = True
#         threads = [
#             threading.Thread(target=self.start_command_server, daemon=True),
#             threading.Thread(target=self.start_driving_server, daemon=True),
#             threading.Thread(target=self.start_touch_server, daemon=True),
#             threading.Thread(target=self.start_status_server, daemon=True),
#         ]
#         if RTSP_AVAILABLE:
#             threads.append(threading.Thread(target=self.start_rtsp_server, daemon=True))
        
#         for t in threads:
#             t.start()
#         print("Robot simulator started in all threads.")

#     def stop(self):
#         print("Stopping simulator...")
#         self.running = False
#         time.sleep(1.2)

#     def log(self, log_type, message):
#         if GUI_AVAILABLE:
#             self.emitter.log_message.emit(log_type, message)
#         else:
#             timestamp = datetime.now().strftime('%H:%M:%S')
#             print(f"[{timestamp}] [{log_type.upper()}] {message.replace(chr(10), ' | ')}")

#     # --- Server Implementations ---
#     def _create_socket(self, sock_type, port):
#         s = socket.socket(socket.AF_INET, sock_type)
#         s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
#         s.bind((ROBOT_IP, port))
#         return s

#     def start_command_server(self):
#         try:
#             with self._create_socket(socket.SOCK_STREAM, COMMAND_PORT) as s:
#                 s.settimeout(1.0)
#                 s.listen(1)
#                 print(f"[TCP Command Server] Listening on {ROBOT_IP}:{COMMAND_PORT}")
#                 while self.running:
#                     try:
#                         client_socket, address = s.accept()
#                         self.log('status', f"Command client connected from {address[0]}")
#                         threading.Thread(target=self.handle_command_client, args=(client_socket,), daemon=True).start()
#                     except socket.timeout:
#                         continue
#         except Exception as e:
#             if self.running: print(f"[TCP Command Server] Error: {e}")

#     def handle_command_client(self, client_socket):
#         with client_socket:
#             while self.running:
#                 try:
#                     data = client_socket.recv(struct.calcsize(STATE_COMMAND_FORMAT))
#                     if not data: break
#                     cmd_id, att_perm, pan, tilt, zoom, wind = struct.unpack(STATE_COMMAND_FORMAT, data)
#                     with self.lock:
#                         self.active_mode_id, self.attack_permission, self.pan_speed, self.tilt_speed, self.zoom_command, self.lateral_wind = cmd_id, att_perm, pan, tilt, zoom, wind
#                     msg = (f"Mode: {ID_TO_NAME.get(cmd_id, 'UNK')}, Perm: {att_perm}, Pan: {pan}, "
#                            f"Tilt: {tilt}, Zoom: {zoom}, Wind: {wind:.1f}")
#                     self.log('command', msg)
#                 except (ConnectionResetError, BrokenPipeError): break
#                 except Exception as e:
#                     print(f"Error in handle_command_client: {e}")
#                     break
#         self.log('status', "Command client disconnected.")

#     def start_driving_server(self):
#         with self._create_socket(socket.SOCK_DGRAM, DRIVING_PORT) as s:
#             s.settimeout(1.0)
#             print(f"[UDP Driving Server] Listening on {ROBOT_IP}:{DRIVING_PORT}")
#             while self.running:
#                 try:
#                     data, _ = s.recvfrom(1024)
#                     move, turn = struct.unpack(DRIVING_COMMAND_FORMAT, data)
#                     with self.lock: self.move_speed, self.turn_angle = move, turn
#                     self.log('driving', f"Move: {move}, Turn: {turn}")
#                 except socket.timeout:
#                     continue

#     def start_touch_server(self):
#         with self._create_socket(socket.SOCK_DGRAM, TOUCH_PORT) as s:
#             s.settimeout(1.0)
#             print(f"[UDP Touch Server] Listening on {ROBOT_IP}:{TOUCH_PORT}")
#             while self.running:
#                 try:
#                     data, _ = s.recvfrom(1024)
#                     x, y = struct.unpack(TOUCH_COORD_FORMAT, data)
#                     # This thread's only job is to receive and log.
#                     self.log('touch', f"X: {x:.3f}, Y: {y:.3f}")
#                 except socket.timeout:
#                     continue

#     def start_status_server(self):
#         with self._create_socket(socket.SOCK_STREAM, STATUS_PORT) as s:
#             s.settimeout(1.0)
#             s.listen(1)
#             print(f"[TCP Status Server] Listening on {ROBOT_IP}:{STATUS_PORT}")
            
#             threading.Thread(target=self.broadcast_status, daemon=True).start()
            
#             while self.running:
#                 try:
#                     client_socket, address = s.accept()
#                     with self.lock: self.status_clients.append(client_socket)
#                     self.log('status', f"Status client connected from {address[0]}")
#                 except socket.timeout:
#                     continue

#     def broadcast_status(self):
#         while self.running:
#             with self.lock:
#                 # --- THIS IS THE FIX ---
#                 # The logic now EXACTLY matches the C++ server.
#                 # The permission request is ONLY active if the mode is a combat mode.
#                 is_combat_mode = 2 <= self.active_mode_id <= 5
#                 permission_active_flag = 1 if is_combat_mode else 0
                
#                 # The GUI checkbox now acts as an override ONLY for testing,
#                 # but the primary logic is driven by the active mode.
#                 if self.permission_request_active:
#                     permission_active_flag = 1

#                 packet = struct.pack(STATUS_PACKET_FORMAT, 1, self.active_mode_id, 
#                                      permission_active_flag, self.crosshair_x, self.crosshair_y)
                
#                 disconnected_clients = []
#                 for client in self.status_clients:
#                     try: client.sendall(packet)
#                     except (ConnectionResetError, BrokenPipeError): disconnected_clients.append(client)
                
#                 for client in disconnected_clients:
#                     self.status_clients.remove(client)
#                     self.log('status', "Status client disconnected.")
#             time.sleep(0.1)

#     def start_rtsp_server(self):
#         Gst.init(None)
#         main_loop = GLib.MainLoop()
#         server = GstRtspServer.RTSPServer()
#         server.set_service(RTSP_PORT)
#         mounts = server.get_mount_points()

#         for i, pattern in enumerate(['ball', 'smpte']):
#             factory = GstRtspServer.RTSPMediaFactory()
#             factory.set_launch(
#                 f"( videotestsrc pattern={pattern} is-live=true ! video/x-raw,width=640,height=480,framerate=30/1 ! "
#                 f"videoconvert ! x264enc speed-preset=ultrafast tune=zerolatency ! rtph264pay name=pay0 pt=96 )"
#             )
#             factory.set_shared(True)
#             mounts.add_factory(f"/cam{i}", factory)
        
#         server.attach(None)
#         host_ip = "127.0.0.1"
#         try:
#             host_ip = socket.gethostbyname(socket.gethostname())
#         except: pass
#         print(f"[RTSP Server] Streams ready at rtsp://{host_ip}:{RTSP_PORT}/cam0 and /cam1")
#         self.log('status', f"RTSP streams available at rtsp://{host_ip}:{RTSP_PORT}")

#         glib_thread = threading.Thread(target=main_loop.run, daemon=True)
#         glib_thread.start()

#         while self.running:
#             time.sleep(0.1)
        
#         main_loop.quit()
#         print("[RTSP Server] Shut down.")

# # --- PyQt5 GUI Class ---
# if GUI_AVAILABLE:
#     class SimulatorGUI(QMainWindow):
#         def __init__(self, simulator):
#             super().__init__()
#             self.simulator = simulator
#             self.setWindowTitle("Robot Control Simulator")
#             self.setGeometry(100, 100, 900, 700)
#             self._set_dark_theme()
#             self.init_ui()
#             self.simulator.emitter.log_message.connect(self.update_log)

#         def log(self, log_type, message):
#             self.simulator.log(log_type, message)

#         def _set_dark_theme(self):
#             self.setStyleSheet("""
#                 QMainWindow, QWidget { background-color: #2E2E2E; color: #E0E0E0; }
#                 QGroupBox { border: 1px solid #555; border-radius: 5px; margin-top: 1ex; font-weight: bold; }
#                 QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 3px; }
#                 QTextEdit { background-color: #1E1E1E; border: 1px solid #555; }
#                 QPushButton { background-color: #555; border: 1px solid #666; padding: 8px; border-radius: 3px; }
#                 QPushButton:hover { background-color: #666; }
#                 QPushButton:pressed { background-color: #777; }
#                 QPushButton:checked { background-color: #007ACC; border: 1px solid #005A9E; }
#                 QTabWidget::pane { border-top: 1px solid #555; }
#                 QTabBar::tab { background: #444; border: 1px solid #555; padding: 8px 20px; }
#                 QTabBar::tab:selected { background: #2E2E2E; }
#                 QSlider::groove:horizontal { border: 1px solid #555; height: 8px; background: #333; margin: 2px 0; }
#                 QSlider::handle:horizontal { background: #007ACC; border: 1px solid #005A9E; width: 18px; margin: -2px 0; border-radius: 3px; }
#                 QLabel { padding-top: 5px; }
#             """)

#         def init_ui(self):
#             central_widget = QWidget()
#             self.setCentralWidget(central_widget)
#             main_layout = QVBoxLayout(central_widget)
            
#             tab_widget = QTabWidget()
#             main_layout.addWidget(tab_widget)
            
#             self.create_monitor_tab(tab_widget)
#             self.create_control_tab(tab_widget)
#             self.create_status_tab(tab_widget)
            
#             self.status_label = QLabel("Ready")
#             self.status_label.setStyleSheet("padding: 5px; background-color: #1E1E1E; border-top: 1px solid #555;")
#             main_layout.addWidget(self.status_label)

#         def create_monitor_tab(self, parent):
#             monitor_tab = QWidget()
#             layout = QVBoxLayout(monitor_tab)
#             self.log_widgets = {}
#             for log_name in ['Command', 'Driving', 'Touch']:
#                 group = QGroupBox(f"{log_name} Log:")
#                 group_layout = QVBoxLayout(group)
#                 text_edit = QTextEdit()
#                 text_edit.setReadOnly(True)
#                 text_edit.setFont(QFont("Monospace", 9))
#                 group_layout.addWidget(text_edit)
#                 layout.addWidget(group)
#                 self.log_widgets[log_name.lower()] = text_edit
#             parent.addTab(monitor_tab, "Monitor")

#         def create_control_tab(self, parent):
#             control_tab = QWidget()
#             layout = QVBoxLayout(control_tab)
            
#             permission_group = QGroupBox("Permission Control")
#             permission_layout = QHBoxLayout(permission_group)
#             self.permission_button = QPushButton("Force 'Request Permission'")
#             self.permission_button.setCheckable(True)
#             self.permission_button.toggled.connect(self.toggle_permission_request)
#             permission_layout.addWidget(self.permission_button)
#             layout.addWidget(permission_group)

#             crosshair_group = QGroupBox("Crosshair Control")
#             crosshair_layout = QGridLayout(crosshair_group)
#             self.x_slider = QSlider(Qt.Horizontal)
#             self.x_slider.setRange(-100, 100)
#             self.x_slider.setValue(-100)
#             self.x_slider.valueChanged.connect(self.update_crosshair)
#             self.y_slider = QSlider(Qt.Horizontal)
#             self.y_slider.setRange(-100, 100)
#             self.y_slider.setValue(-100)
#             self.y_slider.valueChanged.connect(self.update_crosshair)
            
#             crosshair_layout.addWidget(QLabel("X Position:"), 0, 0)
#             crosshair_layout.addWidget(self.x_slider, 0, 1)
#             crosshair_layout.addWidget(QLabel("Y Position:"), 1, 0)
#             crosshair_layout.addWidget(self.y_slider, 1, 1)
            
#             layout.addWidget(crosshair_group)
#             layout.addStretch()
#             parent.addTab(control_tab, "Control")

#         def create_status_tab(self, parent):
#             status_tab = QWidget()
#             layout = QVBoxLayout(status_tab)
#             self.status_log = QTextEdit()
#             self.status_log.setReadOnly(True)
#             self.status_log.setFont(QFont("Monospace", 10))
#             layout.addWidget(self.status_log)
#             parent.addTab(status_tab, "Status")

#         def update_log(self, log_type, message):
#             log_widget = self.log_widgets.get(log_type.lower())
#             if log_widget:
#                 timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
#                 log_widget.append(f"[{timestamp}] {message}")
#             if log_type.lower() == 'status':
#                 self.status_log.append(f"[{datetime.now().strftime('%H:%M:%S')}] {message}")
#                 self.status_label.setText(message)

#         def toggle_permission_request(self, checked):
#             with self.simulator.lock:
#                 self.simulator.permission_request_active = checked
            
#             # --- FIX: Provide clearer feedback in the log ---
#             if checked:
#                 self.log('status', "Forcing Permission Request ON (Overrides mode logic)")
#             else:
#                 self.log('status', "Force Permission Request OFF (Reverting to mode-based logic)")

#         def update_crosshair(self):
#             with self.simulator.lock:
#                 self.simulator.crosshair_x = self.x_slider.value() / 100.0
#                 self.simulator.crosshair_y = self.y_slider.value() / 100.0
#             self.log('status', f"Crosshair set to ({self.simulator.crosshair_x:.2f}, {self.simulator.crosshair_y:.2f})")

#         def closeEvent(self, event):
#             self.simulator.stop()
#             event.accept()

# # --- Main Execution ---
# def main():
#     simulator = RobotSimulator()
#     def handle_shutdown_signal(sig, frame):
#         print("\nCtrl+C detected. Shutting down...")
#         simulator.stop()
#         if GUI_AVAILABLE and 'app' in locals():
#             app.quit()
#         else:
#             sys.exit(0)

#     signal.signal(signal.SIGINT, handle_shutdown_signal)
#     simulator.start()

#     if GUI_AVAILABLE:
#         app = QApplication(sys.argv)
#         gui = SimulatorGUI(simulator)
#         gui.show()
#         app.exec_()
#         simulator.stop()
#     else:
#         print("Running in headless mode. Press Ctrl+C to stop.")
#         try:
#             while simulator.running:
#                 time.sleep(1)
#         except KeyboardInterrupt:
#             pass
#         finally:
#             simulator.stop()

# if __name__ == "__main__":
#     main()


import sys
import os
import socket
import struct
import threading
import time
from datetime import datetime
import signal
import random

# --- GUI Library Import ---
try:
    from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QLabel, 
                                 QPushButton, QTextEdit, QTabWidget, QGridLayout, QSlider, QGroupBox)
    from PyQt5.QtCore import Qt, QTimer, pyqtSignal, QObject
    from PyQt5.QtGui import QFont, QPalette, QColor
    GUI_AVAILABLE = True
    print("PyQt5 found. Running with graphical interface.")
except ImportError:
    GUI_AVAILABLE = False
    print("PyQt5 not found. Running in headless (terminal-only) mode.")

# --- RTSP Server Library Import ---
try:
    import gi
    gi.require_version('Gst', '1.0')
    gi.require_version('GstRtspServer', '1.0')
    from gi.repository import Gst, GstRtspServer, GLib
    RTSP_AVAILABLE = True
    print("GStreamer Python bindings found. RTSP server is available.")
except (ImportError, ValueError):
    RTSP_AVAILABLE = False
    print("GStreamer Python bindings not found. RTSP server will be disabled.")


# --- Constants matching Flutter app and C++ Server ---
ROBOT_IP = '0.0.0.0'
COMMAND_PORT = 65432
DRIVING_PORT = 65434
TOUCH_PORT = 65433
STATUS_PORT = 65435
RTSP_PORT = "8555"

COMMAND_IDS = { 'IDLE': 0, 'DRIVING': 1, 'RECON': 2, 'DRONE': 3, 'MANUAL_ATTACK': 4, 'AUTO_ATTACK': 5 }
ID_TO_NAME = {v: k for k, v in COMMAND_IDS.items()}

# --- Data Structure Formats for struct ---
STATE_COMMAND_FORMAT = "<B?bbbf"
DRIVING_COMMAND_FORMAT = "<bb"
TOUCH_COORD_FORMAT = "<ff"
STATUS_PACKET_FORMAT = "<BBBff"

class SignalEmitter(QObject):
    log_message = pyqtSignal(str, str)

class RobotSimulator:
    def __init__(self):
        self.running = False
        self.lock = threading.Lock()
        self.active_mode_id = COMMAND_IDS['IDLE']
        self.attack_permission = False
        self.pan_speed, self.tilt_speed, self.zoom_command = 0, 0, 0
        self.lateral_wind = 0.0
        self.move_speed, self.turn_angle = 0, 0
        self.permission_request_active = False
        self.crosshair_x, self.crosshair_y = -1.0, -1.0
        self.status_clients = []
        if GUI_AVAILABLE:
            self.emitter = SignalEmitter()

    def start(self):
        self.running = True
        threads = [
            threading.Thread(target=self.start_command_server, daemon=True),
            threading.Thread(target=self.start_driving_server, daemon=True),
            threading.Thread(target=self.start_touch_server, daemon=True),
            threading.Thread(target=self.start_status_server, daemon=True),
        ]
        if RTSP_AVAILABLE:
            threads.append(threading.Thread(target=self.start_rtsp_server, daemon=True))
        
        for t in threads:
            t.start()
        print("Robot simulator started in all threads.")

    def stop(self):
        print("Stopping simulator...")
        self.running = False
        time.sleep(1.2)

    def log(self, log_type, message):
        if GUI_AVAILABLE:
            self.emitter.log_message.emit(log_type, message)
        else:
            timestamp = datetime.now().strftime('%H:%M:%S')
            print(f"[{timestamp}] [{log_type.upper()}] {message.replace(chr(10), ' | ')}")

    def _create_socket(self, sock_type, port):
        s = socket.socket(socket.AF_INET, sock_type)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((ROBOT_IP, port))
        return s

    def start_command_server(self):
        try:
            with self._create_socket(socket.SOCK_STREAM, COMMAND_PORT) as s:
                s.settimeout(1.0)
                s.listen(1)
                print(f"[TCP Command Server] Listening on {ROBOT_IP}:{COMMAND_PORT}")
                while self.running:
                    try:
                        client_socket, address = s.accept()
                        self.log('status', f"Command client connected from {address[0]}")
                        threading.Thread(target=self.handle_command_client, args=(client_socket,), daemon=True).start()
                    except socket.timeout:
                        continue
        except Exception as e:
            if self.running: print(f"[TCP Command Server] Error: {e}")

    def handle_command_client(self, client_socket):
        with client_socket:
            while self.running:
                try:
                    data = client_socket.recv(struct.calcsize(STATE_COMMAND_FORMAT))
                    if not data: break
                    cmd_id, att_perm, pan, tilt, zoom, wind = struct.unpack(STATE_COMMAND_FORMAT, data)
                    with self.lock:
                        self.active_mode_id, self.attack_permission, self.pan_speed, self.tilt_speed, self.zoom_command, self.lateral_wind = cmd_id, att_perm, pan, tilt, zoom, wind
                    msg = (f"Mode: {ID_TO_NAME.get(cmd_id, 'UNK')}, Perm: {att_perm}, Pan: {pan}, "
                           f"Tilt: {tilt}, Zoom: {zoom}, Wind: {wind:.1f}")
                    self.log('command', msg)
                except (ConnectionResetError, BrokenPipeError): break
                except Exception as e:
                    print(f"Error in handle_command_client: {e}")
                    break
        self.log('status', "Command client disconnected.")

    def start_driving_server(self):
        with self._create_socket(socket.SOCK_DGRAM, DRIVING_PORT) as s:
            s.settimeout(1.0)
            print(f"[UDP Driving Server] Listening on {ROBOT_IP}:{DRIVING_PORT}")
            while self.running:
                try:
                    data, _ = s.recvfrom(1024)
                    move, turn = struct.unpack(DRIVING_COMMAND_FORMAT, data)
                    with self.lock: self.move_speed, self.turn_angle = move, turn
                    self.log('driving', f"Move: {move}, Turn: {turn}")
                except socket.timeout:
                    continue

    def start_touch_server(self):
        with self._create_socket(socket.SOCK_DGRAM, TOUCH_PORT) as s:
            s.settimeout(1.0)
            print(f"[UDP Touch Server] Listening on {ROBOT_IP}:{TOUCH_PORT}")
            while self.running:
                try:
                    data, _ = s.recvfrom(1024)
                    x, y = struct.unpack(TOUCH_COORD_FORMAT, data)
                    self.log('touch', f"X: {x:.3f}, Y: {y:.3f}")
                except socket.timeout:
                    continue

    def start_status_server(self):
        with self._create_socket(socket.SOCK_STREAM, STATUS_PORT) as s:
            s.settimeout(1.0)
            s.listen(1)
            print(f"[TCP Status Server] Listening on {ROBOT_IP}:{STATUS_PORT}")
            threading.Thread(target=self.broadcast_status, daemon=True).start()
            while self.running:
                try:
                    client_socket, address = s.accept()
                    with self.lock: self.status_clients.append(client_socket)
                    self.log('status', f"Status client connected from {address[0]}")
                except socket.timeout:
                    continue

    def broadcast_status(self):
        while self.running:
            with self.lock:
                is_combat_mode = 2 <= self.active_mode_id <= 5
                permission_active_flag = 1 if (is_combat_mode or self.permission_request_active) else 0
                packet = struct.pack(STATUS_PACKET_FORMAT, 1, self.active_mode_id, 
                                     permission_active_flag, self.crosshair_x, self.crosshair_y)
                disconnected_clients = []
                for client in self.status_clients:
                    try: client.sendall(packet)
                    except (ConnectionResetError, BrokenPipeError): disconnected_clients.append(client)
                for client in disconnected_clients:
                    self.status_clients.remove(client)
                    self.log('status', "Status client disconnected.")
            time.sleep(0.1)

    def start_rtsp_server(self):
        Gst.init(None)
        main_loop = GLib.MainLoop()
        server = GstRtspServer.RTSPServer()
        server.set_service(RTSP_PORT) # This should be "8555"
        mounts = server.get_mount_points()

        # --- THIS IS THE NEW, MORE ROBUST PIPELINE LOGIC ---

        pipeline_source = ""
        webcam_path = "/dev/video0"
        video_file_path = "30ir.mp4"
        
        # We will use a simpler and more compatible encoder: openh264enc
        # If you don't have it, install with: sudo apt-get install gstreamer1.0-plugins-bad
        encoder = "openh264enc" 

        if os.path.exists(webcam_path):
            print(f"[RTSP Server] Webcam found at {webcam_path}. Using live camera feed.")
            pipeline_source = (
                f"v4l2src device={webcam_path} ! videoconvert ! videoscale ! "
                f"video/x-raw,width=640,height=480 ! {encoder}"
            )
        elif os.path.exists(video_file_path):
            print(f"[RTSP Server] No webcam found. Fallback to looping video file: {video_file_path}")
            # SIMPLIFIED PIPELINE: Let decodebin handle everything.
            pipeline_source = (
                f"filesrc location={video_file_path} ! decodebin ! videoconvert ! videoscale ! "
                f"video/x-raw,width=640,height=480 ! {encoder}"
            )
        else:
            print("[RTSP Server] No webcam or video file found. Using default test pattern.")
            pipeline_source = (
                f"videotestsrc pattern=ball is-live=true ! video/x-raw,width=640,height=480 ! {encoder}"
            )

        factory = GstRtspServer.RTSPMediaFactory()
        factory.set_launch(f"( {pipeline_source} ! rtph264pay name=pay0 pt=96 )")
        factory.set_shared(True)
        
        mounts.add_factory("/cam0", factory)
        mounts.add_factory("/cam1", factory)
        
        server.attach(None)
        
        host_ip = "127.0.0.1"
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.connect(("8.8.8.8", 80))
                host_ip = s.getsockname()[0]
        except Exception:
            pass
            
        print(f"[RTSP Server] Streams ready at rtsp://{host_ip}:{RTSP_PORT}/cam0 and /cam1")
        self.log('status', f"RTSP streams available at rtsp://{host_ip}:{RTSP_PORT}")

        glib_thread = threading.Thread(target=main_loop.run, daemon=True)
        glib_thread.start()

        while self.running:
            time.sleep(0.1)
        
        main_loop.quit()
        print("[RTSP Server] Shut down.")

# --- PyQt5 GUI Class ---
if GUI_AVAILABLE:
    class SimulatorGUI(QMainWindow):
        def __init__(self, simulator):
            super().__init__()
            self.simulator = simulator
            self.setWindowTitle("Robot Control Simulator")
            self.setGeometry(100, 100, 900, 700)
            self._set_dark_theme()
            self.init_ui()
            self.simulator.emitter.log_message.connect(self.update_log)

        def log(self, log_type, message):
            self.simulator.log(log_type, message)

        def _set_dark_theme(self):
            self.setStyleSheet("""
                QMainWindow, QWidget { background-color: #2E2E2E; color: #E0E0E0; }
                QGroupBox { border: 1px solid #555; border-radius: 5px; margin-top: 1ex; font-weight: bold; }
                QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 3px; }
                QTextEdit { background-color: #1E1E1E; border: 1px solid #555; }
                QPushButton { background-color: #555; border: 1px solid #666; padding: 8px; border-radius: 3px; }
                QPushButton:hover { background-color: #666; }
                QPushButton:pressed { background-color: #777; }
                QPushButton:checked { background-color: #007ACC; border: 1px solid #005A9E; }
                QTabWidget::pane { border-top: 1px solid #555; }
                QTabBar::tab { background: #444; border: 1px solid #555; padding: 8px 20px; }
                QTabBar::tab:selected { background: #2E2E2E; }
                QSlider::groove:horizontal { border: 1px solid #555; height: 8px; background: #333; margin: 2px 0; }
                QSlider::handle:horizontal { background: #007ACC; border: 1px solid #005A9E; width: 18px; margin: -2px 0; border-radius: 3px; }
                QLabel { padding-top: 5px; }
            """)

        def init_ui(self):
            central_widget = QWidget()
            self.setCentralWidget(central_widget)
            main_layout = QVBoxLayout(central_widget)
            tab_widget = QTabWidget()
            main_layout.addWidget(tab_widget)
            self.create_monitor_tab(tab_widget)
            self.create_control_tab(tab_widget)
            self.create_status_tab(tab_widget)
            self.status_label = QLabel("Ready")
            self.status_label.setStyleSheet("padding: 5px; background-color: #1E1E1E; border-top: 1px solid #555;")
            main_layout.addWidget(self.status_label)

        def create_monitor_tab(self, parent):
            monitor_tab = QWidget()
            layout = QVBoxLayout(monitor_tab)
            self.log_widgets = {}
            for log_name in ['Command', 'Driving', 'Touch']:
                group = QGroupBox(f"{log_name} Log:")
                group_layout = QVBoxLayout(group)
                text_edit = QTextEdit()
                text_edit.setReadOnly(True)
                text_edit.setFont(QFont("Monospace", 9))
                group_layout.addWidget(text_edit)
                layout.addWidget(group)
                self.log_widgets[log_name.lower()] = text_edit
            parent.addTab(monitor_tab, "Monitor")

        def create_control_tab(self, parent):
            control_tab = QWidget()
            layout = QVBoxLayout(control_tab)
            
            permission_group = QGroupBox("Permission Control")
            permission_layout = QHBoxLayout(permission_group)
            self.permission_button = QPushButton("Force 'Request Permission'")
            self.permission_button.setCheckable(True)
            self.permission_button.toggled.connect(self.toggle_permission_request)
            permission_layout.addWidget(self.permission_button)
            layout.addWidget(permission_group)

            crosshair_group = QGroupBox("Crosshair Control")
            crosshair_layout = QGridLayout(crosshair_group)
            self.x_slider = QSlider(Qt.Horizontal)
            self.x_slider.setRange(-100, 100)
            self.x_slider.setValue(-100)
            self.x_slider.valueChanged.connect(self.update_crosshair)
            self.y_slider = QSlider(Qt.Horizontal)
            self.y_slider.setRange(-100, 100)
            self.y_slider.setValue(-100)
            self.y_slider.valueChanged.connect(self.update_crosshair)
            crosshair_layout.addWidget(QLabel("X Position:"), 0, 0)
            crosshair_layout.addWidget(self.x_slider, 0, 1)
            crosshair_layout.addWidget(QLabel("Y Position:"), 1, 0)
            crosshair_layout.addWidget(self.y_slider, 1, 1)
            layout.addWidget(crosshair_group)

            # --- FIX: ADD THE SIMULATE TOUCH BUTTON ---
            touch_sim_group = QGroupBox("Touch Simulation")
            touch_sim_layout = QHBoxLayout(touch_sim_group)
            simulate_touch_button = QPushButton("Simulate Touch at Center (0.5, 0.5)")
            simulate_touch_button.clicked.connect(self.simulate_touch)
            touch_sim_layout.addWidget(simulate_touch_button)
            layout.addWidget(touch_sim_group)
            # --- END OF FIX ---

            layout.addStretch()
            parent.addTab(control_tab, "Control")

        def create_status_tab(self, parent):
            status_tab = QWidget()
            layout = QVBoxLayout(status_tab)
            self.status_log = QTextEdit()
            self.status_log.setReadOnly(True)
            self.status_log.setFont(QFont("Monospace", 10))
            layout.addWidget(self.status_log)
            parent.addTab(status_tab, "Status")

        def update_log(self, log_type, message):
            log_widget = self.log_widgets.get(log_type.lower())
            if log_widget:
                timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                log_widget.append(f"[{timestamp}] {message}")
            if log_type.lower() == 'status':
                self.status_log.append(f"[{datetime.now().strftime('%H:%M:%S')}] {message}")
                self.status_label.setText(message)

        def toggle_permission_request(self, checked):
            with self.simulator.lock:
                self.simulator.permission_request_active = checked
            self.log('status', f"Forcing Permission Request to {checked}")

        def update_crosshair(self):
            with self.simulator.lock:
                self.simulator.crosshair_x = self.x_slider.value() / 100.0
                self.simulator.crosshair_y = self.y_slider.value() / 100.0
            self.log('status', f"Crosshair set to ({self.simulator.crosshair_x:.2f}, {self.simulator.crosshair_y:.2f})")

        # --- FIX: ADD THE METHOD TO SIMULATE A TOUCH ---
        def simulate_touch(self):
            # This function sends a UDP packet to the simulator's own touch port.
            x, y = 0.5, 0.5 # Simulate a tap at the center of the screen
            packet = struct.pack(TOUCH_COORD_FORMAT, x, y)
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.sendto(packet, ('127.0.0.1', TOUCH_PORT))
            self.log('status', f"Sent simulated touch packet to self: ({x}, {y})")
        # --- END OF FIX ---

        def closeEvent(self, event):
            self.simulator.stop()
            event.accept()

# --- Main Execution ---
def main():
    simulator = RobotSimulator()
    def handle_shutdown_signal(sig, frame):
        print("\nCtrl+C detected. Shutting down...")
        simulator.stop()
        if GUI_AVAILABLE and 'app' in locals():
            app.quit()
        else:
            sys.exit(0)

    signal.signal(signal.SIGINT, handle_shutdown_signal)
    simulator.start()

    if GUI_AVAILABLE:
        app = QApplication(sys.argv)
        gui = SimulatorGUI(simulator)
        gui.show()
        app.exec_()
        simulator.stop()
    else:
        print("Running in headless mode. Press Ctrl+C to stop.")
        try:
            while simulator.running:
                time.sleep(1)
        except KeyboardInterrupt:
            pass
        finally:
            simulator.stop()

if __name__ == "__main__":
    main()
