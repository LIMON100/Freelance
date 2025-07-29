import asyncio
import struct
import time
import threading
from bleak import BleakClient
import numpy as np
import imufusion
import cv2 # The OpenCV library

# --- Device Configuration ---
DEVICE_MAC_ADDRESS = "D2:3E:5A:7F:6C:3D"
DATA_CHARACTERISTIC_UUID = "0000ffe4-0000-1000-8000-00805f9a34fb"

# --- Thread-Safe Global State Variables ---
# We use a Lock to prevent race conditions when accessing these from different threads
data_lock = threading.Lock()

# Raw Inputs
latest_accel = np.zeros(3)
latest_gyro = np.zeros(3)

# Fused Outputs
witmotion_euler = np.zeros(3)
fusion_euler = np.zeros(3)

# System State
is_running = True
is_synchronized = False
connection_status = "Disconnected"

# --- BLE Logic (to be run in a separate thread) ---

def data_notification_handler(sender, data: bytearray):
    """Callback for when BLE data is received. This updates the global state."""
    global latest_accel, latest_gyro, witmotion_euler, fusion_euler
    global is_synchronized, last_update_time

    # The AHRS object is only used in this thread, so it doesn't need a lock
    # but the data it reads/writes does.
    
    if len(data) >= 20 and data[0] == 0x55 and data[1] == 0x61:
        try:
            ax, ay, az, gx, gy, gz, roll, pitch, yaw = struct.unpack('<hhhhhhhhh', data[2:20])

            # Use the lock to safely update the shared variables
            with data_lock:
                latest_accel[:] = np.array([ax, ay, az]) / 32768.0 * 16
                latest_gyro[:] = np.array([gx, gy, gz]) / 32768.0 * 2000
                witmotion_euler[0] = roll / 32768.0 * 180
                witmotion_euler[1] = pitch / 32768.0 * 180
                witmotion_euler[2] = yaw / 32768.0 * 180

                current_time = time.monotonic()
                if not is_synchronized and witmotion_euler[2] != 0:
                    ahrs.heading = witmotion_euler[2]
                    is_synchronized = True

                if last_update_time is not None:
                    delta_time = current_time - last_update_time
                    ahrs.update_no_magnetometer(latest_gyro, latest_accel, delta_time)
                    fused_quat = ahrs.quaternion.to_euler()
                    fusion_euler[:] = fused_quat

                last_update_time = current_time

        except (struct.error, IndexError):
            pass

async def ble_main_loop():
    """The main async function for the BLE thread."""
    global connection_status, is_running
    
    while is_running:
        try:
            connection_status = f"Connecting to {DEVICE_MAC_ADDRESS}..."
            async with BleakClient(DEVICE_MAC_ADDRESS) as client:
                if client.is_connected:
                    connection_status = "Connected & Synchronizing..."
                    await client.start_notify(DATA_CHARACTERISTIC_UUID, data_notification_handler)
                    while client.is_connected and is_running:
                        if is_synchronized:
                            connection_status = "Live Data"
                        await asyncio.sleep(1) # Keep connection alive
                    # When loop exits, the 'async with' will handle disconnection
        except Exception as e:
            connection_status = f"Connection failed: {e}"
            await asyncio.sleep(5) # Wait before retrying

def ble_thread_func():
    """The target function for our BLE thread. It sets up and runs the asyncio loop."""
    global ahrs, last_update_time
    ahrs = imufusion.Ahrs()
    last_update_time = time.monotonic()
    
    # Each thread needs its own asyncio event loop
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(ble_main_loop())

# --- Main Thread (GUI and Camera) ---

if __name__ == "__main__":
    # Start the BLE thread in the background
    ble_thread = threading.Thread(target=ble_thread_func, daemon=True)
    ble_thread.start()

    # Initialize the camera
    cap = cv2.VideoCapture(0) # 0 is usually the default webcam
    if not cap.isOpened():
        print("Error: Could not open camera.")
        exit()

    font = cv2.FONT_HERSHEY_SIMPLEX

    while is_running:
        # Capture a frame from the camera
        ret, frame = cap.read()
        if not ret:
            print("Error: Can't receive frame. Exiting ...")
            break

        # Flip the frame horizontally (like a mirror)
        frame = cv2.flip(frame, 1)

        # Safely read the latest data from the shared variables
        with data_lock:
            # Create local copies to use for drawing this frame
            display_accel = latest_accel.copy()
            display_gyro = latest_gyro.copy()
            display_witmotion = witmotion_euler.copy()
            display_fusion = fusion_euler.copy()
            display_status = connection_status

        # --- Draw the UI Overlay ---
        # Draw a semi-transparent background rectangle for readability
        overlay = frame.copy()
        cv2.rectangle(overlay, (10, 10), (450, 270), (0, 0, 0), -1)
        alpha = 0.6  # Transparency factor.
        frame = cv2.addWeighted(overlay, alpha, frame, 1 - alpha, 0)
        
        # Display text
        cv2.putText(frame, f"Status: {display_status}", (20, 40), font, 0.7, (255, 255, 255), 2)
        cv2.putText(frame, "--- Raw Inputs ---", (20, 80), font, 0.6, (0, 255, 255), 1)
        cv2.putText(frame, f"Accel(g): {display_accel[0]:.2f}, {display_accel[1]:.2f}, {display_accel[2]:.2f}", (20, 110), font, 0.5, (255, 255, 255), 1)
        cv2.putText(frame, f"Gyro(dps): {display_gyro[0]:.2f}, {display_gyro[1]:.2f}, {display_gyro[2]:.2f}", (20, 130), font, 0.5, (255, 255, 255), 1)
        
        cv2.putText(frame, "--- Fused Outputs ---", (20, 170), font, 0.6, (0, 255, 255), 1)
        cv2.putText(frame, f"          Witmotion     imufusion", (20, 190), font, 0.5, (255, 255, 255), 1)
        cv2.putText(frame, f"Roll:     {display_witmotion[0]:>8.2f}    {display_fusion[0]:>8.2f}", (20, 210), font, 0.5, (255, 255, 255), 1)
        cv2.putText(frame, f"Pitch:    {display_witmotion[1]:>8.2f}    {display_fusion[1]:>8.2f}", (20, 230), font, 0.5, (255, 255, 255), 1)
        cv2.putText(frame, f"Yaw:      {display_witmotion[2]:>8.2f}    {display_fusion[2]:>8.2f}", (20, 250), font, 0.5, (255, 255, 255), 1)

        # Display the resulting frame
        cv2.imshow('Witmotion IMU Demo (Press Q to quit)', frame)

        # Exit if 'q' is pressed
        if cv2.waitKey(1) & 0xFF == ord('q'):
            is_running = False

    # When everything done, release the capture and destroy windows
    cap.release()
    cv2.destroyAllWindows()
    print("Program stopped.")