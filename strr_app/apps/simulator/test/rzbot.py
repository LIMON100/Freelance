import sys
import os
import time
import socket
import struct
import threading
import random
import queue
import json
import signal
from datetime import datetime

# Check for GUI availability first
GUI_AVAILABLE = False
try:
    # Set display environment variable if not set
    if 'DISPLAY' not in os.environ:
        os.environ['DISPLAY'] = ':0'
    
    # Try to import Qt libraries
    from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                                QHBoxLayout, QLabel, QPushButton, QTextEdit, 
                                QTabWidget, QGridLayout, QSlider, QGroupBox)
    from PyQt5.QtCore import Qt, QTimer, pyqtSignal, QObject
    from PyQt5.QtGui import QFont, QPalette, QColor, QGuiApplication
    
    # Create a minimal application to test display
    app = QApplication.instance()
    if app is None:
        app = QApplication(sys.argv)
    
    # Check if display is available
    if QGuiApplication.primaryScreen():
        GUI_AVAILABLE = True
        print("GUI available. Running with graphical interface.")
    else:
        print("No display available. Running in headless mode.")
        GUI_AVAILABLE = False
except Exception as e:
    print(f"GUI initialization error: {e}. Running in headless mode.")
    GUI_AVAILABLE = False

# RTSP Server Libraries
try:
    import cv2
    import numpy as np
    import subprocess
    import signal
    RTSP_AVAILABLE = True
except ImportError:
    print("OpenCV not available. RTSP simulation will be limited.")
    RTSP_AVAILABLE = False

# Constants matching Flutter app
ROBOT_IP = '0.0.0.0'  # Listen on all interfaces
COMMAND_PORT = 65432
DRIVING_PORT = 65434
TOUCH_PORT = 65433
STATUS_PORT = 65435

# Command IDs from CommandIds.dart
COMMAND_IDS = {
    'IDLE': 0,
    'DRIVING': 1,
    'RECON': 2,
    'DRONE': 3,
    'MANUAL_ATTACK': 4,
    'AUTO_ATTACK': 5
}

class SignalEmitter(QObject):
    """Helper class to emit signals from non-QT threads"""
    command_received = pyqtSignal(str)
    driving_received = pyqtSignal(str)
    touch_received = pyqtSignal(str)
    status_sent = pyqtSignal(str)
    client_connected = pyqtSignal(str)
    client_disconnected = pyqtSignal(str)

