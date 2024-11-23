import subprocess

def run_cloud_run_job_with_env(project_id, region, job_name, parameters, args=None):
    """
    Ejecuta una Cloud Run Job con variables de entorno y argumentos usando gcloud.

    Args:
        project_id (str): ID del proyecto de GCP.
        region (str): Región de la Job.
        job_name (str): Nombre de la Cloud Run Job.
        parameters (dict): Diccionario con las variables de entorno.
        args (list): Lista de argumentos para la Job.

    Returns:
        output (str): Salida del comando gcloud.
    """
    # Construye las variables de entorno
    env_vars = ",".join([f"{key}={value}" for key, value in parameters.items()])
    
    # Construye el comando base
    command = [
        "gcloud", "run", "jobs", "execute", job_name,
        f"--project={project_id}",
        f"--region={region}",
        f"--update-env-vars={env_vars}"
    ]
    
    # Si hay argumentos, agrégalos
    if args:
        command.append(f"--args={','.join(args)}")
    
    # Ejecuta el comando
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    if result.returncode != 0:
        print("Error ejecutando la Job:", result.stderr)
        raise Exception(result.stderr)

    print("Job ejecutada exitosamente.")
    return result.stdout

# Configuración
project_id = "wavesbyte-taximetro"
region = "us-central1"
job_name = "esp32-compiler"
parameters = {
    "CODE_BUCKET": "codigo-taximetro",
    "OUTPUT_BUCKET": "codigo-taximetro-binario",
    "FILE_NAME": "wb.ino",
    "BIN_NAME": "firmware.bin",
    "BOARD": "esp32:esp32:esp32",
    "CONFIG_FILE": "config.h",
    "USER": "Unknown",
    "UUID": "581d97c7-26e4-40e7-8c89-554a19b793e3",
    "PATENTE": "LFXL-13",
    "RESOLUCION": "1000.0000",
    "TARIFA_INICIAL": "450",
    "TARIFA_CAIDA_PARCIAL_METROS": "200",
    "TARIFA_CAIDA_PARCIAL_MINUTO": "200",
    "COLOR_FONDO_PANTALLA": "TFT_BLACK",
    "COLOR_LETRAS_PANTALLA": "TFT_YELLOW",
    "COLOR_PRECIO_PANTALLA": "TFT_WHITE",
    "MOSTRAR_VELOCIDAD_EN_PANTALLA": "false",
    "PROPAGANDA_1": "www.wavesbyte.cl",
    "PROPAGANDA_2": "+56929641632",
    "PROPAGANDA_3": "-",
    "PROPAGANDA_4": "-"
}
args = ["/workspace/compile_and_upload.py"]  # Argumentos adicionales

# Ejecutar la Job
response = run_cloud_run_job_with_env(project_id, region, job_name, parameters, args)
print("Respuesta:", response)
