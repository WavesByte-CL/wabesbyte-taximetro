from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO, emit
import os
import sys
from esptool import main as esptool_main
import serial.tools.list_ports
from firebase_admin import credentials, firestore, initialize_app
from google.cloud import storage
import time
import threading
import subprocess

# Ruta del archivo de credenciales SA
SERVICE_ACCOUNT_PATH = "programador/wavesbyte-taximetro-504536420576.json"
TEMP_BIN_PATH = "./firmware.bin"

# Inicializar Firebase Admin
try:
    print("Inicializando Firebase Admin...")
    cred = credentials.Certificate(SERVICE_ACCOUNT_PATH)
    initialize_app(cred)
    print("Firebase Admin inicializado correctamente.")
except Exception as e:
    print(f"Error al inicializar Firebase Admin: {e}")
    exit(1)

# Obtener cliente Firestore
try:
    db = firestore.client()
    print("Cliente Firestore obtenido correctamente.")
except Exception as e:
    print(f"Error al obtener cliente Firestore: {e}")
    exit(1)

# Configuración Flask y Socket.IO
app = Flask(__name__)
socketio = SocketIO(app)

# Detectar el puerto del dispositivo conectado
def list_serial_ports():
    """
    Lista los puertos seriales que coincidan con los IDs conocidos.
    """
    ids_conocidos = ["1A86:7523", "10C4:EA60", "0403:6001"]  # IDs conocidos
    ports = serial.tools.list_ports.comports()
    return [
        {"device": port.device, "description": port.description, "hwid": port.hwid}
        for port in ports
        if any(id_conocido in port.hwid for id_conocido in ids_conocidos)
    ]

# Emitir periódicamente los puertos disponibles
def emit_ports_periodically():
    while True:
        try:
            ports = list_serial_ports()
            socketio.emit("update_ports", ports)
        except Exception as e:
            print(f"Error al emitir puertos: {e}")
        time.sleep(2)

# Iniciar hilo para emitir puertos
threading.Thread(target=emit_ports_periodically, daemon=True).start()

# Descargar el binario desde Google Cloud Storage
def download_binary(gcs_path):
    """
    Descarga el archivo binario desde Google Cloud Storage.
    """
    try:
        if not gcs_path.startswith("gs://"):
            raise ValueError("La ruta no es válida para GCS.")

        # Parsear bucket y archivo
        gcs_path = gcs_path[5:]  # Remover 'gs://'
        bucket_name, *file_path_parts = gcs_path.split('/')
        file_path = "/".join(file_path_parts)

        # Inicializar cliente de almacenamiento
        storage_client = storage.Client.from_service_account_json(SERVICE_ACCOUNT_PATH)
        bucket = storage_client.bucket(bucket_name)
        blob = bucket.blob(file_path)

        # Descargar el archivo
        blob.download_to_filename(TEMP_BIN_PATH)
        print(f"Binario descargado correctamente a {TEMP_BIN_PATH}")
        return TEMP_BIN_PATH
    except Exception as e:
        print(f"Error descargando binario desde GCS: {e}")
        raise

# Ejecutar un Cloud Run Job con parámetros
def run_cloud_run_job_with_env(project_id, region, job_name, parameters, args=None):
    """
    Ejecuta un Cloud Run Job y actualiza variables de entorno con parámetros específicos.
    """
    try:
        env_vars = ",".join([f"{key}={value}" for key, value in parameters.items()])
        command = [
            "gcloud", "run", "jobs", "execute", job_name,
            f"--project={project_id}",
            f"--region={region}",
            f"--update-env-vars={env_vars}"
        ]
        if args:
            command.append(f"--args={','.join(args)}")

        print("Ejecutando comando:", " ".join(command))
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if result.returncode != 0:
            raise Exception(result.stderr)
        print(f"Resultado del comando: {result.stdout}")
        return result.stdout
    except Exception as e:
        print(f"Error ejecutando el comando: {e}")
        raise

# Escuchar cambios en Firestore para un job específico
def listen_to_job_status(doc_path, port):
    """
    Registra un listener para escuchar cambios en el documento de Firestore.
    """
    def on_snapshot(doc_snapshot, changes, read_time):
        for doc in doc_snapshot:
            log_data = doc.to_dict()
            status = log_data.get("status", "unknown")
            print(f"Estado actualizado en Firestore: {status}")

            # Emitir estado actualizado al cliente
            socketio.emit("log_update", {"status": f"Firestore Status: {status}"})

            # Si el estado es `completed`, programar el ESP32
            if status == "completed":
                try:
                    binary_path = log_data.get("path")
                    if binary_path:
                        socketio.emit("log_update", {"status": "Descargando binario..."})
                        download_binary(binary_path)
                    
                    socketio.emit("log_update", {"status": "Programando ESP32..."})
                    program_status = program_esp32(port)
                    socketio.emit("log_update", {"status": program_status})
                except Exception as e:
                    error_message = f"Error en la programación del ESP32: {str(e)}"
                    socketio.emit("log_update", {"status": error_message})

    doc_ref = db.document(doc_path)
    doc_ref.on_snapshot(on_snapshot)

# Programar el ESP32 usando esptool
def program_esp32(port, baud_rate="115200"):
    """
    Programa el ESP32 con el binario descargado.
    """
    try:
        firmware_path = TEMP_BIN_PATH
        if not os.path.isfile(firmware_path):
            raise FileNotFoundError(f"El archivo firmware.bin no se encuentra en la raíz del proyecto: {firmware_path}")

        command = [
            "--port", port,
            "--baud", baud_rate,
            "write_flash",
            "--flash_mode", "dio",
            "--flash_size", "4MB",
            "0x10000", firmware_path
        ]
        print("Ejecutando esptool con los siguientes argumentos:", command)

        sys.argv = ["esptool.py"] + command
        esptool_main()
        return "ESP32 programado exitosamente."
    except Exception as e:
        print(f"Error durante la programación del ESP32: {e}")
        raise

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/execute_and_program", methods=["POST"])
def execute_and_program():
    """
    Ejecuta el Cloud Run Job y programa el ESP32 basado en el resultado.
    """
    try:
        parameters = request.form.to_dict()
        project_id = "wavesbyte-taximetro"
        region = "us-central1"
        job_name = "esp32-compiler"
        args = ["/workspace/compile_and_upload.py"]

        # Validar puerto seleccionado
        port = parameters.get("port")
        if not port:
            return jsonify({"status": "error", "message": "Seleccione un puerto antes de ejecutar el trabajo."})

        # Ejecutar el Cloud Run Job
        socketio.emit("log_update", {"status": "Ejecutando Cloud Run Job..."})
        run_cloud_run_job_with_env(project_id, region, job_name, parameters, args)

        # Registrar listener en Firestore
        doc_path = f"logs/{parameters['USER']}/{time.strftime('%Y-%m-%d')}/{parameters['UUID']}"
        listen_to_job_status(doc_path, port)

        return jsonify({"status": "success", "message": "Cloud Run Job iniciado y monitoreando Firestore."})
    except Exception as e:
        error_message = f"Error al ejecutar el trabajo: {str(e)}"
        print(error_message)
        return jsonify({"status": "error", "message": error_message})

if __name__ == "__main__":
    socketio.run(app, debug=True)