class RobotSimulator:
    def __init__(self):
        self.command_socket = None
        self.driving_socket = None
        self.touch_socket = None
        self.status_socket = None
        self.status_clients = []
        
        self.current_mode = COMMAND_IDS['IDLE']
        self.permission_request_active = False
        self.permission_granted = False
        self.crosshair_x = -1.0
        self.crosshair_y = -1.0
        
        self.running = False
        self.message_queue = queue.Queue()
        
        # For GUI updates - only initialize if GUI is available
        if GUI_AVAILABLE:
            self.emitter = SignalEmitter()
    
    def start(self):
        """Start all server components"""
        self.running = True
        
        # Start TCP command server
        self.command_thread = threading.Thread(target=self.start_command_server)
        self.command_thread.daemon = True
        self.command_thread.start()
        
        # Start UDP driving command server
        self.driving_thread = threading.Thread(target=self.start_driving_server)
        self.driving_thread.daemon = True
        self.driving_thread.start()
        
        # Start UDP touch server
        self.touch_thread = threading.Thread(target=self.start_touch_server)
        self.touch_thread.daemon = True
        self.touch_thread.start()
        
        # Start TCP status server
        self.status_thread = threading.Thread(target=self.start_status_server)
        self.status_thread.daemon = True
        self.status_thread.start()
        
        # Start RTSP server if available
        if RTSP_AVAILABLE:
            self.rtsp_thread = threading.Thread(target=self.start_rtsp_server)
            self.rtsp_thread.daemon = True
            self.rtsp_thread.start()
        
        print("Robot simulator started")
    
    def stop(self):
        """Stop all server components"""
        self.running = False
        
        # Close sockets
        if self.command_socket:
            self.command_socket.close()
        if self.status_socket:
            self.status_socket.close()
        
        # Terminate RTSP process if running
        if hasattr(self, 'rtsp_process') and self.rtsp_process:
            self.rtsp_process.terminate()
        
        print("Robot simulator stopped")
    
    def start_command_server(self):
        """Start TCP server for command reception"""
        try:
            # First, try to reuse socket
            self.command_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.command_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.command_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
            
            # Try to bind, and if it fails, try to find a free port
            port = COMMAND_PORT
            max_attempts = 10
            for attempt in range(max_attempts):
                try:
                    self.command_socket.bind((ROBOT_IP, port))
                    break
                except OSError as e:
                    if e.errno == 98:  # Address already in use
                        port += 1
                        if attempt == max_attempts - 1:
                            print(f"Could not find an available port after {max_attempts} attempts")
                            return
                    else:
                        raise e
            
            print(f"Command server listening on {ROBOT_IP}:{port}")
            self.command_socket.listen(5)
            
            while self.running:
                try:
                    client_socket, address = self.command_socket.accept()
                    client_thread = threading.Thread(
                        target=self.handle_command_client,
                        args=(client_socket, address)
                    )
                    client_thread.daemon = True
                    client_thread.start()
                    
                    if GUI_AVAILABLE:
                        self.emitter.client_connected.emit(f"Command client connected from {address}")
                    else:
                        print(f"[{datetime.now().strftime('%H:%M:%S')}] Command client connected from {address}")
                except:
                    break
        except Exception as e:
            print(f"Error in command server: {e}")
    
    def handle_command_client(self, client_socket, address):
        """Handle individual command client"""
        try:
            while self.running:
                data = client_socket.recv(1024)
                if not data:
                    break
                
                # Parse command packet (13 bytes)
                if len(data) >= 9:  # Minimum expected size
                    command_id = data[0]
                    attack_permission = bool(data[1])
                    pan_speed = struct.unpack('b', data[2:3])[0]
                    tilt_speed = struct.unpack('b', data[3:4])[0]
                    zoom_command = struct.unpack('b', data[4:5])[0]
                    lateral_wind_speed = struct.unpack('f', data[5:9])[0]
                    
                    # Update internal state
                    self.current_mode = command_id
                    
                    # Format message for display
                    command_name = "UNKNOWN"
                    for name, id in COMMAND_IDS.items():
                        if id == command_id:
                            command_name = name
                            break
                    
                    message = (
                        f"Command from {address}:\n"
                        f"  ID: {command_name} ({command_id})\n"
                        f"  Attack Permission: {attack_permission}\n"
                        f"  Pan Speed: {pan_speed}\n"
                        f"  Tilt Speed: {tilt_speed}\n"
                        f"  Zoom Command: {zoom_command}\n"
                        f"  Wind Speed: {lateral_wind_speed:.1f}"
                    )
                    
                    if GUI_AVAILABLE:
                        self.emitter.command_received.emit(message)
                    else:
                        print(f"[{datetime.now().strftime('%H:%M:%S')}] {message}")
        except Exception as e:
            print(f"Error handling command client: {e}")
        finally:
            client_socket.close()
            if GUI_AVAILABLE:
                self.emitter.client_disconnected.emit(f"Command client from {address} disconnected")
            else:
                print(f"[{datetime.now().strftime('%H:%M:%S')}] Command client from {address} disconnected")
    
    def start_driving_server(self):
        """Start UDP server for driving commands"""
        try:
            self.driving_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.driving_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.driving_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
            
            # Try to bind, and if it fails, try to find a free port
            port = DRIVING_PORT
            max_attempts = 10
            for attempt in range(max_attempts):
                try:
                    self.driving_socket.bind((ROBOT_IP, port))
                    break
                except OSError as e:
                    if e.errno == 98:  # Address already in use
                        port += 1
                        if attempt == max_attempts - 1:
                            print(f"Could not find an available port for driving server after {max_attempts} attempts")
                            return
                    else:
                        raise e
            
            print(f"Driving server listening on {ROBOT_IP}:{port}")
            
            while self.running:
                try:
                    data, address = self.driving_socket.recvfrom(1024)
                    
                    # Parse driving command (2 bytes)
                    if len(data) >= 2:
                        move_speed = struct.unpack('b', data[0:1])[0]
                        turn_angle = struct.unpack('b', data[1:2])[0]
                        
                        message = (
                            f"Driving command from {address}:\n"
                            f"  Move Speed: {move_speed}\n"
                            f"  Turn Angle: {turn_angle}"
                        )
                        
                        if GUI_AVAILABLE:
                            self.emitter.driving_received.emit(message)
                        else:
                            print(f"[{datetime.now().strftime('%H:%M:%S')}] {message}")
                except:
                    break
        except Exception as e:
            print(f"Error in driving server: {e}")
    
    def start_touch_server(self):
        """Start UDP server for touch coordinates"""
        try:
            self.touch_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.touch_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.touch_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
            
            # Try to bind, and if it fails, try to find a free port
            port = TOUCH_PORT
            max_attempts = 10
            for attempt in range(max_attempts):
                try:
                    self.touch_socket.bind((ROBOT_IP, port))
                    break
                except OSError as e:
                    if e.errno == 98:  # Address already in use
                        port += 1
                        if attempt == max_attempts - 1:
                            print(f"Could not find an available port for touch server after {max_attempts} attempts")
                            return
                    else:
                        raise e
            
            print(f"Touch server listening on {ROBOT_IP}:{port}")
            
            while self.running:
                try:
                    data, address = self.touch_socket.recvfrom(1024)
                    
                    # Parse touch coordinates (8 bytes)
                    if len(data) >= 8:
                        x = struct.unpack('f', data[0:4])[0]
                        y = struct.unpack('f', data[4:8])[0]
                        
                        # Update crosshair position if in manual attack mode
                        if self.current_mode == COMMAND_IDS['MANUAL_ATTACK']:
                            self.crosshair_x = x
                            self.crosshair_y = y
                        
                        message = (
                            f"Touch coordinates from {address}:\n"
                            f"  X: {x:.3f}\n"
                            f"  Y: {y:.3f}"
                        )
                        
                        if GUI_AVAILABLE:
                            self.emitter.touch_received.emit(message)
                        else:
                            print(f"[{datetime.now().strftime('%H:%M:%S')}] {message}")
                except:
                    break
        except Exception as e:
            print(f"Error in touch server: {e}")
    
    def start_status_server(self):
        """Start TCP server for status updates"""
        try:
            self.status_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.status_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.status_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
            
            # Try to bind, and if it fails, try to find a free port
            port = STATUS_PORT
            max_attempts = 10
            for attempt in range(max_attempts):
                try:
                    self.status_socket.bind((ROBOT_IP, port))
                    break
                except OSError as e:
                    if e.errno == 98:  # Address already in use
                        port += 1
                        if attempt == max_attempts - 1:
                            print(f"Could not find an available port for status server after {max_attempts} attempts")
                            return
                    else:
                        raise e
            
            print(f"Status server listening on {ROBOT_IP}:{port}")
            self.status_socket.listen(5)
            
            # Start status broadcast thread
            broadcast_thread = threading.Thread(target=self.broadcast_status)
            broadcast_thread.daemon = True
            broadcast_thread.start()
            
            while self.running:
                try:
                    client_socket, address = self.status_socket.accept()
                    self.status_clients.append(client_socket)
                    
                    if GUI_AVAILABLE:
                        self.emitter.client_connected.emit(f"Status client connected from {address}")
                    else:
                        print(f"[{datetime.now().strftime('%H:%M:%S')}] Status client connected from {address}")
                except:
                    break
        except Exception as e:
            print(f"Error in status server: {e}")
    
    def broadcast_status(self):
        """Broadcast status to all connected clients"""
        while self.running:
            # Create status packet (11 bytes)
            # 1 byte: isRtspRunning (always 1 for simulator)
            # 1 byte: currentModeId
            # 1 byte: permissionRequestActive
            # 4 bytes: crosshairX
            # 4 bytes: crosshairY
            
            # Simulate permission request in attack modes
            if self.current_mode in [COMMAND_IDS['MANUAL_ATTACK'], COMMAND_IDS['AUTO_ATTACK']]:
                if not self.permission_request_active and random.random() < 0.01:  # 1% chance per update
                    self.permission_request_active = True
            
            packet = struct.pack(
                '<BBBff',
                1,  # isRtspRunning
                self.current_mode,
                1 if self.permission_request_active else 0,
                self.crosshair_x,
                self.crosshair_y
            )
            
            # Send to all connected clients
            disconnected_clients = []
            for client in self.status_clients:
                try:
                    client.send(packet)
                except:
                    disconnected_clients.append(client)
            
            # Remove disconnected clients
            for client in disconnected_clients:
                self.status_clients.remove(client)
                if GUI_AVAILABLE:
                    self.emitter.client_disconnected.emit("Status client disconnected")
                else:
                    print(f"[{datetime.now().strftime('%H:%M:%S')}] Status client disconnected")
            
            time.sleep(0.1)  # Send status 10 times per second
    
    def start_rtsp_server(self):
        """Start RTSP server with test pattern"""
        try:
            # Create test pattern video
            self.create_test_pattern()
            
            # Try different RTSP server options
            rtsp_started = False
            
            # Option 1: Try GStreamer with udpsink
            try:
                rtsp_command = [
                    'gst-launch-1.0',
                    'filesrc', 'location=test_pattern.mp4',
                    '!', 'qtdemux',
                    '!', 'h264parse',
                    '!', 'rtph264pay', 'config-interval=1', 'pt=96',
                    '!', 'udpsink', 'host=127.0.0.1', 'port=5400'
                ]
                
                self.rtsp_process = subprocess.Popen(rtsp_command)
                print("RTSP server started with GStreamer")
                rtsp_started = True
                
                # Start RTSP server
                rtsp_server_command = [
                    'gst-launch-1.0',
                    'udpsrc', 'port=5400',
                    '!', 'application/x-rtp,encoding-name=H264,payload=96',
                    '!', 'rtph264depay',
                    '!', 'h264parse',
                    '!', 'avdec_h264',
                    '!', 'videoconvert',
                    '!', 'autovideosink'
                ]
                
                subprocess.run(rtsp_server_command)
            except Exception as e:
                print(f"GStreamer RTSP failed: {e}")
            
            # Option 2: Try using FFmpeg
            if not rtsp_started:
                try:
                    rtsp_command = [
                        'ffmpeg',
                        '-re', '-i', 'test_pattern.mp4',
                        '-c', 'copy',
                        '-f', 'rtsp',
                        'rtsp://127.0.0.1:8554/cam0'
                    ]
                    
                    self.rtsp_process = subprocess.Popen(rtsp_command)
                    print("RTSP server started with FFmpeg")
                    rtsp_started = True
                except Exception as e:
                    print(f"FFmpeg RTSP failed: {e}")
            
            # Option 3: Try using VLC
            if not rtsp_started:
                try:
                    rtsp_command = [
                        'vlc',
                        '--intf', 'dummy',
                        '--no-audio',
                        '--sout', '#rtp{sdp=rtsp://:8554/cam0}',
                        'test_pattern.mp4'
                    ]
                    
                    self.rtsp_process = subprocess.Popen(rtsp_command)
                    print("RTSP server started with VLC")
                    rtsp_started = True
                except Exception as e:
                    print(f"VLC RTSP failed: {e}")
            
            if not rtsp_started:
                print("Could not start RTSP server. Please install GStreamer, FFmpeg, or VLC.")
                
        except Exception as e:
            print(f"Error starting RTSP server: {e}")
    
    def create_test_pattern(self):
        """Create a test pattern video file"""
        if not RTSP_AVAILABLE:
            return
            
        # Create a simple test pattern video
        width, height = 640, 480
        fps = 30
        duration = 10  # seconds
        fourcc = cv2.VideoWriter_fourcc(*'mp4v')
        out = cv2.VideoWriter('test_pattern.mp4', fourcc, fps, (width, height))
        
        for i in range(fps * duration):
            # Create a frame with test pattern
            frame = np.zeros((height, width, 3), dtype=np.uint8)
            
            # Add time display
            timestamp = i / fps
            cv2.putText(frame, f"Time: {timestamp:.2f}s", (50, 50), 
                        cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
            
            # Add mode indicator
            mode_name = "UNKNOWN"
            for name, id in COMMAND_IDS.items():
                if id == self.current_mode:
                    mode_name = name
                    break
            
            cv2.putText(frame, f"Mode: {mode_name}", (50, 100), 
                        cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
            
            # Add crosshair if in attack mode
            if self.current_mode in [COMMAND_IDS['MANUAL_ATTACK'], COMMAND_IDS['AUTO_ATTACK']]:
                cx, cy = int(self.crosshair_x * width), int(self.crosshair_y * height)
                cv2.circle(frame, (cx, cy), 20, (0, 0, 255), 2)
                cv2.line(frame, (cx - 30, cy), (cx + 30, cy), (0, 0, 255), 2)
                cv2.line(frame, (cx, cy - 30), (cx, cy + 30), (0, 0, 255), 2)
            
            # Add moving element
            x = int(width * (0.5 + 0.3 * np.sin(2 * np.pi * i / (fps * 2))))
            y = int(height * (0.5 + 0.3 * np.cos(2 * np.pi * i / (fps * 3))))
            cv2.circle(frame, (x, y), 20, (0, 255, 0), -1)
            
            out.write(frame)
        
        out.release()
    
    def set_permission_granted(self, granted):
        """Set permission granted state"""
        self.permission_granted = granted
        self.permission_request_active = False
        
        message = f"Permission {'granted' if granted else 'denied'}"
        if GUI_AVAILABLE:
            self.emitter.status_sent.emit(message)
        else:
            print(f"[{datetime.now().strftime('%H:%M:%S')}] {message}")
    
    def set_mode(self, mode_id):
        """Set current robot mode"""
        self.current_mode = mode_id
        
        mode_name = "UNKNOWN"
        for name, id in COMMAND_IDS.items():
            if id == mode_id:
                mode_name = name
                break
        
        message = f"Mode set to {mode_name} ({mode_id})"
        if GUI_AVAILABLE:
            self.emitter.status_sent.emit(message)
        else:
            print(f"[{datetime.now().strftime('%H:%M:%S')}] {message}")

class SimulatorGUI(QMainWindow):
    def __init__(self, simulator):
        super().__init__()
        self.simulator = simulator
        
        self.setWindowTitle("Robot Control Simulator")
        self.setGeometry(100, 100, 800, 600)
        
        # Set dark theme
        palette = QPalette()
        palette.setColor(QPalette.Window, QColor(53, 53, 53))
        palette.setColor(QPalette.WindowText, Qt.white)
        palette.setColor(QPalette.Base, QColor(25, 25, 25))
        palette.setColor(QPalette.AlternateBase, QColor(53, 53, 53))
        palette.setColor(QPalette.ToolTipBase, Qt.white)
        palette.setColor(QPalette.ToolTipText, Qt.white)
        palette.setColor(QPalette.Text, Qt.white)
        palette.setColor(QPalette.Button, QColor(53, 53, 53))
        palette.setColor(QPalette.ButtonText, Qt.white)
        palette.setColor(QPalette.BrightText, Qt.red)
        palette.setColor(QPalette.Link, QColor(42, 130, 218))
        palette.setColor(QPalette.Highlight, QColor(42, 130, 218))
        palette.setColor(QPalette.HighlightedText, Qt.black)
        self.setPalette(palette)
        
        self.init_ui()
        
        # Connect signals using lambda functions to avoid the error
        self.simulator.emitter.command_received.connect(lambda msg: self.update_command_log(msg))
        self.simulator.emitter.driving_received.connect(lambda msg: self.update_driving_log(msg))
        self.simulator.emitter.touch_received.connect(lambda msg: self.update_touch_log(msg))
        self.simulator.emitter.status_sent.connect(lambda msg: self.update_status_log(msg))
        self.simulator.emitter.client_connected.connect(lambda msg: self.update_connection_status(msg, True))
        self.simulator.emitter.client_disconnected.connect(lambda msg: self.update_connection_status(msg, False))
        
        # Status update timer
        self.status_timer = QTimer()
        self.status_timer.timeout.connect(self.update_status_display)
        self.status_timer.start(1000)  # Update every second
    
    def init_ui(self):
        """Initialize user interface"""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        main_layout = QVBoxLayout(central_widget)
        
        # Create tab widget
        tab_widget = QTabWidget()
        main_layout.addWidget(tab_widget)
        
        # Create tabs
        self.create_monitor_tab(tab_widget)
        self.create_control_tab(tab_widget)
        self.create_status_tab(tab_widget)
        
        # Create bottom status bar
        self.status_label = QLabel("Ready")
        main_layout.addWidget(self.status_label)
    
    def create_monitor_tab(self, parent):
        """Create monitoring tab"""
        monitor_tab = QWidget()
        layout = QVBoxLayout(monitor_tab)
        
        # Create text areas for logs
        self.command_log = QTextEdit()
        self.command_log.setReadOnly(True)
        self.command_log.setFont(QFont("Courier", 9))
        
        self.driving_log = QTextEdit()
        self.driving_log.setReadOnly(True)
        self.driving_log.setFont(QFont("Courier", 9))
        
        self.touch_log = QTextEdit()
        self.touch_log.setReadOnly(True)
        self.touch_log.setFont(QFont("Courier", 9))
        
        # Add to layout
        layout.addWidget(QLabel("Command Log:"))
        layout.addWidget(self.command_log, 1)
        
        layout.addWidget(QLabel("Driving Log:"))
        layout.addWidget(self.driving_log, 1)
        
        layout.addWidget(QLabel("Touch Log:"))
        layout.addWidget(self.touch_log, 1)
        
        parent.addTab(monitor_tab, "Monitor")
    
    def create_control_tab(self, parent):
        """Create control tab"""
        control_tab = QWidget()
        layout = QVBoxLayout(control_tab)
        
        # Mode control group
        mode_group = QGroupBox("Mode Control")
        mode_layout = QGridLayout(mode_group)
        
        self.mode_buttons = {}
        for i, (name, id) in enumerate(COMMAND_IDS.items()):
            button = QPushButton(name)
            button.clicked.connect(lambda checked, m=id: self.simulator.set_mode(m))
            self.mode_buttons[id] = button
            mode_layout.addWidget(button, i // 2, i % 2)
        
        layout.addWidget(mode_group)
        
        # Permission control group
        permission_group = QGroupBox("Permission Control")
        permission_layout = QHBoxLayout(permission_group)
        
        self.grant_button = QPushButton("Grant Permission")
        self.grant_button.clicked.connect(lambda: self.simulator.set_permission_granted(True))
        
        self.deny_button = QPushButton("Start/Stop") #Start/stop
        self.deny_button.clicked.connect(lambda: self.simulator.set_permission_granted(False))
        
        permission_layout.addWidget(self.grant_button)
        permission_layout.addWidget(self.deny_button)
        
        layout.addWidget(permission_group)
        
        # Crosshair control
        crosshair_group = QGroupBox("Crosshair Control")
        crosshair_layout = QVBoxLayout(crosshair_group)
        
        self.x_slider = QSlider(Qt.Horizontal)
        self.x_slider.setRange(0, 100)
        self.x_slider.setValue(50)
        self.x_slider.valueChanged.connect(self.update_crosshair)
        
        self.y_slider = QSlider(Qt.Horizontal)
        self.y_slider.setRange(0, 100)
        self.y_slider.setValue(50)
        self.y_slider.valueChanged.connect(self.update_crosshair)
        
        crosshair_layout.addWidget(QLabel("X Position:"))
        crosshair_layout.addWidget(self.x_slider)
        crosshair_layout.addWidget(QLabel("Y Position:"))
        crosshair_layout.addWidget(self.y_slider)
        
        layout.addWidget(crosshair_group)
        
        parent.addTab(control_tab, "Control")
    
    def create_status_tab(self, parent):
        """Create status tab"""
        status_tab = QWidget()
        layout = QVBoxLayout(status_tab)
        
        # Current status display
        self.status_display = QTextEdit()
        self.status_display.setReadOnly(True)
        self.status_display.setFont(QFont("Courier", 10))
        
        layout.addWidget(self.status_display)
        
        parent.addTab(status_tab, "Status")
    
    def update_command_log(self, message):
        """Update command log with new message"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.command_log.append(f"[{timestamp}] {message}")
    
    def update_driving_log(self, message):
        """Update driving log with new message"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.driving_log.append(f"[{timestamp}] {message}")
    
    def update_touch_log(self, message):
        """Update touch log with new message"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.touch_log.append(f"[{timestamp}] {message}")
    
    def update_status_log(self, message):
        """Update status log with new message"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.status_display.append(f"[{timestamp}] {message}")
    
    def update_connection_status(self, message, is_connected):
        """Update connection status"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.status_display.append(f"[{timestamp}] {message}")
    
    def update_status_display(self):
        """Update status display with current simulator state"""
        mode_name = "UNKNOWN"
        for name, id in COMMAND_IDS.items():
            if id == self.simulator.current_mode:
                mode_name = name
                break
        
        status_text = (
            f"Current Mode: {mode_name} ({self.simulator.current_mode})\n"
            f"Permission Request Active: {self.simulator.permission_request_active}\n"
            f"Permission Granted: {self.simulator.permission_granted}\n"
            f"Crosshair Position: ({self.simulator.crosshair_x:.3f}, {self.simulator.crosshair_y:.3f})\n"
            f"Connected Status Clients: {len(self.simulator.status_clients)}"
        )
        
        # Update mode button highlighting
        for id, button in self.mode_buttons.items():
            if id == self.simulator.current_mode:
                button.setStyleSheet("background-color: #3a7d44;")
            else:
                button.setStyleSheet("")
        
        # Update crosshair sliders
        self.x_slider.blockSignals(True)
        self.y_slider.blockSignals(True)
        self.x_slider.setValue(int(self.simulator.crosshair_x * 100))
        self.y_slider.setValue(int(self.simulator.crosshair_y * 100))
        self.x_slider.blockSignals(False)
        self.y_slider.blockSignals(False)
    
    def update_crosshair(self):
        """Update crosshair position from sliders"""
        self.simulator.crosshair_x = self.x_slider.value() / 100.0
        self.simulator.crosshair_y = self.y_slider.value() / 100.0
    
    def closeEvent(self, event):
        """Handle window close event"""
        print("Closing application...")
        self.simulator.stop()
        event.accept()

def main():
    # Set up signal handler for Ctrl+C
    def signal_handler(sig, frame):
        print("\nExiting gracefully...")
        if GUI_AVAILABLE and 'app' in locals():
            app.quit()
        else:
            sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    
    # Create simulator first
    simulator = RobotSimulator()
    simulator.start()
    
    # Try to create GUI if available
    if GUI_AVAILABLE:
        app = QApplication.instance()
        if app is None:
            app = QApplication(sys.argv)
        
        gui = SimulatorGUI(simulator)
        gui.show()
        
        try:
            sys.exit(app.exec_())
        except KeyboardInterrupt:
            pass
    else:
        print("Running in headless mode. Press Ctrl+C to stop.")
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            pass
    
    simulator.stop()

if __name__ == "__main__":
    main()