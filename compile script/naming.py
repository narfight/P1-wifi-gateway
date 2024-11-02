Import("env")
import os
import re

def extract_define_value(file_path, define_name):
    try:
        with open(file_path, 'r') as f:
            content = f.read()
            # Recherche le pattern #define DEFINE_NAME value
            pattern = rf'#define\s+{define_name}\s+[\""]?([^\""\n]+)[\""]?'
            match = re.search(pattern, content)
            if match:
                return match.group(1)
    except Exception as e:
        print(f"Erreur lors de la lecture du fichier: {e}")
    return None

# Chemin vers votre fichier source contenant le DEFINE
source_file = "src/GlobalVar.h"  # Ajustez le chemin selon votre structure

Src_Name = "HOSTNAME"
Src_Version = "VERSION"

# Extraction de la valeur
Name = extract_define_value(source_file, Src_Name)
Language = env.GetProjectOption("custom_language")
Version = extract_define_value(source_file, Src_Version)

# Modification du nom du programme
program_name = f"{Name}-{Language}-{Version}"
env.Replace(PROGNAME=program_name)
print(f"Nom du programme défini à: {program_name}")