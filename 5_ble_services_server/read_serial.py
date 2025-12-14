import serial
import time

def read_serial():
    try:
        ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=10)
        print("Connected to ESP32 serial port")
        time.sleep(2)  # Wait for the device to be ready
        
        # Flush input to clear any old data
        ser.flushInput()
        
        print("Reading serial output from ESP32:")
        start_time = time.time()
        
        while time.time() - start_time < 10:  # Read for 10 seconds
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore')
                if line:
                    print(line, end='')
            
        ser.close()
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    read_serial()