# ESP32 Compiler & Uploader

Este proyecto automatiza la compilación de código para ESP32 y su despliegue en Google Cloud Storage. Utiliza Docker, Arduino CLI y Google Cloud SDK.

## Estructura del repositorio

- `docker/`: Archivos relacionados con el contenedor Docker.
- `scripts/`: Scripts de automatización en Python.
- `workspace/`: Carpeta temporal donde se descargan y generan los archivos.
- `config/`: Configuración y credenciales de Google Cloud.
- `docs/`: Documentación del proyecto.

## Requisitos previos

1. **Google Cloud Storage**:
   - Un bucket para el código fuente (`CODE_BUCKET`).
   - Un bucket para los binarios compilados (`OUTPUT_BUCKET`).
2. **Arduino CLI**: Configurado para compilar código de ESP32.
3. **Credenciales de Google Cloud**:
   - Archivo JSON de la cuenta de servicio con acceso a los buckets.
   - Configura `GOOGLE_APPLICATION_CREDENTIALS` para usar las credenciales.
4. Variables de entorno:
   - `CODE_BUCKET`: Bucket para el código fuente.
   - `OUTPUT_BUCKET`: Bucket para los binarios compilados.
   - `FILE_NAME`: Nombre del archivo de código fuente.
   - `CONFIG_FILE`: Nombre del archivo de configuración.
   - `BIN_NAME`: Nombre del archivo binario.

## Uso

1. Construir el contenedor:
   ```bash
   docker build -t esp32-compiler .
   ```

2. Ejecutar el contenedor:
   ```bash
   docker run -it esp32-compiler
   -e CODE_BUCKET=your-code-bucket
   -e OUTPUT_BUCKET=your-output-bucket
   -e FILE_NAME=your-file-name
   -e CONFIG_FILE=your-config-file
   -e BIN_NAME=your-bin-name
   ```
3. Verificar el contenido del bucket de salida:
   ```bash
   gsutil ls gs://your-output-bucket
   ```
