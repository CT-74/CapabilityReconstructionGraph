import os
import subprocess
import sys

# Fichiers
SRC_FILE = "crg_benchmark.cpp"
OUT_FILE = "crg_bench_runner"

# Commande de compilation pour Mac Apple Silicon avec Google Benchmark (via Homebrew)
compile_cmd = [
    "clang++",
    "-O3",               # Optimisation maximale (essentiel pour tester la RAM/Cache)
    "-std=c++17",        # Standard C++
    SRC_FILE,
    "-isystem", "/opt/homebrew/include",  # Headers Google Benchmark (Apple Silicon)
    "-L/opt/homebrew/lib",                # Libs Google Benchmark (Apple Silicon)
    "-lbenchmark",
    "-lbenchmark_main",  # Fournit le int main() automatiquement
    "-lpthread",
    "-o", OUT_FILE
]

print(f"🔨 Compilation : {' '.join(compile_cmd)}")
compile_result = subprocess.run(compile_cmd, capture_output=True, text=True)

if compile_result.returncode != 0:
    print("❌ Erreur de compilation :\n")
    print(compile_result.stderr)
    sys.exit(1)

print("✅ Compilation réussie (-O3).")
print(f"🚀 Lancement du benchmark (Ne touche plus à rien, laisse le CPU focus)...\n")
print("-" * 60)

# Exécution
try:
    # On laisse la sortie standard s'afficher en direct dans la console
    subprocess.run(["./" + OUT_FILE])
except KeyboardInterrupt:
    print("\n🛑 Benchmark interrompu.")
except Exception as e:
    print(f"\n❌ Erreur lors de l'exécution : {e}")