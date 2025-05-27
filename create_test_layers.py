#!/usr/bin/env python3
"""
Simple script to create colored test layers for the background system.
This creates basic colored rectangles that can be used to test the parallax effect.
"""

from PIL import Image, ImageDraw
import os

def create_layer(width, height, color, filename, text=None):
    """Create a simple colored layer with optional text."""
    # Create image with RGBA mode for transparency support
    img = Image.new('RGBA', (width, height), color)
    
    if text:
        draw = ImageDraw.Draw(img)
        # Try to use a default font, fallback to basic if not available
        try:
            from PIL import ImageFont
            font = ImageFont.truetype("/System/Library/Fonts/Arial.ttf", 48)
        except:
            font = ImageDraw.get_font()
        
        # Calculate text position (centered)
        bbox = draw.textbbox((0, 0), text, font=font)
        text_width = bbox[2] - bbox[0]
        text_height = bbox[3] - bbox[1]
        x = (width - text_width) // 2
        y = (height - text_height) // 2
        
        # Draw text with outline for visibility
        outline_color = (0, 0, 0, 255) if sum(color[:3]) > 384 else (255, 255, 255, 255)
        for dx in [-2, -1, 0, 1, 2]:
            for dy in [-2, -1, 0, 1, 2]:
                if dx != 0 or dy != 0:
                    draw.text((x + dx, y + dy), text, font=font, fill=outline_color)
        draw.text((x, y), text, font=font, fill=(255, 255, 255, 255))
    
    img.save(filename)
    print(f"Created {filename}")

def main():
    # Create output directory
    output_dir = "assets/images/backgrounds"
    os.makedirs(output_dir, exist_ok=True)
    
    # Standard dimensions
    width, height = 800, 600
    
    # Create basic test layers
    print("Creating basic test layers...")
    
    # Background1 layer - light blue gradient effect (sky-like)
    create_layer(width, height, (135, 206, 235, 255), 
                os.path.join(output_dir, "background1.png"), "BACKGROUND1")
    
    # Background2 layer - semi-transparent white (clouds-like)
    create_layer(width, height, (255, 255, 255, 128), 
                os.path.join(output_dir, "background2.png"), "BACKGROUND2")
    
    # Background3 layer - dark gray/purple (mountains-like)
    create_layer(width, height, (75, 75, 100, 255), 
                os.path.join(output_dir, "background3.png"), "BACKGROUND3")
    
    # Background4 layer - green (ground-like)
    create_layer(width, height, (34, 139, 34, 255), 
                os.path.join(output_dir, "background4.png"), "BACKGROUND4")
    
    # Create theme-specific layers
    themes = {
        "snow": {
            "background1": (200, 220, 255, 255),      # Cool blue sky
            "background2": (240, 240, 255, 150),      # Cool white clouds
            "background3": (150, 150, 180, 255),      # Blue-gray mountains
            "background4": (240, 240, 255, 255)       # White ground
        },
        "snow_forest": {
            "background1": (255, 255, 255, 255),      # Pure white sky for snow forest
            "background2": (220, 230, 245, 140),      # Softer clouds
            "background3": (120, 140, 160, 255),      # Darker blue-gray mountains with forest
            "background4": (220, 230, 245, 255)       # Slightly warmer white ground
        }
    }
    
    for theme_name, colors in themes.items():
        theme_dir = os.path.join(output_dir, theme_name)
        os.makedirs(theme_dir, exist_ok=True)
        print(f"\nCreating {theme_name} theme layers...")
        
        for layer_name, color in colors.items():
            filename = os.path.join(theme_dir, f"{layer_name}.png")
            text = f"{theme_name.upper()}\n{layer_name.upper()}"
            create_layer(width, height, color, filename, text)
    
    print("\nAll test layers created!")
    print("You can now run the game to see the layered background system in action.")
    print("Use the ImGui interface (F1) to see layer status and controls.")

if __name__ == "__main__":
    main() 