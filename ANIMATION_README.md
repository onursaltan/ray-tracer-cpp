# Ray Tracer Animation System

## Overview

The ray tracer now supports keyframe-based animations! You can animate objects by defining their position, rotation, and scale at different times.

## How to Use

### 1. **Single Frame Render** (Default)

In `main.cpp`, set:
```cpp
bool renderAnimation = false;  // Render single frame
```

This renders one frame to `out.ppm` with all statistics.

### 2. **Animation Render**

In `main.cpp`, set:
```cpp
bool renderAnimation = true;   // Render animation frames
int totalFrames = 90;          // Number of frames (90 frames = 3 seconds at 30fps)
float frameDuration = 1.0f / 30.0f;  // Frame duration (30 FPS)
```

This renders 90 frames to `frames/out_000.ppm`, `frames/out_001.ppm`, etc.

### 3. **Creating Animations**

Define animations in `main.cpp`:

```cpp
// Create animation
Animation cubeRotation;
cubeRotation.addKeyframe({0.0f, Vec3(-3, 1, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)});
cubeRotation.addKeyframe({2.0f, Vec3(-3, 1, 0), Vec3(0, 6.28f, 0), Vec3(1, 1, 1)});
cubeRotation.setLooping(true);

// Assign to object
objects[0]->setAnimation(&cubeRotation);
```

**Keyframe format**: `{time, position, rotation, scale}`
- `time`: Time in seconds (0.0 = start, 2.0 = 2 seconds)
- `position`: Vec3(x, y, z)
- `rotation`: Vec3(rotX, rotY, rotZ) in radians
- `scale`: Vec3(scaleX, scaleY, scaleZ)

### 4. **Looping**

```cpp
animation.setLooping(true);   // Animation loops
animation.setLooping(false);  // Animation plays once
```

### 5. **Getting Duration**

```cpp
float duration = animation.getDuration();  // Get animation length in seconds
```

## Examples

### Example 1: Simple Rotation

```cpp
Animation spin;
spin.addKeyframe({0.0f, Vec3(0, 1, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)});
spin.addKeyframe({1.0f, Vec3(0, 1, 0), Vec3(0, 6.28f, 0), Vec3(1, 1, 1)});
// Full rotation in 1 second
```

### Example 2: Spin + Scale

```cpp
Animation spinScale;
spinScale.addKeyframe({0.0f, Vec3(0, 2, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)});
spinScale.addKeyframe({1.0f, Vec3(0, 2, 0), Vec3(0, 3.14159f, 0), Vec3(1.5, 1.5, 1.5)});
spinScale.addKeyframe({2.0f, Vec3(0, 2, 0), Vec3(0, 6.28f, 0), Vec3(1, 1, 1)});
// Rotates and scales over 2 seconds
```

### Example 3: Movement Path

```cpp
Animation move;
move.addKeyframe({0.0f, Vec3(0, 0, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)});
move.addKeyframe({1.0f, Vec3(5, 0, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)});
move.addKeyframe({2.0f, Vec3(5, 5, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)});
move.addKeyframe({3.0f, Vec3(0, 5, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)});
// Object moves in a square path over 3 seconds
```

## Converting Frames to Video

### Prerequisites
1. Install ffmpeg from https://ffmpeg.org/download.html
2. Add ffmpeg to your PATH (or it will be used if in the same directory)

### Create Video
Run the batch script:
```bash
create_video.bat
```

This creates `output.mp4` from all frames in the `frames/` directory.

### Manual Command
```bash
ffmpeg -framerate 30 -i frames/out_%03d.ppm -c:v libx264 -pix_fmt yuv420p output.mp4
```

## Performance Tips

1. **Reduce frame count** for faster testing:
   ```cpp
   int totalFrames = 30;  // 1 second at 30fps instead of 90
   ```

2. **Animation with soft shadows** is slow. For faster animation:
   - Reduce light radius in `scene.txt`
   - Use `renderAnimation = false` first to test static scene

3. **Monitor progress** in console output showing:
   - Frame number being rendered
   - Total rays shot per frame
   - Render time

## Current Animations (in main.cpp)

- **cube1**: Rotates 360° in 2 seconds
- **sphere3**: Complex rotation + scale animation over 3 seconds

## Editing Keyframes

Modify keyframes in `main.cpp` to change animation behavior:

```cpp
Animation cubeRotation;
cubeRotation.addKeyframe({0.0f, Vec3(-3, 1, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)});
                         // Change these values to modify animation
cubeRotation.addKeyframe({2.0f, Vec3(-3, 1, 0), Vec3(0, 6.28f, 0), Vec3(1, 1, 1)});
                         // Change these values to modify animation
```

## Radians Reference

For rotation values (in radians):
- π/2 = 1.57 (90°)
- π = 3.14159 (180°)
- 2π = 6.28 (360°)
- π/4 = 0.785 (45°)

---

**Enjoy creating animations!** 🎬✨
