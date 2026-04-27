"""
PURPOSE:
This script plots multiple lines representing different mutation rates (1%, 5%, 10%) 
from 'crg_benchmark_final.csv'. It proves that CRG maintains "Structural Immunity" 
regardless of how often entities change state, while classic ECS degrades rapidly.
"""
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import csv
import os
from datetime import datetime

# Formatter to transform 1000000 into "1M" and 1000 into "1K"
def format_km(x, pos):
    if x >= 1e6: return f'{x*1e-6:g}M'
    if x >= 1e3: return f'{x*1e-3:g}K'
    return f'{x:g}'

file_path = 'crg_benchmark_final.csv'
data = {}

if not os.path.exists(file_path):
    print(f"❌ Error: {file_path} not found. Ensure the C++ binary generated it.")
    exit(1)

with open(file_path, 'r') as f:
    reader = csv.reader(f)
    next(reader)
    for row in reader:
        n, rate, ecs, crg = int(row[0]), float(row[1]), float(row[2]), float(row[3])
        if rate not in data: data[rate] = {'n': [], 'ecs': [], 'crg': []}
        data[rate]['n'].append(n)
        data[rate]['ecs'].append(ecs)
        data[rate]['crg'].append(crg)

plt.figure(figsize=(12, 8))
colors = {0.01: '#3498db', 0.05: '#f1c40f', 0.10: '#e74c3c'}

for rate, v in data.items():
    pct = f"{int(rate*100)}%"
    # ECS (Dashed line)
    plt.semilogx(v['n'], v['ecs'], '--', color=colors[rate], alpha=0.4, label=f'ECS (Mut {pct})')
    # CRG (Solid line)
    plt.semilogx(v['n'], v['crg'], 'o-', color=colors[rate], linewidth=2.5, label=f'CRG (Mut {pct})')

# Apply K / M formatting
plt.gca().xaxis.set_major_formatter(ticker.FuncFormatter(format_km))

# Final styling
plt.title('CRG Architecture: Structural Immunity Analysis', fontsize=16, fontweight='bold', pad=20)
plt.xlabel('Number of Entities (Data Set Size)', fontsize=12)
plt.ylabel('Cost per Entity (nanoseconds)', fontsize=12)
plt.legend(loc='upper left', ncol=2, frameon=True, shadow=True)
plt.grid(True, which="both", ls="-", alpha=0.1)

# RAM danger zone annotation
plt.axvspan(16e6/64, max(v['n']), color='red', alpha=0.05, label='RAM Bound Area')

plt.tight_layout()

# TIMESTAMP ADDITION
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
output_filename = f'crg_benchmark_final_{timestamp}.png'

plt.savefig(output_filename, dpi=300)
print(f"✅ Graph successfully generated: {output_filename}")