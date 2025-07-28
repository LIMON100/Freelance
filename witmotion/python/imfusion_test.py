# import asyncio
# import struct
# import time
# import os
# from bleak import BleakClient
# import numpy as np
# import imufusion

# # --- Device Configuration ---
# DEVICE_MAC_ADDRESS = "D2:3E:5A:7F:6C:3D"
# DATA_CHARACTERISTIC_UUID = "0000ffe4-0000-1000-8000-00805f9a34fb"

# # --- Global State Variables ---
# witmotion_euler = np.zeros(3)
# ahrs = imufusion.Ahrs()
# fusion_euler = np.zeros(3)
# last_update_time = None
# is_synchronized = False  # <-- New flag to check if we have synchronized the Yaw

# def clear_screen():
#     os.system('cls' if os.name == 'nt' else 'clear')

# def data_notification_handler(sender, data: bytearray):
#     global witmotion_euler, fusion_euler, last_update_time, is_synchronized

#     if len(data) >= 20 and data[0] == 0x55 and data[1] == 0x61:
#         try:
#             ax, ay, az, gx, gy, gz, roll, pitch, yaw = struct.unpack('<hhhhhhhhh', data[2:20])

#             witmotion_euler[0] = roll / 32768.0 * 180
#             witmotion_euler[1] = pitch / 32768.0 * 180
#             witmotion_euler[2] = yaw / 32768.0 * 180

#             accel = np.array([ax, ay, az]) / 32768.0 * 16
#             gyro = np.array([gx, gy, gz]) / 32768.0 * 2000

#             # --- SYNCHRONIZATION STEP ---
#             # On the very first valid packet, set our algorithm's heading
#             # to match the device's heading.
#             if not is_synchronized and witmotion_euler[2] != 0:
#                 ahrs.heading = witmotion_euler[2]
#                 is_synchronized = True
#                 print("Yaw has been synchronized!")

#             current_time = time.monotonic()
#             if last_update_time is not None:
#                 delta_time = current_time - last_update_time
#                 ahrs.update_no_magnetometer(gyro, accel, delta_time)
#                 fused_quat = ahrs.quaternion.to_euler()
#                 fusion_euler[:] = fused_quat

#             last_update_time = current_time

#         except (struct.error, IndexError):
#             pass

# async def main(address):
#     print(f"Attempting to connect to {address}...")
#     async with BleakClient(address) as client:
#         if not client.is_connected:
#             print("Failed to connect.")
#             return

#         print(f"Successfully connected.")
#         await client.start_notify(DATA_CHARACTERISTIC_UUID, data_notification_handler)
        
#         print("Waiting for first data packet to synchronize Yaw...")
#         while not is_synchronized:
#             await asyncio.sleep(0.1)

#         while client.is_connected:
#             clear_screen()
#             print("Yaw has been synchronized!")
#             print("-" * 70)
#             print(f"| {'':^28} | {'Witmotion (Internal)':^18} | {'imufusion (Python)':^18} |")
#             print("-" * 70)
#             print(f"| {'Roll':<28} | {witmotion_euler[0]:>18.2f} | {fusion_euler[0]:>18.2f} |")
#             print(f"| {'Pitch':<28} | {witmotion_euler[1]:>18.2f} | {fusion_euler[1]:>18.2f} |")
#             print(f"| {'Yaw':<28} | {witmotion_euler[2]:>18.2f} | {fusion_euler[2]:>18.2f} |")
#             print("-" * 70)
            
#             await asyncio.sleep(0.05)

# if __name__ == "__main__":
#     try:
#         asyncio.run(main(DEVICE_MAC_ADDRESS))
#     except (asyncio.CancelledError, KeyboardInterrupt):
#         print("\nProgram stopped.")
#     except Exception as e:
#         print(f"\nAn error occurred: {e}")




import asyncio
import struct
import time
import os
from bleak import BleakClient
import numpy as np
import imufusion

# --- Device Configuration ---
DEVICE_MAC_ADDRESS = "D2:3E:5A:7F:6C:3D"
DATA_CHARACTERISTIC_UUID = "0000ffe4-0000-1000-8000-00805f9a34fb"

