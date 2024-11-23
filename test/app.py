import firebase_admin
from firebase_admin import credentials, firestore

# Ruta del archivo de credenciales
SERVICE_ACCOUNT_PATH = "test/wavesbyte-taximetro-504536420576.json"  # Actualiza la ruta

# Inicializar Firebase Admin explícitamente
try:
    print("Inicializando Firebase Admin...")
    cred = credentials.Certificate(SERVICE_ACCOUNT_PATH)
    firebase_admin.initialize_app(cred)
    print("Firebase Admin inicializado correctamente.")
except ValueError as e:
    print("Firebase Admin ya estaba inicializado:", e)
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

# Ruta del documento en Firestore
DOCUMENT_PATH = "logs/Unknown/2024-11-23/ccad7ab8-a576-4a69-bcac-160fae51c7e9"

# Leer el documento inicial para verificar la conexión
try:
    print(f"Intentando leer el documento en: {DOCUMENT_PATH}")
    doc = db.document(DOCUMENT_PATH).get()
    if doc.exists:
        print("Documento encontrado. Contenido inicial:")
        data = doc.to_dict()
        for key, value in data.items():
            print(f"{key}: {value}")
    else:
        print("El documento no existe en Firestore.")
except Exception as e:
    print(f"Error al leer el documento: {e}")

# Listener para cambios en el documento
def on_snapshot(doc_snapshot, changes, read_time):
    """Callback que se ejecuta cuando el documento cambia."""
    print("Listener activado. Detectando cambios...")
    for doc in doc_snapshot:
        if doc.exists:
            # Leer todos los campos del documento
            data = doc.to_dict()
            print("=== Documento Actualizado ===")
            for key, value in data.items():
                print(f"{key}: {value}")
            print("=============================")

            # Detectar cambios en el campo `status`
            if data.get("status") == "completed":
                print("El estado cambió a 'completed'. ¡Acción tomada!")
        else:
            print("El documento no existe.")

# Registrar el listener en Firestore
try:
    print(f"Intentando registrar el listener en: {DOCUMENT_PATH}")
    doc_ref = db.document(DOCUMENT_PATH)
    doc_watch = doc_ref.on_snapshot(on_snapshot)
    print("Listener registrado correctamente.")
except Exception as e:
    print(f"Error al registrar el listener: {e}")

# Mantener el script activo
print("Presiona CTRL+C para detener...")
try:
    while True:
        pass
except KeyboardInterrupt:
    print("Script terminado por el usuario.")
