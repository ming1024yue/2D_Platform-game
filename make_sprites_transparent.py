#!/usr/bin/env python3
"""
Make all character sprites have transparent backgrounds
Converts black backgrounds to transparent
"""

import cv2
import numpy as np
import os
from PIL import Image

def make_transparent(image_path, output_path=None):
    """
    Convert black background to transparent
    """
    if output_path is None:
        output_path = image_path
    
    # Load image with PIL to handle transparency better
    img = Image.open(image_path).convert("RGBA")
    data = np.array(img)
    
    # Get the RGB channels
    rgb = data[:, :, :3]
    alpha = data[:, :, 3]
    
    # Create mask for black pixels (or very dark pixels)
    # Black background is typically (0,0,0) but we'll be a bit tolerant
    black_mask = (rgb[:, :, 0] < 10) & (rgb[:, :, 1] < 10) & (rgb[:, :, 2] < 10)
    
    # Set alpha to 0 (transparent) for black pixels
    alpha[black_mask] = 0
    
    # Update the alpha channel
    data[:, :, 3] = alpha
    
    # Save the image
    result_img = Image.fromarray(data, 'RGBA')
    result_img.save(output_path, 'PNG')
    
    return black_mask.sum()  # Return number of pixels made transparent

def process_directory(directory_path):
    """
    Process all PNG files in a directory
    """
    print(f"Processing directory: {directory_path}")
    
    if not os.path.exists(directory_path):
        print(f"Directory not found: {directory_path}")
        return
    
    png_files = [f for f in os.listdir(directory_path) if f.endswith('.png')]
    
    for filename in png_files:
        file_path = os.path.join(directory_path, filename)
        print(f"  Processing: {filename}")
        
        try:
            transparent_pixels = make_transparent(file_path)
            print(f"    Made {transparent_pixels} pixels transparent")
        except Exception as e:
            print(f"    Error processing {filename}: {e}")

def main():
    # Process all sprite directories
    base_path = "assets/images/characters/separated_finn"
    
    directories = [
        "idle",
        "walking", 
        "jump",
        "attack",
        "get_hit",
        "die"
    ]
    
    for directory in directories:
        dir_path = os.path.join(base_path, directory)
        process_directory(dir_path)
    
    # Also process the main player.png
    print("\nProcessing main player.png...")
    try:
        transparent_pixels = make_transparent("assets/images/characters/player.png")
        print(f"Made {transparent_pixels} pixels transparent in player.png")
    except Exception as e:
        print(f"Error processing player.png: {e}")
    
    print("\nTransparency conversion complete!")

if __name__ == "__main__":
    main() 