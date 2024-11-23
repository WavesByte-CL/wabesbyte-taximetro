import os
import requests
import subprocess
import serial.tools.list_ports
from google.cloud import storage
from google.oauth2 import service_account
from google.auth.transport.requests import Request
from urllib.parse import urlencode
from datetime import datetime
import re
import sys
import esptool
from io import StringIO
import tempfile
import json

def get_chip_info(port):
    captured_output = StringIO()
    old_stdout = sys.stdout
    sys.stdout = captured_output
    try:
        esptool.main([
            "--port", port,
            "read_mac"
        ])
    except Exception as e:
        print("Error al obtener el ID del Taxímetro WavesByte:", e, file=old_stdout)
    finally:
        sys.stdout = old_stdout 

    output = captured_output.getvalue()
    mac_line = next((line for line in output.split('\n') if "MAC:" in line), None)
    if mac_line:
        mac_address = mac_line.split(' ')[-1].strip()
        mac_address = mac_address.replace(":", "").upper()
        return mac_address
    else:
        print("No se pudo encontrar el ID del Taxímetro WavesByte en la salida.")
        raise Exception("No se pudo encontrar el ID del Taxímetro WavesByte en la salida.")

def run_esptool_command(command):
    full_command = [sys.executable, '-m', 'esptool'] + command
    return subprocess.run(full_command, capture_output=True, text=True)

def get_resource_path(relative_path):
    if getattr(sys, 'frozen', False):
        base_path = sys._MEIPASS
    else:
        base_path = os.path.dirname(os.path.abspath(__file__))

    return os.path.join(base_path, relative_path)

def read_mac(port):
    result = run_esptool_command(['--port', port, 'read_mac'])
    if result.returncode == 0:
        mac_address = re.search(r"MAC: ([\da-fA-F:]{17})", result.stdout)
        if mac_address:
            cleaned_mac_address = mac_address.group(1).replace(':', '').upper()
            print(f"ID del Taxímetro WavesByte: {cleaned_mac_address}")
            return cleaned_mac_address
        else:
            return "ID del Taxímetro WavesByte no encontrada."
    else:
        return "Error al leer el ID del Taxímetro WavesByte: " + result.stderr

def call_cloud_function(serialNumber, date, patente="XXXX-XX", n_pulsos=1, resolucion=190.1, tarifa_inicial=3, tarifa_caida_mts=4, tarifa_caida_min=5, color_fondo="TFT_BLACK", color_letras="TFT_BLACK", color_numeros="TFT_BLACK", velocidad_pantalla="false", propaganda_1="", propaganda_2="", propaganda_3="", propaganda_4="", timeout=300):
    service_account_file = get_resource_path('data.json')
    base_url = 'https://us-central1-wavesbyte-taximeter.cloudfunctions.net/function-wavesbyte-compilator'
    
    query_params = {
        "serialNumber": serialNumber,
        "date": date,
        "patente": patente,
        "n_pulsos": n_pulsos,
        "resolucion": resolucion,
        "tarifa_inicial": tarifa_inicial,
        "tarifa_caida_mts": tarifa_caida_mts,
        "tarifa_caida_min": tarifa_caida_min,
        "color_fondo": color_fondo,
        "color_letras": color_letras,
        "color_numeros": color_numeros,
        "velocidad_pantalla": velocidad_pantalla,
        "propaganda_1": propaganda_1,
        "propaganda_2": propaganda_2,
        "propaganda_3": propaganda_3,
        "propaganda_4": propaganda_4
    }
    url_with_params = f"{base_url}?{urlencode(query_params)}"
    credentials = service_account.IDTokenCredentials.from_service_account_file(
        service_account_file,
        target_audience=base_url
    )
    auth_req = Request()
    credentials.refresh(auth_req)
    id_token = credentials.token
    headers = {
        "Authorization": f"Bearer {id_token}",
        "Content-Type": "application/json"
    }
    
    try:
        response = requests.post(url_with_params, headers=headers, json={}, timeout=timeout)
        if response.status_code == 200:
            print("Compilación ejecutada exitosamente.")
            return True
        else:
            print(f"Error al compilar código: {response.status_code}, {response.text}")
            raise Exception(f"Error al compilar código: {response.status_code}, {response.text}")
    except requests.exceptions.Timeout:
        print("El proceso ha superado el tiempo máximo de espera.")
        raise TimeoutError("El proceso ha superado el tiempo máximo de espera. Intentelo nuevamente.")

def download_blob(bucket_name, source_blob_name, destination_file_name):
    temp_dir = tempfile.mkdtemp()
    destination_path = os.path.join(temp_dir, destination_file_name)

    service_account_file = get_resource_path('data.json')
    credentials = service_account.Credentials.from_service_account_file(service_account_file)
    storage_client = storage.Client(credentials=credentials)

    bucket = storage_client.bucket(bucket_name)
    blob = bucket.blob(source_blob_name)
    blob.download_to_filename(destination_path)
    return destination_path

