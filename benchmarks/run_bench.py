import subprocess
import matplotlib.pyplot as plt
import platform
import matplotlib.ticker as ticker

SOURCE_FILE = "crg_benchmark.cpp"
EXECUTABLE = "crg_benchmark.exe" if platform.system() == "Windows" else "./crg_benchmark"
ENTITY_COUNTS = [10_000, 50_000, 100_000, 500_000, 1_000_000, 5_000_000, 10_000_000]

def compile_project():
    print(f"🔨 Compiling {SOURCE_FILE}...")
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

    # Conversion en Nanosecondes par Entité
    ecs_ns_per_entity = [(t * 1_000_000) / c for t, c in zip(ecs_times, ENTITY_COUNTS)]
    crg_ns_per_entity = [(t * 1_000_000) / c for t, c in zip(crg_times, ENTITY_COUNTS)]

    # --- FORMATTING ---
    fig, ax = plt.subplots(figsize=(15, 9))
    fig.patch.set_facecolor('#f8f9fa')
    ax.set_facecolor('#ffffff')

    # Courbes
    ax.plot(ENTITY_COUNTS, ecs_ns_per_entity, 'o--', color='#e74c3c', linewidth=2.5, label="Classic ECS (Swap & Pop / Sparse Set Updates)")
    ax.plot(ENTITY_COUNTS, crg_ns_per_entity, 's-', color='#2ecc71', linewidth=3.5, label="CRG Architecture (O(1) Data Stability)")
    
    ax.fill_between(ENTITY_COUNTS, crg_ns_per_entity, ecs_ns_per_entity, color='#e74c3c', alpha=0.1, label="Archetype Poisoning Penalty")

    # Axes
    ax.set_xscale('log')
    
    # On ajoute énormément d'espace en haut (50%) pour le diagramme matériel
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

    # ==========================================
    # L'OVERLAY ARCHITECTURE MATÉRIELLE (LES FLÈCHES)
    # ==========================================
    
    # 1. Le Mur du Cache L3 (Ligne verticale)
    l3_boundary = 80000
    ax.axvline(x=l3_boundary, color='#34495e', linestyle=':', linewidth=2.5, zorder=1)
    ax.text(l3_boundary, max_y * 1.40, "L3 Cache Boundary\n(~6.4 MB of Data)", 
            color='#34495e', ha='center', va='center', fontweight='bold', 
            bbox=dict(boxstyle='round', facecolor='#fdfefe', edgecolor='#bdc3c7', alpha=0.9))

    # 2. Flèche Instruction Bound (Gauche)
    ax.annotate('', xy=(10000, max_y * 1.30), xytext=(l3_boundary, max_y * 1.30),
                arrowprops=dict(arrowstyle='<->', color='#8e44ad', lw=2.5))
    ax.text(28000, max_y * 1.32, "INSTRUCTION BOUND (L1/L2 CACHE)\nmemcpy() overhead > Pointer overhead", 
            color='#8e44ad', ha='center', va='bottom', fontsize=10, fontweight='bold')

    # 3. Flèche Memory Bound (Droite)
    ax.annotate('', xy=(l3_boundary, max_y * 1.30), xytext=(10000000, max_y * 1.30),
                arrowprops=dict(arrowstyle='<->', color='#d35400', lw=2.5))
    ax.text(800000, max_y * 1.32, "MEMORY BOUND (RAM)\nECS suffers massive Cache Misses on Sparse Set updates", 
            color='#d35400', ha='center', va='bottom', fontsize=10, fontweight='bold')

    # ==========================================
    # TEXTES EXPLICATIFS CLASSIQUES
    # ==========================================
    props = dict(boxstyle='round', facecolor='white', alpha=0.95, edgecolor='gray')
    
    # Explication ECS
    ax.text(0.35, 0.65, 
            "Classic ECS Degradation:\nBeyond the L3 Cache, the ECS RAM accesses\nbecome unpredictable due to entity mutations.\nThe unit cost skyrockets to RAM latency limits.", 
            transform=ax.transAxes, fontsize=11, bbox=props, va='top', ha='left', zorder=5)
            
    # Explication CRG
    ax.text(0.98, 0.25, 
            "CRG Flatline:\nData remains perfectly contiguous.\nThe cost per entity is strictly O(1)\nregardless of the dataset size.", 
            transform=ax.transAxes, fontsize=11, bbox=props, va='center', ha='right', zorder=5)

    # Flèche verticale "RAM Latency Gap" (à la fin)
    if ecs_ns_per_entity[-1] > crg_ns_per_entity[-1]:
        ax.annotate('', xy=(ENTITY_COUNTS[-1], crg_ns_per_entity[-1]), 
                    xytext=(ENTITY_COUNTS[-1], ecs_ns_per_entity[-1]),
                    arrowprops=dict(arrowstyle='<->', color='#c0392b', lw=2.5, shrinkA=5, shrinkB=5))
        mid_y = (ecs_ns_per_entity[-1] + crg_ns_per_entity[-1]) / 2
        ax.text(ENTITY_COUNTS[-1] * 0.85, mid_y, 
                "The RAM\nLatency Gap", 
                color='#c0392b', fontweight='bold', fontsize=11, 
                ha='right', va='center', bbox=dict(boxstyle='round', facecolor='white', alpha=0.9, edgecolor='none'))

    # Petite flèche pour expliquer le démarrage de l'ECS
    ax.annotate("ECS pays Swap&Pop\noverhead in ALU cycles", 
                xy=(10000, ecs_ns_per_entity[0]), 
                xytext=(12000, max_y * 0.4),
                arrowprops=dict(facecolor='#7f8c8d', arrowstyle='->', connectionstyle="arc3,rad=-0.2"),
                fontsize=9, color='#7f8c8d', fontweight='bold')

    ax.legend(loc='lower left', bbox_to_anchor=(0.02, 0.40), fontsize=11)
    ax.grid(True, which="both", ls="-", alpha=0.2)
    
    plt.tight_layout()
    output_file = "crg_cost_per_entity_architect.png"
    plt.savefig(output_file, dpi=150)
    print(f"\n📈 Graph successfully generated: {output_file}")
    plt.show()