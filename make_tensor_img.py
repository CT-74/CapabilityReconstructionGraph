import os
import matplotlib.pyplot as plt
import numpy as np

def generate_capability_tensor():
    # 1. Figure moins large, format plus équilibré (9.5 au lieu de 11)
    fig = plt.figure(figsize=(9.5, 8.5))
    ax = fig.add_subplot(111, projection='3d')

    models = [0, 1, 2] # Scout, Heavy, Drone
    wheres = [0, 1, 2] # Ground, Desert, Snow
    whens = [0, 1]     # Day, Night

    target = (1, 1, 1)
    
    # Palette haute visibilité
    color_dot = '#e41a1c'  # Rouge vif
    color_x = '#ff7f0e'    # Orange (Model)
    color_y = '#2ca02c'    # Vert (Where)
    color_z = '#9467bd'    # Violet (When)

    # Remplissage du tenseur
    for m in models:
        for w in wheres:
            for t in whens:
                is_target = (m == target[0] and w == target[1] and t == target[2])
                
                color = color_dot if is_target else '#e0e0e0'
                size = 200 if is_target else 60
                alpha = 1.0 if is_target else 0.8
                edge = 'white' if is_target else 'none'
                
                ax.scatter(m, w, t, c=color, s=size, alpha=alpha, edgecolors=edge, linewidth=2, zorder=5)

                if is_target:
                    ax.plot([m, m], [w, w], [0, t], color=color_z, linestyle='--', linewidth=2.5, zorder=4)
                    ax.plot([m, m], [0, w], [t, t], color=color_x, linestyle='--', linewidth=2.5, zorder=4)
                    ax.plot([0, m], [w, w], [t, t], color=color_y, linestyle='--', linewidth=2.5, zorder=4)
                    
                    # Le Tooltip multi-couleur empilé
                    ax.text(m, w, t + 0.23, "   Heavy", color=color_x, fontsize=13, fontweight='bold', zorder=6)
                    ax.text(m, w, t + 0.14, "   Desert", color=color_y, fontsize=13, fontweight='bold', zorder=6)
                    ax.text(m, w, t + 0.05, "   Night", color=color_z, fontsize=13, fontweight='bold', zorder=6)

    # Configuration des labels avec retour à la ligne (\n)
    ax.set_xticks(models)
    x_labels = ax.set_xticklabels(['Scout', 'Heavy', 'Drone'], fontsize=12)
    x_labels[target[0]].set_color(color_x)
    x_labels[target[0]].set_fontweight('bold')
    ax.set_xlabel('\nModel', fontsize=14, color=color_x, fontweight='bold')

    ax.set_yticks(wheres)
    y_labels = ax.set_yticklabels(['Ground', 'Desert', 'Snow'], fontsize=12)
    y_labels[target[1]].set_color(color_y)
    y_labels[target[1]].set_fontweight('bold')
    ax.set_ylabel('\nWhere', fontsize=14, color=color_y, fontweight='bold')

    ax.set_zticks(whens)
    z_labels = ax.set_zticklabels(['Day', 'Night'], fontsize=12)
    z_labels[target[2]].set_color(color_z)
    z_labels[target[2]].set_fontweight('bold')
    ax.set_zlabel('\nWhen', fontsize=14, color=color_z, fontweight='bold')

    # Titre et Légende (Le titre redescend un peu naturellement avec le nouveau 'top')
    ax.set_title("N-Dimensional Capability Tensor Resolution", fontsize=15, pad=20)
    ax.plot([], [], 'o', color=color_dot, markersize=14, label='Resolved Capability (O(1))')
    ax.legend(loc='upper right', fontsize=12, framealpha=1.0)

    # Nettoyage visuel
    ax.xaxis.pane.fill = False
    ax.yaxis.pane.fill = False
    ax.zaxis.pane.fill = False
    ax.xaxis.pane.set_edgecolor('lightgray')
    ax.yaxis.pane.set_edgecolor('lightgray')
    ax.zaxis.pane.set_edgecolor('lightgray')
    ax.grid(color='lightgray', linestyle='-', linewidth=0.5)

    ax.view_init(elev=25, azim=-55)

    # 2. Marge du haut abaissée (top=0.88) pour sauver le titre, 
    # et marge droite toujours à 0.85 pour sauver le "When"
    fig.subplots_adjust(left=0.05, right=0.85, top=0.88, bottom=0.05)

    output_dir = "img"
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "tensor_routing.png")
    
    plt.savefig(output_path, dpi=300, transparent=False, facecolor='white')
    print(f"✅ Image UX (Centrée et non-rognée) regénérée avec succès : {output_path}")

if __name__ == '__main__':
    generate_capability_tensor()