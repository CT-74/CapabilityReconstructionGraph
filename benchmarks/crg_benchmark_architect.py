"""
PURPOSE:
This script reads 'crg_benchmark_architect.csv' and generates a line chart (semi-log X axis).
It visualizes the cost per entity as the dataset grows, specifically highlighting 
where the hardware cache limits (L1 and L3 memory walls) are hit.
"""
import matplotlib.pyplot as plt
import csv
import os
from datetime import datetime

file_path = 'crg_benchmark_architect.csv'

if not os.path.exists(file_path):
    print(f"Erreur : {file_path} non trouvé. Lance d'abord le binaire C++.")
    exit(1)

n, ecs, crg = [], [], []
with open(file_path, 'r') as f:
    next(f) # Skip header
    for row in csv.reader(f):
        n.append(int(row[0]))
        ecs.append(float(row[1]))
        crg.append(float(row[2]))

plt.figure(figsize=(12, 7))

# On utilise une échelle logarithmique pour l'axe X (Set de données)
plt.semilogx(n, ecs, 'o-', label='Classic ECS (Idealized Loop)', color='#34495e', alpha=0.6, linestyle='--')
plt.semilogx(n, crg, 'o-', label='CRG Stage 10 (Matrix Resolution)', color='#3498db', linewidth=2.5)

# --- ANALYSE DU CACHING ---
plt.axvline(x=32000/64, color='gray', linestyle=':', alpha=0.5) 
plt.text(32000/64, plt.ylim()[1]*0.9, ' L1 Cache', color='gray', fontsize=9)

l3_limit_entities = 16000000 / 64
plt.axvline(x=l3_limit_entities, color='#e74c3c', linestyle='--', alpha=0.4) 
plt.text(l3_limit_entities, plt.ylim()[1]*0.05, ' Memory Wall (L3 Limit)', color='#e74c3c', rotation=90, va='bottom')

plt.xlabel('Number of Entities (Data Set Size - Log Scale)')
plt.ylabel('Cost per Entity (nanoseconds)')
plt.title('Performance Stability & Cache Locality Analysis (Stage 10)')
plt.legend()
plt.grid(True, which="both", ls="-", alpha=0.1)

# TIMESTAMP ADDITION
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
output_filename = f'crg_benchmark_architect_{timestamp}.png'

plt.savefig(output_filename, dpi=300, bbox_inches='tight')
print(f"Graphique sauvegardé : {output_filename}")