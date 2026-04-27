"""
PURPOSE:
This script reads the last line of 'crg_benchmark_bar.csv' to create a direct 
bar chart comparison. It visually demonstrates the massive penalty of 
"Archetype Poisoning" (Structural Mutation in classic ECS) vs O(1) Matrix Resolution (CRG).
"""
import matplotlib.pyplot as plt
import csv
import os
from datetime import datetime

file_path = 'crg_benchmark_bar.csv'
MUTATION_RATE = "5%" # On le définit en constante pour le titre

if not os.path.exists(file_path):
    print(f"Erreur : {file_path} non trouvé.")
    exit(1)

with open(file_path, 'r') as f:
    data = list(csv.reader(f))[-1]
    ecs_val, crg_val = float(data[1]), float(data[2])

plt.figure(figsize=(10, 7))
colors = ['#e74c3c', '#2ecc71']
labels = ['ECS Archetype\n(Structural Mutation)', 'CRG Stage 10\n(Value Mutation)']

bars = plt.bar(labels, [ecs_val, crg_val], color=colors, edgecolor='black', alpha=0.8)

plt.title(f'State Transition Impact ({MUTATION_RATE} Mutation Rate per Frame)', 
          fontsize=16, fontweight='bold', pad=20)
plt.suptitle('1,000,000 Entities - 64 bytes per row', fontsize=10, y=0.92)

plt.ylabel('Total Execution Time (ms)', fontsize=12)
plt.grid(axis='y', linestyle='--', alpha=0.3)

for bar in bars:
    height = bar.get_height()
    plt.text(bar.get_x() + bar.get_width()/2., height + 0.5,
             f'{height:.2f} ms', ha='center', va='bottom', 
             fontsize=12, fontweight='bold')

plt.annotate('Swap & Pop + Sparse Set Update\n= Cache Trashing', 
             xy=(0, ecs_val), xytext=(0.3, ecs_val + 5),
             arrowprops=dict(facecolor='black', shrink=0.05, width=1, headwidth=8))

plt.annotate('O(1) Matrix Resolution\n= Cache Friendly', 
             xy=(1, crg_val), xytext=(0.5, crg_val + 10),
             arrowprops=dict(facecolor='black', shrink=0.05, width=1, headwidth=8))

plt.tight_layout()

# TIMESTAMP ADDITION
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
output_filename = f'crg_benchmark_bar_{timestamp}.png'

plt.savefig(output_filename, dpi=300)
print(f"PNG mis à jour avec les détails de mutation : {output_filename}")