import asyncio
from bleak import BleakClient

# !! IMPORTANT !!
# MAKE SURE THIS IS THE CORRECT MAC ADDRESS FOR YOUR DEVICE
DEVICE_MAC_ADDRESS = "D2:3E:5A:7F:6C:3D" 

async def main(address):
    """
    Connects to a BLE device and lists all its services and characteristics.
    """
    print(f"Connecting to {address} to discover services...")
    
    try:
        async with BleakClient(address) as client:
            if not client.is_connected:
                print(f"Failed to connect to {address}")
                return

            print(f"Connected to {address}. Discovering services...")
            print("-" * 50)
            
            # Iterate through all services found on the device
            for service in client.services:
                print(f"[Service] {service.uuid}: {service.description}")
                
                # For each service, iterate through its characteristics
                for char in service.characteristics:
                    print(f"  [Characteristic] {char.uuid}:")
                    print(f"    Description: {char.description}")
                    print(f"    Properties: {', '.join(char.properties)}")
                    # The properties tell you what you can do with a characteristic:
                    # 'read', 'write', 'notify', 'indicate', etc.
                    # We are looking for one with 'notify' for streaming data.

                print("-" * 50)

    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    if DEVICE_MAC_ADDRESS == "XX:XX:XX:XX:XX:XX":
        print("ERROR: Please set the DEVICE_MAC_ADDRESS variable in the script.")
    else:
        asyncio.run(main(DEVICE_MAC_ADDRESS))