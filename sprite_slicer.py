from PIL import Image
import os

def slice_spritesheet(input_path, output_dir, tile_size=16):
    # Open the sprite sheet
    sprite_sheet = Image.open(input_path)
    width, height = sprite_sheet.size
    
    # Create output directory if it doesn't exist
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # Calculate number of tiles in each row and column
    num_tiles_x = width // tile_size
    num_tiles_y = height // tile_size
    
    # Slice the sprite sheet
    for y in range(num_tiles_y):
        for x in range(num_tiles_x):
            # Calculate tile coordinates
            left = x * tile_size
            top = y * tile_size
            right = left + tile_size
            bottom = top + tile_size
            
            # Extract and save tile
            tile = sprite_sheet.crop((left, top, right, bottom))
            tile_filename = f'tile_{y}_{x}.png'
            tile_path = os.path.join(output_dir, tile_filename)
            tile.save(tile_path)
            print(f'Saved {tile_filename}')

if __name__ == '__main__':
    input_path = 'assets/images/Enviroment.png'
    output_dir = 'assets/images/environment_tiles'
    slice_spritesheet(input_path, output_dir)
    print('Sprite sheet slicing complete!') 