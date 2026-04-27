"""
PURPOSE:
A simplified plotting script that reads 'results.csv' to generate the primary 
presentation slide for CppCon. It creates a bold, easy-to-read bar chart 
comparing Classic ECS vs Hybrid CRG total execution times.
"""
import matplotlib.pyplot as plt
import csv
from datetime import datetime

entities = []
ecs_times = []
crg_times = []

with open('results.csv', 'r') as file:
    reader = csv.reader(file)
    for row in reader:
        entities.append(int(row[0]))
        ecs_times.append(float(row[1]))
        crg_times.append(float(row[2]))

plt.figure(figsize=(10, 6))
plt.bar(['Classic ECS\n(Archetype Poisoning)', 'Hybrid CRG\n(Stage 10)'], 
        [ecs_times[0], crg_times[0]], 
        color=['#e74c3c', '#2ecc71'])

plt.ylabel('Temps d\'exécution (ms) - Plus bas = Meilleur')
plt.title(f'Performance de Mutation d\'État ({entities[0]:,} Entités)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

# TIMESTAMP ADDITION
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
output_filename = f'cppcon_benchmark_slide_{timestamp}.png'

plt.savefig(output_filename, dpi=300, bbox_inches='tight')
print(f"Graphique généré : {output_filename}")