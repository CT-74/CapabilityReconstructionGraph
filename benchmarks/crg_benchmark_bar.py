import subprocess, os, platform, csv, matplotlib.pyplot as plt

# Paths configuration[cite: 16]
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
    
    # Compilation[cite: 16]
    print(f"Compiling {os.path.basename(SOURCE)}...")
    subprocess.run(["clang++", "-O3", "-march=native", "-std=c++17", SOURCE, "-o", EXE], check=True)
    
    # Execution[cite: 6]
    print("Running Mutation Impact Benchmark...")
    subprocess.run([EXE], check=True, cwd=CURRENT_DIR)

    # Visualization[cite: 16]
    with open(CSV, 'r') as f:
        data = list(csv.reader(f))[-1]
        ecs_val, crg_val = float(data[1]), float(data[2])

    plt.figure(figsize=(8, 6))
    plt.bar(['ECS (Structural)', 'CRG (Value)'], [ecs_val, crg_val], color=['#e74c3c', '#2ecc71'])
    plt.title("Mutation Impact (5% State Transition Rate)")
    plt.ylabel("Total Execution Time (ms)")
    
    plt.savefig(os.path.join(IMG_DIR, "crg_benchmark_bar.png"), dpi=300)

if __name__ == "__main__":
    run_suite()