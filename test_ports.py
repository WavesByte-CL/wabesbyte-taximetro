import serial.tools.list_ports

def list_ports():
    print("Buscando dispositivos...")
    ports = list(serial.tools.list_ports.comports())
    for port in ports:
        print(f"Puerto encontrado: {port.device} - HWID: {port.hwid}")

list_ports()
