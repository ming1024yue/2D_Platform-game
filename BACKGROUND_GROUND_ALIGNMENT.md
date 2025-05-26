# Ground Layer Alignment System

## Overview

The ground background layer has been enhanced to properly align with the actual game platforms where the player stands, rather than just being an arbitrary background image.

## Key Changes

### 1. Ground Platform Constant
A new constant `GROUND_HEIGHT` has been added to maintain consistency across all ground-related positioning:

```cpp
static constexpr float GROUND_HEIGHT = 60.f; // Height of ground platform from bottom of screen
```

### 2. Aligned Ground Layer Positioning
The ground background layer now specifically aligns with the actual game platforms:

- **Ground Level**: `WINDOW_HEIGHT - GROUND_HEIGHT` (540 pixels with 600px window)
- **Ground Layer Position**: Shows 40% of the texture above the ground platforms
- **Visual Coverage**: Ground texture extends from above the platform level down to the bottom of the screen

### 3. Smart Parallax Adjustment
The ground layer uses proper parallax movement that matches camera movement while maintaining alignment:

```cpp
// Position ground texture to cover ground platforms and extend below
startY = groundLevel - (scaledHeight * 0.4f); // 40% above ground level

// Apply full parallax effect for ground layer
startY += (viewCenter.y - WINDOW_HEIGHT / 2.0f) * layer.parallaxSpeed;
```

## Benefits

### Visual Consistency
- ✅ Ground texture appears "behind" and "under" the game platforms
- ✅ No visual disconnect between platforms and background
- ✅ Ground texture provides context for what the platforms represent

### Gameplay Enhancement
- ✅ Players can see ground detail/texture around platforms
- ✅ The world feels more cohesive and realistic
- ✅ Background provides depth without interfering with gameplay

### Technical Improvements
- ✅ Consistent positioning using `GROUND_HEIGHT` constant
- ✅ Easy to modify ground level for all game elements
- ✅ Proper parallax movement maintains alignment during scrolling

## Layer Positioning

| Layer | Parallax Speed | Positioning | Purpose |
|-------|----------------|-------------|---------|
| Sky | 0.0 (static) | Fills entire screen | Static background |
| Clouds | 0.1 (very slow) | From top of screen | Distant atmosphere |
| Mountains | 0.3 (medium) | From top of screen | Mid-distance terrain |
| **Ground** | **1.0 (full)** | **Aligned with platforms** | **Foreground surface** |

## Implementation Details

### Before
- Ground layer positioned arbitrarily from top of view
- No relationship to actual game platforms
- Visual disconnect between background and gameplay elements

### After
- Ground layer positioned relative to `WINDOW_HEIGHT - GROUND_HEIGHT`
- Shows ground texture above and below platform level
- Maintains alignment during camera movement and parallax scrolling

## Usage

The system automatically positions the ground layer correctly. For level designers and artists:

1. **Ground Textures**: Should show surface detail that would appear around/under platforms
2. **Texture Height**: Should be tall enough to cover from above platform level to bottom of screen
3. **Tiling**: Ground textures tile horizontally to cover the entire level width

## Examples

### Forest Level
- Ground texture shows grass, dirt, and forest floor details
- Appears under wooden platforms naturally
- Creates cohesive forest environment

### Desert Level
- Ground texture shows sand dunes and desert surface
- Appears under stone/sand platforms naturally  
- Creates cohesive desert environment

### Snow Level
- Ground texture shows snow and ice details
- Appears under icy platforms naturally
- Creates cohesive winter environment

## Technical Implementation

The alignment is handled in `drawBackgroundLayers()` with special logic for the ground layer:

```cpp
if (layer.name == "ground") {
    float groundLevel = WINDOW_HEIGHT - GROUND_HEIGHT;
    startY = groundLevel - (scaledHeight * 0.4f);
    startY += (viewCenter.y - WINDOW_HEIGHT / 2.0f) * layer.parallaxSpeed;
}
```

This ensures the ground background layer always appears in the correct position relative to the game platforms, creating a seamless integration between background art and gameplay elements. 