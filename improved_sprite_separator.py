#!/usr/bin/env python3
"""
Improved Sprite Sheet Separator for Finn Character
Separates sprite frames based on analysis of the actual sprite structure.
"""

import cv2
import numpy as np
import os
import argparse

def separate_finn_sprites(sprite_path, output_dir, frame_width=32):
    """
    Separate Finn sprite sheet into movement categories.
    Based on common sprite sheet layouts, we'll make educated guesses about frame groupings.
    """
    # Load the sprite sheet
    sprite_sheet = cv2.imread(sprite_path)
    if sprite_sheet is None:
        raise ValueError(f"Could not load sprite sheet from {sprite_path}")
    
    height, width = sprite_sheet.shape[:2]
    num_frames = width // frame_width
    
    print(f"Processing sprite sheet: {width}x{height}")
    print(f"Extracting {num_frames} frames of {frame_width}x{height} pixels")
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    # Common sprite sheet organization patterns:
    # For a 28-frame sprite sheet, typical groupings might be:
    # - Idle: 4-6 frames
    # - Walking: 6-8 frames  
    # - Jump: 2-4 frames
    # - Attack: 4-6 frames
    # - Get Hit: 2-3 frames
    # - Die: 4-6 frames
    
    # Let's try a reasonable grouping based on common patterns
    movement_groups = {
        'idle': list(range(0, 4)),        # Frames 0-3: Idle animation
        'walking': list(range(4, 12)),    # Frames 4-11: Walking cycle
        'jump': list(range(12, 16)),      # Frames 12-15: Jump sequence
        'attack': list(range(16, 22)),    # Frames 16-21: Attack animation
        'get_hit': list(range(22, 25)),   # Frames 22-24: Hit reaction
        'die': list(range(25, 28))        # Frames 25-27: Death animation
    }
    
    # Extract and save frames for each movement
    all_frames = []
    
    # First, extract all individual frames
    for i in range(num_frames):
        x_start = i * frame_width
        frame = sprite_sheet[:, x_start:x_start + frame_width]
        all_frames.append(frame)
    
    # Save frames by movement category
    for movement, frame_indices in movement_groups.items():
        if not frame_indices:
            continue
            
        movement_dir = os.path.join(output_dir, movement)
        os.makedirs(movement_dir, exist_ok=True)
        
        print(f"\nProcessing {movement} animation:")
        print(f"  Frames: {frame_indices}")
        
        movement_frames = []
        
        # Save individual frames
        for i, frame_idx in enumerate(frame_indices):
            if frame_idx < len(all_frames):
                frame = all_frames[frame_idx]
                movement_frames.append(frame)
                
                # Save individual frame
                frame_path = os.path.join(movement_dir, f"{movement}_frame_{i:02d}.png")
                cv2.imwrite(frame_path, frame)
                print(f"    Saved: {frame_path}")
        
        # Create combined sprite sheet for this movement
        if movement_frames:
            combined_width = len(movement_frames) * frame_width
            combined_height = height
            combined_sheet = np.zeros((combined_height, combined_width, 3), dtype=np.uint8)
            
            for i, frame in enumerate(movement_frames):
                x_offset = i * frame_width
                combined_sheet[:, x_offset:x_offset + frame_width] = frame
            
            combined_path = os.path.join(movement_dir, f"{movement}_spritesheet.png")
            cv2.imwrite(combined_path, combined_sheet)
            print(f"    Created combined sheet: {combined_path}")

def create_manual_separator_tool(sprite_path, output_dir, frame_width=32):
    """
    Create a tool for manual separation with visual preview.
    """
    sprite_sheet = cv2.imread(sprite_path)
    height, width = sprite_sheet.shape[:2]
    num_frames = width // frame_width
    
    # Create a numbered preview for manual identification
    preview_frames = []
    
    for i in range(num_frames):
        x_start = i * frame_width
        frame = sprite_sheet[:, x_start:x_start + frame_width].copy()
        
        # Add frame number overlay
        cv2.putText(frame, str(i), (2, height-5), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
        preview_frames.append(frame)
    
    # Create grid layout
    frames_per_row = 8
    num_rows = (num_frames + frames_per_row - 1) // frames_per_row
    
    preview_width = frames_per_row * frame_width
    preview_height = num_rows * height
    preview = np.zeros((preview_height, preview_width, 3), dtype=np.uint8)
    
    for i, frame in enumerate(preview_frames):
        row = i // frames_per_row
        col = i % frames_per_row
        
        preview_y = row * height
        preview_x = col * frame_width
        preview[preview_y:preview_y + height, preview_x:preview_x + frame_width] = frame
    
    preview_path = os.path.join(output_dir, "numbered_frames_preview.png")
    cv2.imwrite(preview_path, preview)
    print(f"Numbered preview saved to: {preview_path}")
    print("Use this preview to manually identify which frames belong to which movements.")

def main():
    parser = argparse.ArgumentParser(description='Separate Finn sprite sheet by movement')
    parser.add_argument('--sprite', default='assets/images/characters/FinnSprite.png', 
                       help='Path to sprite sheet')
    parser.add_argument('--output', default='assets/images/characters/separated_finn', 
                       help='Output directory')
    parser.add_argument('--frame-width', type=int, default=32, 
                       help='Width of each frame in pixels')
    parser.add_argument('--manual', action='store_true', 
                       help='Create numbered preview for manual identification')
    
    args = parser.parse_args()
    
    try:
        if args.manual:
            print("Creating manual separation tool...")
            create_manual_separator_tool(args.sprite, args.output, args.frame_width)
        else:
            print("Separating sprites automatically...")
            separate_finn_sprites(args.sprite, args.output, args.frame_width)
        
        print(f"\nProcess complete! Check {args.output} for results.")
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main() 