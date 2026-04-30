import os
import numpy as np
import matplotlib.pyplot as plt

def generate_breakeven_plot():
    # Entity sizes from 8 bytes to 256 bytes
    sizes = np.linspace(8, 256, 100)
    
    # CRG overhead vs Static ECS baseline
    overhead_crg_static = 0.05
    # ECS Penalty: Base swap cost + copying N bytes
    cost_ecs_pop_base = 1.0
    cost_ecs_pop_byte = 0.02
    # CRG Penalty: Just a pointer update
    cost_crg_update = 1.0
    
    # R = Taux de mutation
    rates = overhead_crg_static / (cost_ecs_pop_byte * sizes + cost_ecs_pop_base - cost_crg_update + 0.1)
    rates_percent = rates * 100
    
    fig, ax = plt.subplots(figsize=(10, 6))
    
    ax.plot(sizes, rates_percent, color='#e41a1c', linewidth=3, label='Break-Even Curve (Cost ECS = Cost CRG)')
    
    # Regions
    ax.fill_between(sizes, rates_percent, 100, color='#2ca02c', alpha=0.2, label='CRG Supremacy (Zero-Migration Advantage)')
    ax.fill_between(sizes, 0, rates_percent, color='#1f77b4', alpha=0.2, label='ECS Supremacy (Static Loop Advantage)')
    
    ax.set_xlim(8, 256)
    ax.set_ylim(0, 15)
    
    ax.set_xlabel('Entity Size in Bytes (Data Payload)', fontsize=12, fontweight='bold')
    ax.set_ylabel('Structural Mutation Rate (%)', fontsize=12, fontweight='bold')
    ax.set_title('ECS vs CRG Break-Even Analysis', fontsize=14, pad=15, fontweight='bold')
    
    ax.axvline(x=64, color='gray', linestyle='--', alpha=0.5)
    ax.text(66, 1, 'Standard 64B\n(Cache Line)', color='gray')
    
    ax.legend(loc='upper right', fontsize=11)
    ax.grid(True, linestyle=':', alpha=0.7)
    
    output_dir = "img"
    os.makedirs(output_dir, exist_ok=True)
    out_path = os.path.join(output_dir, "breakeven_curve.png")
    
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    print(f"✅ Image generated: {out_path}")

if __name__ == '__main__':
    generate_breakeven_plot()
