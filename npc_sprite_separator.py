#!/usr/bin/env python3
"""
NPC Sprite Sheet Separator
Separates NPC sprite sheet into idle and walking animations.
"""

import cv2
import numpy as np
import os

def separate_npc_sprites(sprite_path, output_dir, frame_width=32, frame_height=32):
    """
    Separate NPC sprite sheet into idle and walking animations.
    First row is idle, second row is walking.
    """
    # Load the sprite sheet
    sprite_sheet = cv2.imread(sprite_path)
    if sprite_sheet is None:
        raise ValueError(f"Could not load sprite sheet from {sprite_path}")
    
    height, width = sprite_sheet.shape[:2]
    
    print(f"Processing sprite sheet: {width}x{height}")
    print(f"Frame size: {frame_width}x{frame_height}")
    
    # Create output directories
    os.makedirs(output_dir, exist_ok=True)
    os.makedirs(os.path.join(output_dir, "idle"), exist_ok=True)
    os.makedirs(os.path.join(output_dir, "walking"), exist_ok=True)
    
    # Number of frames per row
    frames_per_row = width // frame_width
    
    # Process idle frames (first row)
    idle_frames = []
    for i in range(frames_per_row):
        x = i * frame_width
        frame = sprite_sheet[0:frame_height, x:x+frame_width]
        if frame.size > 0:
            idle_frames.append(frame)
            # Save individual frame
            frame_path = os.path.join(output_dir, "idle", f"idle_frame_{i:02d}.png")
            cv2.imwrite(frame_path, frame)
            print(f"Saved idle frame {i}: {frame_path}")
    
    # Process walking frames (second row)
    walking_frames = []
    for i in range(frames_per_row):
        x = i * frame_width
        frame = sprite_sheet[frame_height:2*frame_height, x:x+frame_width]
        if frame.size > 0:
            walking_frames.append(frame)
            # Save individual frame
            frame_path = os.path.join(output_dir, "walking", f"walking_frame_{i:02d}.png")
            cv2.imwrite(frame_path, frame)
            print(f"Saved walking frame {i}: {frame_path}")
    
    # Create combined sprite sheets for each animation
    if idle_frames:
        combined_width = len(idle_frames) * frame_width
        combined_idle = np.zeros((frame_height, combined_width, 3), dtype=np.uint8)
        for i, frame in enumerate(idle_frames):
            x_offset = i * frame_width
            combined_idle[:, x_offset:x_offset+frame_width] = frame
        cv2.imwrite(os.path.join(output_dir, "idle", "idle_spritesheet.png"), combined_idle)
        print("Created idle animation sprite sheet")
    
    if walking_frames:
        combined_width = len(walking_frames) * frame_width
        combined_walking = np.zeros((frame_height, combined_width, 3), dtype=np.uint8)
        for i, frame in enumerate(walking_frames):
            x_offset = i * frame_width
            combined_walking[:, x_offset:x_offset+frame_width] = frame
        cv2.imwrite(os.path.join(output_dir, "walking", "walking_spritesheet.png"), combined_walking)
        print("Created walking animation sprite sheet")

if __name__ == "__main__":
    sprite_path = "assets/images/npc/32x32.png"
    output_dir = "assets/images/npc/separated"
    separate_npc_sprites(sprite_path, output_dir) 