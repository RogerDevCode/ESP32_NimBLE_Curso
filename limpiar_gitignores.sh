#!/bin/bash

echo "🔍 Buscando archivos .gitignore redundantes en subcarpetas..."
echo "-----------------------------------------------------------"

# Buscamos archivos llamados .gitignore, pero EXCLUIMOS el de la raíz actual (./.gitignore)
# -type f: Solo archivos
# -name: Con ese nombre
# ! -path: Que NO sea el de la ruta actual
files=$(find . -type f -name ".gitignore" ! -path "./.gitignore")

if [ -z "$files" ]; then
    echo "✅ No se encontraron .gitignore internos. ¡Tu estructura está limpia!"
    exit 0
fi

# Mostrar los archivos encontrados
echo "$files"
echo "-----------------------------------------------------------"
echo "⚠️  ATENCIÓN: Se han encontrado los archivos listados arriba."
echo "   El .gitignore de la raíz (./.gitignore) NO se tocará."
echo ""
read -p "¿Deseas eliminar estos archivos redundantes? (s/N): " confirm

if [[ "$confirm" =~ ^[sS]$ ]]; then
    # Ejecuta el mismo comando find pero con la acción -delete
    find . -type f -name ".gitignore" ! -path "./.gitignore" -delete
    echo "🗑️  Archivos eliminados correctamente."
    echo "👉 Ahora ejecuta 'git status' para ver los cambios."
else
    echo "❌ Operación cancelada. No se borró nada."
fi