"""
PURPOSE:
Visualizes the N-Dimensional Capability Tensor in 3D.
Highlights the O(1) coordinate-based resolution with color-coded labels.
"""
import os
import matplotlib.pyplot as plt
import numpy as np

# --- PATH CONFIGURATION ---
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
IMG_DIR = os.path.join(CURRENT_DIR, "..", "img")

def generate_capability_tensor():
    fig = plt.figure(figsize=(9.5, 8.5))
    ax = fig.add_subplot(111, projection='3d')

    models = [0, 1, 2] # Scout, Heavy, Drone
    wheres = [0, 1, 2] # Ground, Desert, Snow
    whens = [0, 1]     # Day, Night

    target = (1, 1, 1) # Target coordinates for visualization
    
    # High visibility color palette
    color_dot = '#e41a1c'  # Target dot
    color_x = '#ff7f0e'    # Model Axis (Orange)
    color_y = '#2ca02c'    # Where Axis (Green)
    color_z = '#9467bd'    # When Axis (Purple)

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
                    # Drawing projection lines
                    ax.plot([m, m], [w, w], [0, t], color=color_z, linestyle='--', linewidth=2.5, zorder=4)
                    ax.plot([m, m], [0, w], [t, t], color=color_x, linestyle='--', linewidth=2.5, zorder=4)
                    ax.plot([0, m], [w, w], [t, t], color=color_y, linestyle='--', linewidth=2.5, zorder=4)
                    
                    # Multi-colored labels for the coordinate
                    ax.text(m, w, t + 0.23, "   Heavy", color=color_x, fontsize=13, fontweight='bold', zorder=6)
                    ax.text(m, w, t + 0.14, "   Desert", color=color_y, fontsize=13, fontweight='bold', zorder=6)
                    ax.text(m, w, t + 0.05, "   Night", color=color_z, fontsize=13, fontweight='bold', zorder=6)

    # Label configuration - X Axis (Model)
    ax.set_xticks(models)
    x_labels = ax.set_xticklabels(['Scout', 'Heavy', 'Drone'], fontsize=12)
    for i, label in enumerate(x_labels):
        label.set_color(color_x)
        if i == target[0]:
            label.set_fontweight('bold')
    ax.set_xlabel('\nModel', fontsize=14, color=color_x, fontweight='bold')

    # Label configuration - Y Axis (Where)
    ax.set_yticks(wheres)
    y_labels = ax.set_yticklabels(['Ground', 'Desert', 'Snow'], fontsize=12)
    for i, label in enumerate(y_labels):
        label.set_color(color_y)
        if i == target[1]:
            label.set_fontweight('bold')
    ax.set_ylabel('\nWhere', fontsize=14, color=color_y, fontweight='bold')

    # Label configuration - Z Axis (When)
    ax.set_zticks(whens)
    z_labels = ax.set_zticklabels(['Day', 'Night'], fontsize=12)
    for i, label in enumerate(z_labels):
        label.set_color(color_z)
        if i == target[2]:
            label.set_fontweight('bold')
    ax.set_zlabel('\nWhen', fontsize=14, color=color_z, fontweight='bold')

    ax.set_title("N-Dimensional Capability Tensor Resolution", fontsize=15, pad=20)
    ax.view_init(elev=25, azim=-55)

    os.makedirs(IMG_DIR, exist_ok=True)
    output_path = os.path.join(IMG_DIR, "tensor_routing.png")
    
    plt.savefig(output_path, dpi=300, transparent=False, facecolor='white')
    print(f"✅ Image updated: {output_path}")

if __name__ == '__main__':
    generate_capability_tensor()