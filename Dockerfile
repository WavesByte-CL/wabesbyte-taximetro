FROM python:3.10-slim

# Instalación de dependencias necesarias
RUN apt-get update && apt-get install -y \
    curl \
    git \
    build-essential \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Instalar Arduino CLI
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=/usr/local/bin sh

# Configurar Arduino CLI y actualizar índices
RUN arduino-cli core update-index && \
    arduino-cli core install esp32:esp32@2.0.14

# Instalar dependencias de Python
COPY requirements.txt /workspace/requirements.txt
RUN pip install --no-cache-dir -r /workspace/requirements.txt

# Crear directorio de trabajo
WORKDIR /workspace

# Copiar el script principal
COPY compile_and_upload.py /workspace/compile_and_upload.py

# Copiar la carpeta `lib` al contenedor
COPY lib /workspace/lib

# Mostrar el contenido inicial del contenedor (opcional para depuración)
RUN ls -R /workspace

# Comando predeterminado
CMD ["python", "/workspace/compile_and_upload.py"]
