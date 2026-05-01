"""
PURPOSE:
Automates the compilation and execution of the raw performance benchmark.
Calculates memory bandwidth (Gi/s) to analyze the 'Dispatch Tax'.
"""
import subprocess
import matplotlib.pyplot as plt
import platform
import matplotlib.ticker as ticker
import os

# Paths and configuration
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(CURRENT_DIR, ".."))
BIN_DIR = os.path.join(CURRENT_DIR, "bin")
DATA_DIR = os.path.join(CURRENT_DIR, "data")
IMG_DIR = os.path.join(ROOT_DIR, "img")

SOURCE_FILE = "crg_compare_perf.cpp"
EXECUTABLE = os.path.join(BIN_DIR, "crg_compare_perf.bin")
if platform.system() == "Windows": EXECUTABLE = EXECUTABLE.replace(".bin", ".exe")

ENTITY_COUNTS = [10_000, 100_000, 1_000_000, 10_000_000]

def compile_project():
    print(f"🔨 Compiling {SOURCE_FILE}...")
    os.makedirs(BIN_DIR, exist_ok=True)
    cmd = ["clang++", "-O3", "-march=native", "-std=c++17", SOURCE_FILE, "-o", EXECUTABLE]
    subprocess.run(cmd, check=True)

def run_benchmarks():
    ecs_bws, crg_bws = [], []
    os.makedirs(DATA_DIR, exist_ok=True)
    
    for count in ENTITY_COUNTS:
        print(f"Testing {count:,} entities...", end="", flush=True)
        res = subprocess.run([EXECUTABLE, str(count)], capture_output=True, text=True)
        parts = res.stdout.strip().split(',')
        if len(parts) == 5:
            ecs_bws.append(float(parts[3])) # ECS Bandwidth
            crg_bws.append(float(parts[4])) # CRG Bandwidth
            print(f" ✅")
    return ecs_bws, crg_bws

if __name__ == "__main__":
    compile_project()
    ecs, crg = run_benchmarks()
    
    # Plotting logic (Simplified for brevity, similar to previous bandwidth plot)
    plt.figure(figsize=(12, 7))
    plt.semilogx(ENTITY_COUNTS, ecs, 'o--', label='Pure ECS Bandwidth')
    plt.semilogx(ENTITY_COUNTS, crg, 's-', label='CRG Bandwidth')
    plt.title("Dispatch Tax vs Memory Latency (Bandwidth Analysis)")
    plt.ylabel("Gi/s")
    plt.legend()
    
    os.makedirs(IMG_DIR, exist_ok=True)
    plt.savefig(os.path.join(IMG_DIR, "crg_bandwidth_analysis.png"))