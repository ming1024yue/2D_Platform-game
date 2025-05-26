#!/usr/bin/env python3
"""
Sprite Sheet Slicer
Slices a 5x5 sprite sheet into 25 individual tiles
"""

import os
from PIL import Image
import argparse

def slice_sprite_sheet(input_path, output_dir, rows=5, cols=5):
    """
    Slice a sprite sheet into individual tiles
    
    Args:
        input_path (str): Path to the input sprite sheet
        output_dir (str): Directory to save individual tiles
        rows (int): Number of rows in the sprite sheet
        cols (int): Number of columns in the sprite sheet
    """
    
    # Open the sprite sheet
    try:
        sprite_sheet = Image.open(input_path)
        print(f"Loaded sprite sheet: {input_path}")
        print(f"Image size: {sprite_sheet.size}")
    except Exception as e:
        print(f"Error loading image: {e}")
        return False
    
    # Calculate tile dimensions
    sheet_width, sheet_height = sprite_sheet.size
    tile_width = sheet_width // cols
    tile_height = sheet_height // rows
    
    print(f"Tile size: {tile_width}x{tile_height}")
    
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Extract each tile
    tile_count = 0
    for row in range(rows):
        for col in range(cols):
            # Calculate the bounding box for this tile
            left = col * tile_width
            top = row * tile_height
            right = left + tile_width
            bottom = top + tile_height
            
            # Extract the tile
            tile = sprite_sheet.crop((left, top, right, bottom))
            
            # Save the tile
            tile_filename = f"tile_{row:02d}_{col:02d}.png"
            tile_path = os.path.join(output_dir, tile_filename)
            tile.save(tile_path)
            
            tile_count += 1
            print(f"Saved tile {tile_count}/25: {tile_filename}")
    
    print(f"\nSuccessfully sliced sprite sheet into {tile_count} tiles!")
    print(f"Tiles saved to: {output_dir}")
    return True

def main():
    parser = argparse.ArgumentParser(description="Slice a sprite sheet into individual tiles")
    parser.add_argument("--input", "-i", 
                       default="assets/images/platformer/IceSet.png",
                       help="Path to input sprite sheet (default: assets/images/platformer/IceSet.png)")
    parser.add_argument("--output", "-o",
                       default="assets/images/platformer/tiles",
                       help="Output directory for tiles (default: assets/images/platformer/tiles)")
    parser.add_argument("--rows", "-r", type=int, default=5,
                       help="Number of rows in sprite sheet (default: 5)")
    parser.add_argument("--cols", "-c", type=int, default=5,
                       help="Number of columns in sprite sheet (default: 5)")
    
    args = parser.parse_args()
    
    # Check if input file exists
    if not os.path.exists(args.input):
        print(f"Error: Input file '{args.input}' not found!")
        return 1
    
    # Slice the sprite sheet
    success = slice_sprite_sheet(args.input, args.output, args.rows, args.cols)
    
    return 0 if success else 1

if __name__ == "__main__":
    exit(main()) 