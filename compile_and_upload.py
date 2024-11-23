import os
import subprocess
from google.cloud import storage, firestore
import uuid
from datetime import datetime

# Configuración de variables
CODE_BUCKET = os.getenv("CODE_BUCKET", "code-bucket")
OUTPUT_BUCKET = os.getenv("OUTPUT_BUCKET", "output-bucket")
FILE_NAME = os.getenv("FILE_NAME", "wb.ino")
CONFIG_FILE = os.getenv("CONFIG_FILE", "config.h")
BIN_NAME = os.getenv("BIN_NAME", "firmware.bin")
BOARD = os.getenv("BOARD", "esp32:esp32:esp32")
WORKSPACE = "/workspace"
USER = os.getenv("USER", "unknown-user")  # Usuario que activa la función
RUN_UUID = os.getenv("UUID", str(uuid.uuid4()))  # UUID recibido como variable de entorno o generado automáticamente
CURRENT_DATE = datetime.now().strftime("%Y-%m-%d")  # Fecha actual en formato YYYY-MM-DD

# Variables específicas del archivo `config.h`
PATENTE = os.getenv("PATENTE", "SIN-PATENTE")
RESOLUCION = os.getenv("RESOLUCION", "DEFAULT")
TARIFA_INICIAL = os.getenv("TARIFA_INICIAL", "0.0")
TARIFA_CAIDA_PARCIAL_METROS = os.getenv("TARIFA_CAIDA_PARCIAL_METROS", "0.0")
TARIFA_CAIDA_PARCIAL_MINUTO = os.getenv("TARIFA_CAIDA_PARCIAL_MINUTO", "0.0")
COLOR_FONDO_PANTALLA = os.getenv("COLOR_FONDO_PANTALLA", "0x000000")
COLOR_LETRAS_PANTALLA = os.getenv("COLOR_LETRAS_PANTALLA", "0xFFFFFF")
COLOR_PRECIO_PANTALLA = os.getenv("COLOR_PRECIO_PANTALLA", "0xFFFF00")
MOSTRAR_VELOCIDAD_EN_PANTALLA = os.getenv("MOSTRAR_VELOCIDAD_EN_PANTALLA", "false")
PROPAGANDA_1 = os.getenv("PROPAGANDA_1", "")
PROPAGANDA_2 = os.getenv("PROPAGANDA_2", "")
PROPAGANDA_3 = os.getenv("PROPAGANDA_3", "")
PROPAGANDA_4 = os.getenv("PROPAGANDA_4", "")

# Inicializa Google Cloud Storage
storage_client = storage.Client()
code_bucket = storage_client.bucket(CODE_BUCKET)
output_bucket = storage_client.bucket(OUTPUT_BUCKET)

# Inicializa Firestore
firestore_client = firestore.Client()

def log_to_firestore(status, message=None, path=None, env_vars=None):
    """
    Registra logs en Firestore.
    """
    log_data = {
        "user": USER,
        "uuid": RUN_UUID,
        "date": CURRENT_DATE,
        "patente": PATENTE,
        "status": status,
        "message": message,
        "path": path,
        "env_vars": env_vars,
        "timestamp": datetime.utcnow()
    }
    # Crear la estructura de Firestore: logs/user/date/uuid/document
    doc_ref = firestore_client.collection("logs") \
        .document(USER) \
        .collection(CURRENT_DATE) \
        .document(RUN_UUID)
    doc_ref.set(log_data)
    print(f"Log registrado en Firestore: {log_data}")

def get_env_vars():
    """
    Obtiene todas las variables de entorno relevantes para incluirlas en los logs.
    """
    return {
        "PATENTE": PATENTE,
        "RESOLUCION": RESOLUCION,
        "TARIFA_INICIAL": TARIFA_INICIAL,
        "TARIFA_CAIDA_PARCIAL_METROS": TARIFA_CAIDA_PARCIAL_METROS,
        "TARIFA_CAIDA_PARCIAL_MINUTO": TARIFA_CAIDA_PARCIAL_MINUTO,
        "COLOR_FONDO_PANTALLA": COLOR_FONDO_PANTALLA,
        "COLOR_LETRAS_PANTALLA": COLOR_LETRAS_PANTALLA,
        "COLOR_PRECIO_PANTALLA": COLOR_PRECIO_PANTALLA,
        "MOSTRAR_VELOCIDAD_EN_PANTALLA": MOSTRAR_VELOCIDAD_EN_PANTALLA,
        "PROPAGANDA_1": PROPAGANDA_1,
        "PROPAGANDA_2": PROPAGANDA_2,
        "PROPAGANDA_3": PROPAGANDA_3,
        "PROPAGANDA_4": PROPAGANDA_4,
        "CODE_BUCKET": CODE_BUCKET,
        "OUTPUT_BUCKET": OUTPUT_BUCKET,
        "FILE_NAME": FILE_NAME,
        "CONFIG_FILE": CONFIG_FILE,
        "BIN_NAME": BIN_NAME,
        "BOARD": BOARD,
        "USER": USER,
        "UUID": RUN_UUID,
        "DATE": CURRENT_DATE
    }

def download_file(bucket, remote_path, local_path):
    """Descarga un archivo desde Google Cloud Storage."""
    print(f"Descargando {remote_path}...")
    blob = bucket.blob(remote_path)
    os.makedirs(os.path.dirname(local_path), exist_ok=True)
    blob.download_to_filename(local_path)
    if not os.path.exists(local_path):
        raise FileNotFoundError(f"El archivo {remote_path} no se descargó correctamente.")
    print(f"Archivo descargado en {local_path}.")

