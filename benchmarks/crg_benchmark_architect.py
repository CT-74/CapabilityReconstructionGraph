import subprocess, os, platform, csv, matplotlib.pyplot as plt

# Paths configuration
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
BIN_DIR = os.path.join(CURRENT_DIR, "bin")
DATA_DIR = os.path.join(CURRENT_DIR, "data")
IMG_DIR = os.path.join(CURRENT_DIR, "..", "img")

SOURCE = os.path.join(CURRENT_DIR, "crg_benchmark_architect.cpp")
EXE = os.path.join(BIN_DIR, "architect.bin" if platform.system() != "Windows" else "architect.exe")
CSV = os.path.join(DATA_DIR, "crg_benchmark_architect.csv")

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
    
    # Execution: C++ writes directly to data/crg_benchmark_architect.csv
    print("Running Architect Benchmark...")
    subprocess.run([EXE], check=True, cwd=CURRENT_DIR)

    # Visualization
    n, ecs, crg = [], [], []
    with open(CSV, 'r') as f:
        reader = csv.reader(f)
        next(reader)
        for row in reader:
            n.append(int(row[0]))
            ecs.append(float(row[1]))
            crg.append(float(row[2]))

    plt.figure(figsize=(10, 6))
    plt.semilogx(n, ecs, 'o--', label='Classic ECS', color='#34495e', alpha=0.6)
    plt.semilogx(n, crg, 'o-', label='CRG Architecture', color='#3498db', linewidth=2)
    plt.title("Hardware Limits & Cache Locality Analysis")
    plt.xlabel("Number of Entities"); plt.ylabel("ns/entity")
    plt.legend(); plt.grid(True, alpha=0.2)
    
    plt.savefig(os.path.join(IMG_DIR, "crg_benchmark_architect.png"), dpi=300)

if __name__ == "__main__":
    run_suite()