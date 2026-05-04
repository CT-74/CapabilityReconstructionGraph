import subprocess, os, platform, csv, matplotlib.pyplot as plt

# Paths configuration
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
BIN_DIR = os.path.join(CURRENT_DIR, "bin")
DATA_DIR = os.path.join(CURRENT_DIR, "data")
IMG_DIR = os.path.join(CURRENT_DIR, "..", "img")

SOURCE = os.path.join(CURRENT_DIR, "crg_benchmark_bar.cpp")
EXE = os.path.join(BIN_DIR, "bar.bin" if platform.system() != "Windows" else "bar.exe")
CSV = os.path.join(DATA_DIR, "crg_benchmark_bar.csv")

def run_suite():
    if not os.path.exists(SOURCE):
        print(f"Error: {SOURCE} not found.")
        return

    os.makedirs(BIN_DIR, exist_ok=True)
    os.makedirs(DATA_DIR, exist_ok=True)
    os.makedirs(IMG_DIR, exist_ok=True)
    
    # Compilation
    print(f"Compiling {os.path.basename(SOURCE)}...")
    subprocess.run(["clang++", "-O3", "-march=native", "-std=c++17", SOURCE, "-o", EXE], check=True)
    
    # Execution
    print("Running Mutation Impact Benchmark...")
    subprocess.run([EXE], check=True, cwd=CURRENT_DIR)

    # Visualization
    with open(CSV, 'r') as f:
        # On lit toutes les lignes, on prend la dernière[cite: 9]
        data = list(csv.reader(f))[-1]
        
        # On extrait N (data[0]), ECS (data[1]) et CRG (data[2])
        n_entities = int(data[0])
        ecs_val, crg_val = float(data[1]), float(data[2])

    print(f"✅ Data loaded: N={n_entities:,}, ECS={ecs_val}ms, CRG={crg_val}ms")

    plt.figure(figsize=(8, 6))
    
    # Ajout des valeurs au-dessus des barres pour une lecture instantanée
    bars = plt.bar(['ECS (Structural)', 'CRG (Value)'], [ecs_val, crg_val], color=['#e74c3c', '#2ecc71'])
    for bar in bars:
        yval = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2, yval + (yval * 0.02), f'{yval:.1f} ms', ha='center', va='bottom', fontweight='bold')

    # Mise à jour du titre avec le nombre d'entités (N)
    plt.title(f"Mutation Impact (5% State Transition Rate)\nN = {n_entities:,} Entities", fontsize=14, pad=15)
    plt.ylabel("Total Execution Time (ms)")
    
    # Ajustement des marges pour ne pas couper le texte
    plt.ylim(0, max(ecs_val, crg_val) * 1.15)
    plt.tight_layout()
    
    output_img = os.path.join(IMG_DIR, "crg_benchmark_bar.png")
    plt.savefig(output_img, dpi=300)
    print(f"📈 Graph successfully saved to {output_img}")

if __name__ == "__main__":
    run_suite()