def setup_sketch():
    """Descarga y organiza el sketch y sus dependencias."""
    print("Configurando sketch...")
    sketch_name = os.path.splitext(FILE_NAME)[0]  # Obtener el nombre sin extensión
    sketch_dir = os.path.join(WORKSPACE, sketch_name)  # Usar el nombre del archivo como nombre del directorio
    os.makedirs(sketch_dir, exist_ok=True)

    # Descargar el archivo principal del sketch
    download_file(code_bucket, FILE_NAME, os.path.join(sketch_dir, FILE_NAME))

    # Descargar el archivo de configuración
    download_file(code_bucket, CONFIG_FILE, os.path.join(sketch_dir, CONFIG_FILE))

    print(f"Sketch configurado en {sketch_dir}.")
    return sketch_dir

def edit_config_file(config_path):
    """
    Edita el archivo config.h con las variables de entorno proporcionadas.
    Mantiene intactas las líneas no especificadas.
    """
    print(f"Editando archivo de configuración: {config_path}")

    # Variables de entorno a reemplazar
    replacements = {
        "PATENTE": f'"{PATENTE}"',
        "RESOLUCION": RESOLUCION,
        "TARIFA_INICIAL": TARIFA_INICIAL,
        "TARIFA_CAIDA_PARCIAL_METROS": TARIFA_CAIDA_PARCIAL_METROS,
        "TARIFA_CAIDA_PARCIAL_MINUTO": TARIFA_CAIDA_PARCIAL_MINUTO,
        "COLOR_FONDO_PANTALLA": COLOR_FONDO_PANTALLA,
        "COLOR_LETRAS_PANTALLA": COLOR_LETRAS_PANTALLA,
        "COLOR_PRECIO_PANTALLA": COLOR_PRECIO_PANTALLA,
        "MOSTRAR_VELOCIDAD_EN_PANTALLA": MOSTRAR_VELOCIDAD_EN_PANTALLA,
        "PROPAGANDA_1": f'"{PROPAGANDA_1}"',
        "PROPAGANDA_2": f'"{PROPAGANDA_2}"',
        "PROPAGANDA_3": f'"{PROPAGANDA_3}"',
        "PROPAGANDA_4": f'"{PROPAGANDA_4}"',
    }

    # Leer el archivo config.h
    with open(config_path, "r") as file:
        lines = file.readlines()

    # Reemplazar las líneas correspondientes
    for key, value in replacements.items():
        for i, line in enumerate(lines):
            if line.startswith(f"#define {key} "):  # Solo reemplaza las líneas que coinciden
                lines[i] = f"#define {key} {value}\n"
                break

    # Guardar los cambios en el archivo
    with open(config_path, "w") as file:
        file.writelines(lines)

    print(f"Archivo {config_path} actualizado correctamente.")

def compile_code(sketch_dir):
    """Compila el código fuente usando Arduino CLI."""
    print("Compilando el código fuente...")
    try:
        compile_command = [
            "arduino-cli", "compile",
            "--fqbn", BOARD,
            "--libraries", "/workspace/lib",  # Usar la carpeta `lib` pre-copiada en el contenedor
            "--output-dir", os.path.join(sketch_dir, "build"),
            sketch_dir
        ]
        # Ejecutar el comando y capturar salida/errores
        result = subprocess.run(compile_command, text=True, capture_output=True)
        if result.returncode != 0:
            print("Error durante la compilación:")
            print(f"STDOUT:\n{result.stdout}")
            print(f"STDERR:\n{result.stderr}")
            raise subprocess.CalledProcessError(result.returncode, compile_command)
    except subprocess.CalledProcessError as e:
        log_to_firestore("error", message=str(e), env_vars=get_env_vars())
        raise RuntimeError(f"Fallo al compilar el sketch: {e}")

    # Buscar el archivo binario generado
    bin_path = os.path.join(sketch_dir, "build", f"{os.path.splitext(FILE_NAME)[0]}.ino.bin")
    if not os.path.exists(bin_path):
        raise FileNotFoundError(f"Binario {bin_path} no encontrado.")
    print(f"Binario generado en {bin_path}.")
    return bin_path

def upload_binary(bin_path):
    """Sube el binario al bucket de Google Cloud Storage."""
    output_folder = f"{USER}/{CURRENT_DATE}/{RUN_UUID}/{PATENTE}"  # Crear una carpeta con USER, fecha, UUID y PATENTE
    output_path = f"{output_folder}/{BIN_NAME}"
    print(f"Subiendo el binario a {output_path}...")
    blob = output_bucket.blob(output_path)
    blob.upload_from_filename(bin_path)
    # Generar la URL final del archivo en Google Cloud Storage
    final_path = f"gs://{OUTPUT_BUCKET}/{output_path}"
    print(f"Binario subido a {final_path}.")
    log_to_firestore("success", path=final_path, env_vars=get_env_vars())
    return final_path

def main():
    try:
        print(f"Iniciando ejecución con UUID: {RUN_UUID}, USER: {USER}, y PATENTE: {PATENTE}")
        log_to_firestore("start", message="Proceso iniciado", env_vars=get_env_vars())

        # Configurar el sketch con sus dependencias
        sketch_dir = setup_sketch()

        # Editar el archivo de configuración
        config_path = os.path.join(sketch_dir, CONFIG_FILE)
        edit_config_file(config_path)

        # Compilar el código
        bin_file = compile_code(sketch_dir)

        # Subir el binario al bucket de destino
        final_path = upload_binary(bin_file)

        log_to_firestore("completed", message="Proceso completado con éxito", path=final_path, env_vars=get_env_vars())
        print("Proceso completado exitosamente.")
    except Exception as e:
        log_to_firestore("failed", message=str(e), env_vars=get_env_vars())
        print(f"Error: {e}")

if __name__ == "__main__":
    main()
