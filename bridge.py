import asyncio
import serial
import json

# pip install bleak websockets
from bleak import BleakClient, BleakScanner
import websockets

# BLE UUIDs (Nordic UART Service)
SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
TX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

# Uno USB port — update this to match your system
UNO_USB_PORT = '/dev/cu.usbmodem1101'

connected_clients = set()

async def bridge_logic():
    print("Scanning for ESP32_Sensor...")
    device = await BleakScanner.find_device_by_name("ESP32_Sensor", timeout=10)
    if not device:
        print("ESP32_Sensor not found! Make sure it's powered on.")
        return

    print(f"Found: {device.name} ({device.address})")

    try:
        uno = serial.Serial(UNO_USB_PORT, 115200, timeout=1)
        print(f"Uno connected: {UNO_USB_PORT}")
    except Exception as e:
        print(f"Uno connection failed: {e}")
        print("Tip: Close Arduino IDE Serial Monitor first!")
        return

    def on_notify(sender, data):
        raw_data = data.decode('utf-8').strip()
        if not raw_data:
            return
        # Forward to Uno
        uno.write((raw_data + '\n').encode())

        # Forward to browser via WebSocket
        if connected_clients:
            try:
                p = raw_data.split(',')
                if len(p) == 6:
                    msg = json.dumps({
                        "ax":float(p[0]), "ay":float(p[1]), "az":float(p[2]),
                        "gx":float(p[3]), "gy":float(p[4]), "gz":float(p[5])
                    })
                    for c in connected_clients:
                        asyncio.ensure_future(c.send(msg))
            except ValueError:
                pass

        print(f"  -> {raw_data}")

    async def handler(websocket):
        connected_clients.add(websocket)
        print(f"Browser connected ({len(connected_clients)} clients)")
        try:
            async for _ in websocket: pass
        finally:
            connected_clients.remove(websocket)
            print(f"Browser disconnected ({len(connected_clients)} clients)")

    async with BleakClient(device) as client:
        print("BLE connected!")
        await client.start_notify(TX_CHAR_UUID, on_notify)

        async with websockets.serve(handler, "localhost", 8765):
            print("Bridge running!")
            print("  BLE -> Uno (serial)")
            print("  BLE -> Browser (ws://localhost:8765)")
            print(f"  Open: http://localhost:9000/mar27_monitoring.html")

            while client.is_connected:
                await asyncio.sleep(1)

    print("BLE disconnected.")

if __name__ == "__main__":
    asyncio.run(bridge_logic())
