"""
PURPOSE:
An end-to-end automation script for the "Cost per Entity" benchmark.
Compiles the C++ source into 'bin/', runs it, and generates a highly 
annotated graph saved in the project's global 'img/' folder.
"""
import subprocess
import matplotlib.pyplot as plt
import platform
import matplotlib.ticker as ticker
import os

# --- PATH CONFIGURATION ---
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(CURRENT_DIR, ".."))
BIN_DIR = os.path.join(CURRENT_DIR, "bin")
IMG_DIR = os.path.join(ROOT_DIR, "img")

# Updated source and binary paths
SOURCE_FILE = os.path.join(CURRENT_DIR, "crg_benchmark_bar.cpp")
EXECUTABLE = os.path.join(BIN_DIR, "crg_benchmark_bar.bin")
if platform.system() == "Windows": EXECUTABLE = EXECUTABLE.replace(".bin", ".exe")

ENTITY_COUNTS = [10_000, 50_000, 100_000, 500_000, 1_000_000, 5_000_000, 10_000_000]

def compile_project():
    print(f"🔨 Compiling {SOURCE_FILE} into {BIN_DIR}...")
    os.makedirs(BIN_DIR, exist_ok=True)
    cmd = ["clang++", "-O3", "-march=native", "-std=c++17", SOURCE_FILE, "-o", EXECUTABLE]
    subprocess.run(cmd, check=True)

def run_benchmarks():
    ecs_times, crg_times = [], []
    for count in ENTITY_COUNTS:
        print(f"Testing {count:,} entities...", end="", flush=True)
        res = subprocess.run([EXECUTABLE, str(count)], capture_output=True, text=True)
        parts = res.stdout.strip().split(',')
        if len(parts) == 3:
            ecs_times.append(float(parts[1]))
            crg_times.append(float(parts[2]))
            print(f" ✅")
        else:
            print(" ❌ Parsing error")
    return ecs_times, crg_times

if __name__ == "__main__":
    compile_project()
    ecs_times, crg_times = run_benchmarks()

    # Calculation logic
    ecs_ns_per_entity = [(t * 1_000_000) / c for t, c in zip(ecs_times, ENTITY_COUNTS)]
    crg_ns_per_entity = [(t * 1_000_000) / c for t, c in zip(crg_times, ENTITY_COUNTS)]

    fig, ax = plt.subplots(figsize=(15, 9))
    fig.patch.set_facecolor('#f8f9fa')
    ax.set_facecolor('#ffffff')

    ax.plot(ENTITY_COUNTS, ecs_ns_per_entity, 'o--', color='#e74c3c', linewidth=2.5, label="Classic ECS (Swap & Pop / Sparse Set Updates)")
    ax.plot(ENTITY_COUNTS, crg_ns_per_entity, 's-', color='#2ecc71', linewidth=3.5, label="CRG Architecture (O(1) Data Stability)")
    ax.fill_between(ENTITY_COUNTS, crg_ns_per_entity, ecs_ns_per_entity, color='#e74c3c', alpha=0.1, label="Archetype Poisoning Penalty")

    ax.set_xscale('log')
    max_y = max(max(ecs_ns_per_entity), max(crg_ns_per_entity))
    ax.set_ylim(bottom=0, top=max_y * 1.50) 
    
    def format_entities(x, pos):
        if x >= 1_000_000: return f'{int(x/1_000_000)}M'
        if x >= 1_000: return f'{int(x/1_000)}K'
        return str(int(x))
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(format_entities))

    ax.set_xlabel("Number of Entities processed per frame (Log Scale)", fontsize=12, fontweight='bold')
    ax.set_ylabel("Cost per Entity (Nanoseconds) - LOWER IS BETTER", fontsize=12, fontweight='bold')
    ax.set_title("Per-Entity Cost Analysis: Hardware Limits & Archetype Poisoning", fontsize=16, fontweight='bold', pad=20)

    # L3 Cache Boundary
    l3_boundary = 80000
    ax.axvline(x=l3_boundary, color='#34495e', linestyle=':', linewidth=2.5, zorder=1)
    ax.text(l3_boundary, max_y * 1.40, "L3 Cache Boundary\n(~6.4 MB of Data)", color='#34495e', ha='center', va='center', fontweight='bold', bbox=dict(boxstyle='round', facecolor='#fdfefe', edgecolor='#bdc3c7', alpha=0.9))

    # Annotations and legend[cite: 18]
    ax.legend(loc='lower left', bbox_to_anchor=(0.02, 0.40), fontsize=11)
    ax.grid(True, which="both", ls="-", alpha=0.2)
    
    plt.tight_layout()

    # FINAL UPDATE: Save to central img/ folder without timestamp
    os.makedirs(IMG_DIR, exist_ok=True)
    output_file = os.path.join(IMG_DIR, "crg_cost_per_entity_analysis.png")
    
    plt.savefig(output_file, dpi=150)
    print(f"\n📈 Success: Graph generated at {output_file}")