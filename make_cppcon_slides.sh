#!/bin/bash

# --- CONFIGURATION ---
# On récupère le chemin où se trouve le script pour être sûr de travailler
# depuis la racine, peu importe d'où on l'appelle.
PROJECT_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$PROJECT_ROOT"

echo "------------------------------------------------"
echo "🚀 CRG PRESENTATION GENERATOR"
echo "------------------------------------------------"

# 1. Activation de l'environnement virtuel
if [ -f "benchmarks/venv/bin/activate" ]; then
    echo "📦 Activating virtual environment..."
    source benchmarks/venv/bin/activate
else
    echo "❌ Error: venv not found in benchmarks/. Run 'python3 -m venv benchmarks/venv' first."
    exit 1
fi

# 2. Lancement du générateur
# Ce script va automatiquement appeler run_all.py dans benchmarks/
# puis générer les PPTX dans cppcon/
echo "🛠️  Updating benchmarks and generating slides..."
python3 cppcon/generate_decks.py

# 3. Nettoyage
echo "🧹 Deactivating environment..."
deactivate

echo "------------------------------------------------"
echo "✅ Done! Check your 'cppcon' folder for the PPTX files."
echo "------------------------------------------------"