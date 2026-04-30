import os
import matplotlib.pyplot as plt
import matplotlib.patches as patches

def generate_tensor_routing():
    fig, ax = plt.subplots(figsize=(14, 6), facecolor='#121212')
    ax.set_xlim(0, 14)
    ax.set_ylim(0, 6)
    ax.axis('off')

    # Colors
    text_color = '#e0e0e0'
    box_bg = '#252525'
    box_edge = '#444444'
    highlight = '#2ecc71'
    arrow_color = '#4da6ff'

    # 1. N-Dimensional Space Box
    ax.add_patch(patches.FancyBboxPatch((0.5, 1.5), 3.5, 3, boxstyle="round,pad=0.2", fc=box_bg, ec=box_edge, lw=2))
    ax.text(2.25, 4.8, "Capability Space\n(N-Context Axes)", color=text_color, fontsize=14, ha='center', va='center', fontweight='bold')
    
    # Dimensions
    ax.text(1.0, 3.8, "X: Entity State (3)", color='#aaaaaa', fontsize=12)
    ax.text(1.0, 3.2, "Y: World Zone (4)", color='#aaaaaa', fontsize=12)
    ax.text(1.0, 2.6, "Z: AI Authority (2)", color='#aaaaaa', fontsize=12)
    
    ax.text(2.25, 1.8, "Coordinates:\n[Combat, Desert, Local]", color='#ffcc00', fontsize=11, ha='center', va='center', bbox=dict(facecolor='#332200', edgecolor='#ffcc00', boxstyle='round,pad=0.3'))

    # 2. Horner's Method Arrow
    ax.annotate('', xy=(5.5, 3), xytext=(4.2, 3), arrowprops=dict(arrowstyle="->", color=arrow_color, lw=3))
    ax.text(4.85, 3.4, "Horner's Method", color=arrow_color, fontsize=12, ha='center', va='center', fontweight='bold')
    ax.text(4.85, 2.8, "O(1) Flattening", color='#aaaaaa', fontsize=10, ha='center', va='center')

    # 3. DenseTypeID (Rows)
    ax.text(6.8, 4.8, "DenseTypeID\n(Model Index)", color=text_color, fontsize=14, ha='center', va='center', fontweight='bold')
    ax.text(6.8, 3.8, "Row 0: Scout", color='#aaaaaa', fontsize=12, ha='center')
    ax.text(6.8, 3.2, "Row 1: Drone", color='#ffcc00', fontsize=12, ha='center', fontweight='bold')
    ax.text(6.8, 2.6, "Row 2: Heavy", color='#aaaaaa', fontsize=12, ha='center')
    
    ax.annotate('', xy=(8.2, 3.2), xytext=(7.6, 3.2), arrowprops=dict(arrowstyle="->", color=arrow_color, lw=3))

    # 4. The Tensor (Contiguous RAM)
    ax.add_patch(patches.FancyBboxPatch((8.5, 1.5), 5, 3, boxstyle="round,pad=0.2", fc=box_bg, ec=box_edge, lw=2))
    ax.text(11, 4.8, "Capability Tensor\n(Contiguous RAM Matrix)", color=text_color, fontsize=14, ha='center', va='center', fontweight='bold')

    # Draw Grid
    grid_x, grid_y = 9.2, 2.0
    cell_w, cell_h = 0.6, 0.6
    for r in range(3):
        for c in range(6):
            x = grid_x + c * cell_w
            y = grid_y + (2-r) * cell_h
            
            # Highlight Drone (Row 1) + Offset (Col 3)
            is_target = (r == 1 and c == 3)
            fc = highlight if is_target else '#1a1a1a'
            ec = '#ffffff' if is_target else '#333333'
            
            ax.add_patch(patches.Rectangle((x, y), cell_w, cell_h, fc=fc, ec=ec, lw=2 if is_target else 1))
            
            if is_target:
                ax.text(x + cell_w/2, y + cell_h/2, "Ptr", color='black', fontsize=10, ha='center', va='center', fontweight='bold')

    ax.text(11, 1.0, "Zero-Cost DOD Descriptor Fetch", color=highlight, fontsize=12, ha='center', va='center', fontweight='bold')

    plt.tight_layout()
    os.makedirs("img", exist_ok=True)
    out_path = "img/tensor_routing_dark.png"
    plt.savefig(out_path, dpi=300, facecolor=fig.get_facecolor(), edgecolor='none')
    print(f"✅ Image generated: {out_path}")

if __name__ == '__main__':
    generate_tensor_routing()