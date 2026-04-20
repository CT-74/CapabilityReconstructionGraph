import matplotlib.pyplot as plt
import csv

# On lit les données générées par le programme C++
entities = []
ecs_times = []
crg_times = []

with open('results.csv', 'r') as file:
    reader = csv.reader(file)
    for row in reader:
        entities.append(int(row[0]))
        ecs_times.append(float(row[1]))
        crg_times.append(float(row[2]))

# Création du graphique
plt.figure(figsize=(10, 6))
plt.bar(['Classic ECS\n(Archetype Poisoning)', 'Hybrid CRG\n(Stage 10)'], 
        [ecs_times[0], crg_times[0]], 
        color=['#e74c3c', '#2ecc71'])

plt.ylabel('Temps d\'exécution (ms) - Plus bas = Meilleur')
plt.title(f'Performance de Mutation d\'État ({entities[0]:,} Entités)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

# On sauvegarde la fameuse image !
plt.savefig('cppcon_benchmark_slide.png', dpi=300, bbox_inches='tight')
print("Graphique généré : cppcon_benchmark_slide.png")