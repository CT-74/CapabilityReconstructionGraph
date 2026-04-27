"""
PURPOSE:
Automates the compilation and execution of the raw bandwidth benchmark from the current directory. 
Calculates effective memory bandwidth (Gi/s) to prove that the 'Dispatch Tax' 
is eclipsed by main memory latency.
"""
import subprocess
import matplotlib.pyplot as plt
import platform
import matplotlib.ticker as ticker
from datetime import datetime

# Files are in the current directory
SOURCE_FILE = "crg_compare_perf.cpp"
EXECUTABLE = "crg_compare_perf.exe" if platform.system() == "Windows" else "./crg_compare_perf"
ENTITY_COUNTS = [10_000, 50_000, 100_000, 500_000, 1_000_000, 5_000_000, 10_000_000]

def compile_project():
    print(f"🔨 Compiling {SOURCE_FILE}...")
    cmd = ["clang++", "-O3", "-march=native", "-std=c++17", SOURCE_FILE, "-o", EXECUTABLE]
    subprocess.run(cmd, check=True)

def run_benchmarks():
    ecs_bws, crg_bws = [], []
    for count in ENTITY_COUNTS:
        print(f"Testing {count:,} entities...", end="", flush=True)
        res = subprocess.run([EXECUTABLE, str(count)], capture_output=True, text=True)
        parts = res.stdout.strip().split(',')
        if len(parts) == 5:
            ecs_bws.append(float(parts[3]))
            crg_bws.append(float(parts[4]))
            print(f" ✅")
        else:
            print(" ❌ Parsing error")
    return ecs_bws, crg_bws

if __name__ == "__main__":
    compile_project()
    ecs_bws, crg_bws = run_benchmarks()

    fig, ax = plt.subplots(figsize=(15, 9))
    fig.patch.set_facecolor('#f8f9fa')
    ax.set_facecolor('#ffffff')

    ax.plot(ENTITY_COUNTS, ecs_bws, 'o--', color='#7f8c8d', linewidth=2.5, label="Pure ECS (Hardcoded Loop / Compiler SIMD Auto-vectorization)")
    ax.plot(ENTITY_COUNTS, crg_bws, 's-', color='#3498db', linewidth=3.5, label="CRG Architecture (Dynamic Logic Injection / Function Pointers)")
    ax.fill_between(ENTITY_COUNTS, crg_bws, ecs_bws, color='#7f8c8d', alpha=0.15, label="The 'Dispatch Tax' (Indirection Cost)")

    ax.set_xscale('log')
    max_y = max(max(ecs_bws), max(crg_bws))
    ax.set_ylim(bottom=0, top=max_y * 1.45) 
    
    def format_entities(x, pos):
        if x >= 1_000_000: return f'{int(x/1_000_000)}M'
        if x >= 1_000: return f'{int(x/1_000)}K'
        return str(int(x))
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(format_entities))

    ax.set_xlabel("Number of Entities in contiguous memory (Log Scale)", fontsize=12, fontweight='bold')
    ax.set_ylabel("Bandwidth (Gi/s) - HIGHER IS BETTER", fontsize=12, fontweight='bold')
    ax.set_title("Raw Performance: The CPU Dispatch Tax vs Main Memory Latency", fontsize=16, fontweight='bold', pad=20)

    l3_boundary = 80000
    ax.axvline(x=l3_boundary, color='#34495e', linestyle=':', linewidth=2.5, zorder=1)
    ax.text(l3_boundary, max_y * 1.35, "L3 Cache Boundary\n(~6.4 MB of Data)", color='#34495e', ha='center', va='center', fontweight='bold', bbox=dict(boxstyle='round', facecolor='#fdfefe', edgecolor='#bdc3c7', alpha=0.9))

    ax.annotate('', xy=(10000, max_y * 1.25), xytext=(l3_boundary, max_y * 1.25), arrowprops=dict(arrowstyle='<->', color='#2c3e50', lw=2.5))
    ax.text(28000, max_y * 1.27, "COMPUTE BOUND (L1/L2 CACHE)\nCompiler vectorizes ECS loop.\nCRG pays the Dispatch Tax.", color='#2c3e50', ha='center', va='bottom', fontsize=10, fontweight='bold')

    ax.annotate('', xy=(l3_boundary, max_y * 1.25), xytext=(10000000, max_y * 1.25), arrowprops=dict(arrowstyle='<->', color='#2980b9', lw=2.5))
    ax.text(800000, max_y * 1.27, "MEMORY BOUND (MAIN RAM)\nCPU starves for data. The Dispatch Tax is hidden by RAM latency.\nCRG Bandwidth converges with ECS.", color='#2980b9', ha='center', va='bottom', fontsize=10, fontweight='bold')

    props = dict(boxstyle='round', facecolor='white', alpha=0.95, edgecolor='gray')
    ax.text(0.02, 0.15, "The Pure ECS Illusion:\nAn array of identical entities processed by a single function\nis incredibly fast in cache (gray line), but games require\npolymorphism, which breaks this ideal scenario.", transform=ax.transAxes, fontsize=11, bbox=props, va='bottom', ha='left', zorder=5)
    ax.text(0.98, 0.55, "The CRG Trade-off:\nCRG sacrifices some cache-level throughput to enable O(1) logic injection.\nOnce we hit the RAM limit, this overhead becomes negligible,\nproving that OOP flexibility is effectively 'free' at scale.", transform=ax.transAxes, fontsize=11, bbox=props, va='center', ha='right', zorder=5)

    ax.legend(loc='upper right', bbox_to_anchor=(0.98, 0.90), fontsize=11)
    ax.grid(True, which="both", ls="-", alpha=0.2)
    
    plt.tight_layout()

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"crg_raw_performance_architect_{timestamp}.png"
    
    plt.savefig(output_file, dpi=150)
    print(f"\n📈 Graph successfully generated: {output_file}")