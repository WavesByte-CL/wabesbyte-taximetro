import os
import sys
from esptool import main as esptool_main

def program_esp32(port, baud_rate="115200"):
    """
    Programa un ESP32 utilizando esptool.py desde una carpeta local.

    Args:
        port (str): El puerto donde está conectado el ESP32 (e.g., /dev/ttyUSB0 o COM3).
        baud_rate (str): Velocidad en baudios para la programación (por defecto 115200).

    Returns:
        None
    """
    try:
        # Verificar si el archivo firmware existe
        firmware_path = os.path.join(os.getcwd(), "firmware.bin")
        if not os.path.isfile(firmware_path):
            raise FileNotFoundError(f"El archivo firmware.bin no se encuentra en la raíz del proyecto: {firmware_path}")

        # Configurar el comando para esptool
        command = [
            "--port", port,
            "--baud", baud_rate,
            "write_flash",
            "--flash_mode", "dio",
            "--flash_size", "4MB",
            "0x10000", firmware_path
        ]
        print("Ejecutando esptool con los siguientes argumentos:", command)

        # Llamar a esptool desde la carpeta local
        sys.argv = ["esptool.py"] + command
        esptool_main()

        print("El ESP32 ha sido programado exitosamente.")
    except Exception as e:
        print(f"Error durante la programación del ESP32: {e}")


if __name__ == "__main__":
    # Solicitar al usuario el puerto
    port = "/dev/cu.usbserial-2130"
    baud_rate = "115200"

    # Programar el ESP32
    program_esp32(port, baud_rate)
