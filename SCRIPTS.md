# Build and Run Scripts

This project includes convenient scripts to build and run the game easily.

## Available Scripts

### `./run` - Build and Run (Recommended)
```bash
./run
```
- ✅ Builds the project using `cmake --build build`
- ✅ Runs the game using `./build/game` 
- ✅ Automatically stops if build fails
- ✅ Ensures correct working directory

**This is the main script you should use for development!**

### `./compile` - Build Only
```bash
./compile
```
- ✅ Builds the project using `cmake --build build`
- ✅ Shows build status
- ✅ Doesn't run the game (useful for checking compilation)

### `./clean` - Clean Build Artifacts
```bash
./clean
```
- ✅ Removes compiled objects and temporary files
- ✅ Keeps the executable but cleans intermediate files
- ✅ Use when you want a fresh build

## Usage Examples

### Normal Development Workflow
```bash
# Build and run in one command
./run
```

### Just Check if Code Compiles
```bash
# Build only (don't run)
./compile
```

### Clean Build (when things go wrong)
```bash
# Clean and rebuild
./clean
./compile
```

### Manual Commands (if you prefer)
```bash
# Manual build
cmake --build build

# Manual run (from project root!)
./build/game
```

## Important Notes

⚠️ **Always run from project root** (`/Users/startup/my-game/`)

✅ **Correct way:**
```bash
cd /Users/startup/my-game
./run
```

❌ **Wrong way:**
```bash
cd /Users/startup/my-game/build
./game  # This won't find assets!
```

## Why These Scripts?

1. **Convenience** - One command does everything
2. **Error Prevention** - Stops if build fails
3. **Correct Working Directory** - Ensures assets are found
4. **Consistent Environment** - Same experience every time
5. **Visual Feedback** - Clear status messages with emojis

## Troubleshooting

If you get permission errors:
```bash
chmod +x run compile clean
```

If assets aren't loading:
- Make sure you're running from the project root
- Check that `assets/` folder exists in the current directory
- Use `pwd` to verify you're in `/Users/startup/my-game` 