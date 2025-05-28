#!/usr/bin/env python3
"""
Make sprite backgrounds transparent
Removes the background from sprite frames and saves them with transparency
"""

from PIL import Image
import os

def make_transparent(input_path, output_path, threshold=30):
    """
    Make the background of a sprite transparent.
    Assumes the background is relatively uniform in color.
    """
    # Open the image
    img = Image.open(input_path).convert('RGBA')
    data = img.getdata()

    # Get background color from the top-left pixel
    bg_color = data[0][:3]  # RGB values of the background

    # Create new image data with transparency
    new_data = []
    for item in data:
        # Check if the pixel is close to the background color
        if (abs(item[0] - bg_color[0]) < threshold and 
            abs(item[1] - bg_color[1]) < threshold and 
            abs(item[2] - bg_color[2]) < threshold):
            # Make it transparent
            new_data.append((255, 255, 255, 0))
        else:
            # Keep the original color
            new_data.append(item)

    # Update the image with new data
    img.putdata(new_data)
    
    # Save the image
    img.save(output_path, 'PNG')
    print(f"Processed: {output_path}")

def process_directory(directory):
    """
    Process all PNG files in a directory and its subdirectories
    """
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.png'):
                input_path = os.path.join(root, file)
                # Save in the same location, overwriting the original
                make_transparent(input_path, input_path)

if __name__ == "__main__":
    sprite_dir = "assets/images/npc/separated"
    print(f"Processing sprites in {sprite_dir}...")
    process_directory(sprite_dir)
    print("All sprites processed!") 