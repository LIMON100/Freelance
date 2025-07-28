import asyncio
import struct
from bleak import BleakClient

# !! IMPORTANT !!
# REPLACE THIS WITH THE MAC ADDRESS OF YOUR WITMOTION IMU
DEVICE_MAC_ADDRESS = "D2:3E:5A:7F:6C:3D" 

# Standard UUIDs for Witmotion devices for data notification
# You might need to check your device's manual if this doesn't work.
# DATA_CHARACTERISTIC_UUID = "0000ffe4-0000-1000-8000-00805f9b34fb"
DATA_CHARACTERISTIC_UUID = "0000ffe4-0000-1000-8000-00805f9a34fb" 

def data_notification_handler(sender, data: bytearray):
    """
    This function is called whenever a new data packet is received.
    It parses the byte array and prints the IMU data.
    """
    # The common data packet starts with 0x55 and has a type of 0x61
    if data[0] == 0x55 and data[1] == 0x61:
        # Unpack the 11 bytes of data following the header
        # The format string '<hhh hhh hhh' means:
        # < : Little-endian
        # h : signed short (2 bytes)
        # We unpack 9 of these signed shorts
        try:
            ax, ay, az, gx, gy, gz, roll, pitch, yaw = struct.unpack('<hhhhhhhhh', data[2:20])

            # Apply scaling factors from the device manual
            accel_x = ax / 32768.0 * 16  # Acceleration in g
            accel_y = ay / 32768.0 * 16  # Acceleration in g
            accel_z = az / 32768.0 * 16  # Acceleration in g

            gyro_x = gx / 32768.0 * 2000 # Angular velocity in °/s
            gyro_y = gy / 32768.0 * 2000 # Angular velocity in °/s
            gyro_z = gz / 32768.0 * 2000 # Angular velocity in °/s
            
            angle_roll = roll / 32768.0 * 180  # Angle in °
            angle_pitch = pitch / 32768.0 * 180 # Angle in °
            angle_yaw = yaw / 32768.0 * 180   # Angle in °

            # Use carriage return to print on the same line, creating a "live" display
            print(
                f"Acc(g): X={accel_x:6.2f} Y={accel_y:6.2f} Z={accel_z:6.2f} | "
                f"Gyro(°/s): X={gyro_x:6.1f} Y={gyro_y:6.1f} Z={gyro_z:6.1f} | "
                f"Angle(°): Roll={angle_roll:6.2f} Pitch={angle_pitch:6.2f} Yaw={angle_yaw:6.2f}",
                end='\r'
            )
        except struct.error:
            # This can happen if a data packet is malformed or incomplete
            pass # Silently ignore malformed packets

async def main(address):
    """
    Connects to the device and subscribes to data notifications.
    """
    if address == "XX:XX:XX:XX:XX:XX":
        print("ERROR: Please replace 'XX:XX:XX:XX:XX:XX' with your device's MAC address.")
        return

    print(f"Attempting to connect to {address}...")
    async with BleakClient(address) as client:
        if client.is_connected:
            print(f"Successfully connected to {address}")
            print("Subscribing to data notifications... Move the IMU to see live data.")
            
            # Subscribe to the characteristic
            await client.start_notify(DATA_CHARACTERISTIC_UUID, data_notification_handler)
            
            # Keep the script running to receive notifications
            # You can change the duration or use a different condition to stop
            while client.is_connected:
                await asyncio.sleep(1)

        else:
            print(f"Failed to connect to {address}")

if __name__ == "__main__":
    try:
        asyncio.run(main(DEVICE_MAC_ADDRESS))
    except asyncio.CancelledError:
        print("Program was cancelled.")
    except Exception as e:
        print(f"An error occurred: {e}")