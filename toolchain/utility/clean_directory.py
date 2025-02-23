import sys
import os
import shutil

def clean_directory(output_dir):
    # Erstelle das Verzeichnis, falls es nicht existiert
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    # Lösche alle Dateien im Verzeichnis
    for filename in os.listdir(output_dir):
        file_path = os.path.join(output_dir, filename)
        try:
            if os.path.isfile(file_path) or os.path.islink(file_path):
                os.unlink(file_path)
            elif os.path.isdir(file_path):
                shutil.rmtree(file_path)
        except Exception as e:
            print(f'Fehler beim Löschen von {file_path}. Grund: {e}')

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Benutzung: python clean_directory.py <OUTPUT_DIR>")
        sys.exit(1)
    
    output_dir = sys.argv[1]
    clean_directory(output_dir)
