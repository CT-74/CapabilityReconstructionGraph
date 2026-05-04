import subprocess, os, platform, csv, matplotlib.pyplot as plt

# Paths configuration
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
BIN_DIR = os.path.join(CURRENT_DIR, "bin")
DATA_DIR = os.path.join(CURRENT_DIR, "data")
IMG_DIR = os.path.join(CURRENT_DIR, "..", "img")

SOURCE = os.path.join(CURRENT_DIR, "crg_benchmark_final.cpp")
EXE = os.path.join(BIN_DIR, "final.bin" if platform.system() != "Windows" else "final.exe")
CSV = os.path.join(DATA_DIR, "crg_benchmark_final.csv")

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
    print("Running Structural Immunity Benchmark...")
    subprocess.run([EXE], check=True, cwd=CURRENT_DIR)

    # Visualization
    data_map = {}
    with open(CSV, 'r') as f:
        reader = csv.reader(f)
        next(reader)
        for row in reader:
            rate = float(row[1])
            if rate not in data_map: 
                data_map[rate] = {'n': [], 'ecs': [], 'crg': []}
            data_map[rate]['n'].append(int(row[0]))
            data_map[rate]['ecs'].append(float(row[2]))
            data_map[rate]['crg'].append(float(row[3]))

    plt.figure(figsize=(10, 6))
    for rate, values in data_map.items():
        plt.semilogx(values['n'], values['ecs'], '--', label=f'ECS ({int(rate*100)}% Mut)')
        plt.semilogx(values['n'], values['crg'], 'o-', label=f'CRG ({int(rate*100)}% Mut)')
    
    plt.title("Structural Immunity Analysis")
    plt.xlabel("Entities"); plt.ylabel("ns/entity")
    plt.legend(); plt.grid(True, alpha=0.2)
    
    plt.savefig(os.path.join(IMG_DIR, "crg_benchmark_final.png"), dpi=300)

if __name__ == "__main__":
    run_suite()