# --- Global State Variables ---
# Raw Inputs
latest_accel = np.zeros(3)
latest_gyro = np.zeros(3)

# Fused Outputs
witmotion_euler = np.zeros(3)
ahrs = imufusion.Ahrs()
fusion_euler = np.zeros(3)

# System State
last_update_time = None
is_synchronized = False

def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

def data_notification_handler(sender, data: bytearray):
    global latest_accel, latest_gyro, witmotion_euler, fusion_euler, last_update_time, is_synchronized

    if len(data) >= 20 and data[0] == 0x55 and data[1] == 0x61:
        try:
            ax, ay, az, gx, gy, gz, roll, pitch, yaw = struct.unpack('<hhhhhhhhh', data[2:20])

            # --- Capture Raw Data (The "Ingredients") ---
            latest_accel[:] = np.array([ax, ay, az]) / 32768.0 * 16
            latest_gyro[:] = np.array([gx, gy, gz]) / 32768.0 * 2000

            # --- Capture Witmotion's Fused Data (Chef A's Cake) ---
            witmotion_euler[0] = roll / 32768.0 * 180
            witmotion_euler[1] = pitch / 32768.0 * 180
            witmotion_euler[2] = yaw / 32768.0 * 180

            # --- Synchronize Yaw on the first run ---
            if not is_synchronized and witmotion_euler[2] != 0:
                ahrs.heading = witmotion_euler[2]
                is_synchronized = True
                print("Yaw has been synchronized!")

            # --- Calculate Our Fused Data (Chef B's Cake) ---
            current_time = time.monotonic()
            if last_update_time is not None:
                delta_time = current_time - last_update_time
                ahrs.update_no_magnetometer(latest_gyro, latest_accel, delta_time)
                fused_quat = ahrs.quaternion.to_euler()
                fusion_euler[:] = fused_quat

            last_update_time = current_time

        except (struct.error, IndexError):
            pass

async def main(address):
    print(f"Attempting to connect to {address}...")
    async with BleakClient(address) as client:
        if not client.is_connected:
            print("Failed to connect.")
            return

        print(f"Successfully connected.")
        await client.start_notify(DATA_CHARACTERISTIC_UUID, data_notification_handler)
        
        print("Waiting for first data packet to synchronize Yaw...")
        while not is_synchronized:
            await asyncio.sleep(0.1)

        while client.is_connected:
            clear_screen()
            print("Yaw has been synchronized!")

            # --- Display Raw Data Inputs ---
            print("\n--- Raw Data Inputs (The 'Ingredients') ---")
            print(f"Accel (g):     X={latest_accel[0]:8.2f}, Y={latest_accel[1]:8.2f}, Z={latest_accel[2]:8.2f}")
            print(f"Gyro (°/s):    X={latest_gyro[0]:8.2f}, Y={latest_gyro[1]:8.2f}, Z={latest_gyro[2]:8.2f}")
            
            # --- Display Fused Outputs Comparison ---
            print("\n--- Fused Outputs (The 'Finished Cakes') ---")
            print("-" * 70)
            print(f"| {'':^28} | {'Witmotion (original data)':^18} | {'imufusion (Python)':^18} |")
            print("-" * 70)
            print(f"| {'Roll':<28} | {witmotion_euler[0]:>18.2f} | {fusion_euler[0]:>18.2f} |")
            print(f"| {'Pitch':<28} | {witmotion_euler[1]:>18.2f} | {fusion_euler[1]:>18.2f} |")
            print(f"| {'Yaw':<28} | {witmotion_euler[2]:>18.2f} | {fusion_euler[2]:>18.2f} |")
            print("-" * 70)
            
            await asyncio.sleep(0.05)

if __name__ == "__main__":
    try:
        asyncio.run(main(DEVICE_MAC_ADDRESS))
    except (asyncio.CancelledError, KeyboardInterrupt):
        print("\nProgram stopped.")
    except Exception as e:
        print(f"\nAn error occurred: {e}")