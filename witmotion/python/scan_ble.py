import asyncio
from bleak import BleakScanner

async def main():
    """
    Scans for BLE devices and prints their details.
    """
    print("Scanning for BLE devices for 10 seconds...")

    # Discover devices for 10 seconds
    devices = await BleakScanner.discover(timeout=10.0)

    if not devices:
        print("No BLE devices found. Make sure your Witmotion IMU is turned on and in range.")
        return

    print(f"Found {len(devices)} devices:")
    print("-" * 30)
    for i, device in enumerate(devices):
        # The device name might be "WT" followed by its model, e.g., "WT901"
        # Or it might be None if the name is not broadcasted
        device_name = device.name if device.name else "Unknown"
        print(f"{i+1:2d}: {device.address} - {device_name}")
    print("-" * 30)
    print("Look for a device with a name like 'WT...' and note its MAC address.")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except Exception as e:
        print(f"An error occurred: {e}")