import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import csv
import os

# Formateur pour transformer 1000000 en "1M" et 1000 en "1K"
def format_km(x, pos):
    if x >= 1e6: return f'{x*1e-6:g}M'
    if x >= 1e3: return f'{x*1e-3:g}K'
    return f'{x:g}'

file_path = 'crg_benchmark_final.csv'
data = {}
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
    # ECS (Pointillés)
    plt.semilogx(v['n'], v['ecs'], '--', color=colors[rate], alpha=0.4, label=f'ECS (Mut {pct})')
    # CRG (Plein)
    plt.semilogx(v['n'], v['crg'], 'o-', color=colors[rate], linewidth=2.5, label=f'CRG (Mut {pct})')

# Application du formatage K / M
plt.gca().xaxis.set_major_formatter(ticker.FuncFormatter(format_km))

# Styling final
plt.title('CRG Architecture: Structural Immunity Analysis', fontsize=16, fontweight='bold', pad=20)
plt.xlabel('Number of Entities (Data Set Size)', fontsize=12)
plt.ylabel('Cost per Entity (nanoseconds)', fontsize=12)
plt.legend(loc='upper left', ncol=2, frameon=True, shadow=True)
plt.grid(True, which="both", ls="-", alpha=0.1)

# Zone de danger RAM
plt.axvspan(16e6/64, max(v['n']), color='red', alpha=0.05, label='RAM Bound Area')

plt.tight_layout()
plt.savefig('crg_benchmark_final.png', dpi=300)
print("Graphique mis à jour avec labels K/M.")