#!/bin/bash

# S'assurer qu'on s'exécute dans le dossier du script (talk/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
cd "$SCRIPT_DIR"

VENV_DIR=".venv"

echo "========================================"
echo "🎬 CRG Presentation Generator"
echo "========================================"

# 1. Création de l'environnement virtuel s'il n'existe pas
if [ ! -d "$VENV_DIR" ]; then
    echo "📦 Création de l'environnement virtuel Python ($VENV_DIR)..."
    python3 -m venv "$VENV_DIR"
    if [ $? -ne 0 ]; then
        echo "❌ Erreur : Impossible de créer le venv. Assurez-vous que python3-venv est installé."
        exit 1
    fi
else
    echo "✅ Environnement virtuel trouvé."
fi

# 2. Activation du venv
source "$VENV_DIR/bin/activate"

# 3. Installation des dépendances (en mode silencieux pour un log propre)
echo "📥 Installation/Vérification des dépendances..."
pip install --upgrade pip -q
pip install python-pptx qrcode[pil] matplotlib requests -q

# 4. Exécution du générateur de slides
echo "🚀 Génération des slides en cours..."
python3 make_talk.py

# 5. Récupération du code de retour pour savoir si tout s'est bien passé
if [ $? -eq 0 ]; then
    echo "✨ Succès ! Les présentations PowerPoint sont prêtes."
else
    echo "❌ Une erreur est survenue pendant la génération."
fi

# 6. Nettoyage (Désactivation du venv)
deactivate