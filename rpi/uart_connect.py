import serial
import time

PORT = '/dev/serial0'
BAUDRATE = 115_200  # Adjust back to 921600 if your setup handles high-speed mini-UART reliably

ser = serial.Serial(
    port=PORT,
    baudrate=BAUDRATE,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0.5
)

try:
    print(f"Connected to: {ser.name} at {BAUDRATE} baud")
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    while True:
        # 1. Send data
        ser.write(b"Hello from Raspberry Pi 4!\n")
        print("Data sent.")

        # 2. Listen for incoming data for the remainder of the 1-second interval
        start_time = time.time()
        while time.time() - start_time < 1.0:
            if ser.in_waiting > 0:
                raw_data = ser.readline()
                decoded = raw_data.decode('utf-8', errors='replace').strip()
                if decoded:
                    print(f"Received: {decoded}")
            time.sleep(0.02)

except KeyboardInterrupt:
    print("\nProgram stopped by user.")

finally:
    ser.close()
    print("Serial port closed.")