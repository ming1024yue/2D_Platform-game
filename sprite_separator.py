#!/usr/bin/env python3
"""
Sprite Sheet Separator for Finn Character
Separates sprite frames based on background colors:
- Blue: idle
- Red: walking
- Purple: jump
- Orange: get hit
- Green: die
- Yellow: attack
"""

import cv2
import numpy as np
import os
from PIL import Image
import argparse

def get_dominant_background_color(image_section):
    """
    Get the dominant background color from an image section.
    Assumes background is the most common color around the edges.
    """
    # Get edge pixels (top, bottom, left, right borders)
    h, w = image_section.shape[:2]
    edge_pixels = []
    
    # Top and bottom edges
    edge_pixels.extend(image_section[0, :].reshape(-1, 3))
    edge_pixels.extend(image_section[h-1, :].reshape(-1, 3))
    
    # Left and right edges
    edge_pixels.extend(image_section[:, 0].reshape(-1, 3))
    edge_pixels.extend(image_section[:, w-1].reshape(-1, 3))
    
    edge_pixels = np.array(edge_pixels)
    
    # Find the most common color
    unique_colors, counts = np.unique(edge_pixels.reshape(-1, 3), axis=0, return_counts=True)
    dominant_color = unique_colors[np.argmax(counts)]
    
    return dominant_color

def classify_movement_by_color(bgr_color):
    """
    Classify movement type based on background color (BGR format).
    """
    # Convert BGR to RGB for easier understanding
    b, g, r = bgr_color
    
    # Define color ranges (allowing for some tolerance)
    color_mappings = {
        'idle': ([100, 0, 0], [255, 100, 100]),      # Blue range
        'walking': ([0, 0, 100], [100, 100, 255]),   # Red range  
        'jump': ([100, 0, 100], [255, 100, 255]),    # Purple range
        'get_hit': ([0, 100, 100], [100, 255, 255]), # Orange range
        'die': ([0, 100, 0], [100, 255, 100]),       # Green range
        'attack': ([0, 200, 200], [100, 255, 255])   # Yellow range
    }
    
    for movement, (min_color, max_color) in color_mappings.items():
        if (min_color[0] <= b <= max_color[0] and 
            min_color[1] <= g <= max_color[1] and 
            min_color[2] <= r <= max_color[2]):
            return movement
    
    # If no match, try a simpler approach based on dominant channel
    if b > g and b > r:
        return 'idle'  # Blue dominant
    elif r > g and r > b:
        return 'walking'  # Red dominant
    elif r > 150 and b > 150 and g < 100:
        return 'jump'  # Purple (red + blue)
    elif r > 150 and g > 150 and b < 100:
        return 'attack'  # Yellow (red + green)
    elif r > 150 and g > 100 and b < 150:
        return 'get_hit'  # Orange (red + some green)
    elif g > r and g > b:
        return 'die'  # Green dominant
    
    return 'unknown'

def separate_sprite_sheet(sprite_path, output_dir, grid_cols=None, grid_rows=None):
    """
    Separate sprite sheet into individual movements based on background colors.
    """
    # Load the sprite sheet
    sprite_sheet = cv2.imread(sprite_path)
    if sprite_sheet is None:
        raise ValueError(f"Could not load sprite sheet from {sprite_path}")
    
    height, width = sprite_sheet.shape[:2]
    
    # If grid dimensions not provided, try to auto-detect
    if grid_cols is None or grid_rows is None:
        # This is a simple heuristic - you might need to adjust based on your sprite sheet
        # For now, let's assume a common grid size
        grid_cols = 8  # Common sprite sheet width
        grid_rows = 6  # Common sprite sheet height
    
    # Calculate frame dimensions
    frame_width = width // grid_cols
    frame_height = height // grid_rows
    
    # Create output directory structure
    movements = {}
    
    print(f"Processing sprite sheet: {width}x{height}")
    print(f"Grid: {grid_cols}x{grid_rows}, Frame size: {frame_width}x{frame_height}")
    
    # Process each frame
    for row in range(grid_rows):
        for col in range(grid_cols):
            # Extract frame
            x = col * frame_width
            y = row * frame_height
            frame = sprite_sheet[y:y+frame_height, x:x+frame_width]
            
            # Skip empty frames
            if frame.size == 0:
                continue
            
            # Get background color and classify movement
            bg_color = get_dominant_background_color(frame)
            movement = classify_movement_by_color(bg_color)
            
            print(f"Frame ({row},{col}): Background color {bg_color} -> {movement}")
            
            # Store frame in appropriate movement category
            if movement not in movements:
                movements[movement] = []
            movements[movement].append((frame, f"frame_{row}_{col}"))
    
    # Save separated sprites
    os.makedirs(output_dir, exist_ok=True)
    
    for movement, frames in movements.items():
        if not frames:
            continue
            
        movement_dir = os.path.join(output_dir, movement)
        os.makedirs(movement_dir, exist_ok=True)
        
        print(f"\nSaving {len(frames)} frames for {movement}")
        
        for i, (frame, frame_name) in enumerate(frames):
            # Save individual frame
            frame_path = os.path.join(movement_dir, f"{frame_name}.png")
            cv2.imwrite(frame_path, frame)
            
        # Also create a combined sprite sheet for each movement
        if len(frames) > 1:
            # Arrange frames in a horizontal strip
            combined_width = sum(frame.shape[1] for frame, _ in frames)
            combined_height = max(frame.shape[0] for frame, _ in frames)
            combined_sheet = np.zeros((combined_height, combined_width, 3), dtype=np.uint8)
            
            x_offset = 0
            for frame, _ in frames:
                frame_width = frame.shape[1]
                combined_sheet[:frame.shape[0], x_offset:x_offset+frame_width] = frame
                x_offset += frame_width
            
            combined_path = os.path.join(movement_dir, f"{movement}_combined.png")
            cv2.imwrite(combined_path, combined_sheet)
            print(f"Created combined sheet: {combined_path}")

def main():
    parser = argparse.ArgumentParser(description='Separate Finn sprite sheet by movement')
    parser.add_argument('--sprite', default='assets/images/characters/FinnSprite.png', 
                       help='Path to sprite sheet')
    parser.add_argument('--output', default='assets/images/characters/separated', 
                       help='Output directory')
    parser.add_argument('--cols', type=int, help='Number of columns in sprite grid')
    parser.add_argument('--rows', type=int, help='Number of rows in sprite grid')
    
    args = parser.parse_args()
    
    try:
        separate_sprite_sheet(args.sprite, args.output, args.cols, args.rows)
        print(f"\nSprite separation complete! Check {args.output} for results.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main() 