def get_ports():
    print("Buscando taxímetro conectado...")
    puerto_conectado = False
    puertos = serial.tools.list_ports.comports()
    ids_conocidos = ["1A86:7523",
                     "10C4:EA60",
                     "0403:6001"]
    for puerto in puertos:
        print(f"Revisando puerto {puerto.device} con HWID {puerto.hwid}")
        if any(id_conocido in puerto.hwid for id_conocido in ids_conocidos):
            print(f"Taxímetro WavesByte encontrado en el puerto {puerto.device}.")
            puerto_conectado = True
            return puerto.device
    print("Taxímetro WavesByte no encontrado.")
    return puerto_conectado

def programming_esp32(port, file_path):
    flash_mode = "dio"
    flash_size = "4MB"
    flash_address = "0x10000"

    try:
        esptool.main([
            "--port", port,
            "write_flash",
            "--flash_mode", flash_mode,
            "--flash_size", flash_size,
            flash_address,
            file_path
        ])
        print("El Taxímetro WavesByte ha sido programado exitosamente.")
    except Exception as e:
        print("Error al programar el Taxímetro WavesByte:", e)
        raise Exception("Error al programar el Taxímetro WavesByte:", e)

def FirebaseLogin(email, password, api_key="AIzaSyBEtlP-c9JiSyBABgIwLvBdMtEkVboYnro"):
    print("Iniciando sesión en sistema...")
    url = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword"
    headers = {"Content-Type": "application/json"}
    data = {
        "email": email,
        "password": password,
        "returnSecureToken": True
    }
    params = {"key": api_key}
    response = requests.post(url, headers=headers, json=data, params=params)

    if response.status_code == 200:
        print("Inicio de sesión exitoso.")
        user_info = response.json()
        return user_info
    else:
        print("Error al iniciar sesión.")
        return None

def get_project_root():
    return os.path.dirname(os.path.abspath(__file__))

def WriteDataToJsonFile(data, filename="data.json"):
    root_path = get_project_root()
    file_path = os.path.join(root_path, filename)
    with open(file_path, 'w') as file:
        file.write(data)

def FirebaseExtractData(user_info, document_path="wavesbyte/sa", project_id="wavesbyte-taximeter"):
    print("Extrayendo datos...")
    url = f"https://firestore.googleapis.com/v1/projects/{project_id}/databases/(default)/documents/{document_path}"
    headers = {"Authorization": f"Bearer {user_info['idToken']}"}
    response = requests.get(url, headers=headers)
    if response.status_code == 200:
        print("Datos recopilados correctamente.")
    else:
        print("Error al obtener los datos.")
    code = response.json()['fields']['wb-sa']['stringValue']
    return code

def run_process(username, password, patente, n_pulsos, resolucion, tarifa_inicial, tarifa_caida_mts, tarifa_caida_min, color_fondo, color_letras, color_numeros, velocidad_pantalla, propaganda_1, propaganda_2, propaganda_3, propaganda_4):
    try:
        user_info = FirebaseLogin(username, password)
        data_json = FirebaseExtractData(user_info)
        WriteDataToJsonFile(data_json)
        port = get_ports()
        if not port:
            print("No se encontró el taxímetro conectado. Verifique la conexión y vuelva a intentarlo.")
            return
        serialNumber = get_chip_info(port)
        date = datetime.now().strftime("%Y-%m-%d")
        print(f"ID Taxímetro WavesByte: {serialNumber}")
        print(f"Fecha: {date}")
        print("Ejecutando programa en la nube. Esto puede tardar unos minutos...")
        resultado = call_cloud_function(serialNumber, date, patente, n_pulsos, resolucion, tarifa_inicial, tarifa_caida_mts, tarifa_caida_min, color_fondo, color_letras, color_numeros, velocidad_pantalla, propaganda_1, propaganda_2, propaganda_3, propaganda_4)

        if resultado:
            file_path = download_blob('wavesbyte', f'wb-compilator/latest/taximetros/{serialNumber}/{date}/build/wb.bin', 'wb.bin')
            print("Programando Taxímetro WavesByte...")
            programming_esp32(port, file_path)
        else:
            print("No se puede continuar debido a un error con la función.")
    except Exception as e:
        print(f"Ocurrió un error: {str(e)}")
    finally:
        if 'file_path' in locals() and os.path.exists(file_path):
            os.remove(file_path)
        if 'data.json' in locals() and os.path.exists("data.json"):
            os.remove("data.json")
        print("Proceso finalizado.")

if __name__ == '__main__':
    username = "cibtronic@gmail.com"
    password = "123456"
    patente = "XXXX-XX"
    n_pulsos = 1
    resolucion = 190.1
    tarifa_inicial = 3
    tarifa_caida_mts = 4
    tarifa_caida_min = 5
    color_fondo = "TFT_BLACK"
    color_letras = "TFT_BLACK"
    color_numeros = "TFT_BLACK"
    velocidad_pantalla = "false"
    propaganda_1 = ""
    propaganda_2 = ""
    propaganda_3 = ""
    propaganda_4 = ""

    run_process(username, password, patente, n_pulsos, resolucion, tarifa_inicial, tarifa_caida_mts, tarifa_caida_min, color_fondo, color_letras, color_numeros, velocidad_pantalla, propaganda_1, propaganda_2, propaganda_3, propaganda_4)
