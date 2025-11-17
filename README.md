# HandParticles

Real-time particle system controlled by hand gestures via webcam. 20,000 particles respond to three hand gestures with distinct visual behaviors.

## Features

- **Gesture Detection**: Computer vision pipeline detects fist, open hand, and thumbs up
- **Particle Modes**: Each gesture triggers different particle behavior
- **Visual Feedback**: Navy ink particles with red velocity indication on eggshell background

## Gestures

| Gesture | Effect |
|---------|--------|
| **Fist** | Chaotic spin - particles rotate and jitter around screen center |
| **Open Hand** | Calm - particles slow down with minimal jitter |
| **Thumbs Up** | Form Cube - particles organize into rotating 3D cube formation |
| **None** | Normal - standard drift behavior |

## Controls

| Key | Action |
|-----|--------|
| `TAB` | Toggle GUI visibility |
| `s` | Save screenshot |
| `g` | Toggle gesture detection (auto/manual) |
| `b` | Capture background (remove hand first) |
| `c` | Toggle webcam view |
| `d` | Toggle debug info |
| `1-4` | Manual gesture override |

## Setup

### Requirements
- openFrameworks 0.12.1+
- ofxOpenCV
- ofxGui
- Webcam

### Build
```bash
make
make RunRelease
```

## Configuration

### CV Parameters (adjust via GUI)
- **Gesture Threshold** (30-255): Hand detection sensitivity
- **Blur Amount** (1-15): Noise reduction
- **Min/Max Contour Area**: Hand size bounds
- **Background Subtraction**: Toggle for cluttered scenes

### Particle Parameters
- **Spin Strength**: Vortex force intensity
- **Chaotic Jitter**: Noise amplitude
- **Formation Strength**: Cube attraction force
- **Rotation Speed**: Cube rotation rate

## Tips

- **Lighting**: Ensure good contrast between hand and background
- **Distance**: Keep hand 1-2 feet from camera
- **Background Subtraction**: Press `b` (hand out of frame), then enable toggle
- **Calibration**: Adjust threshold slider until hand appears white in processed view

## Technical Details

### Computer Vision Pipeline
1. Gaussian blur for noise reduction
2. Morphological operations (erode/dilate) to clean contours
3. Optional background subtraction
4. Convex hull analysis for finger detection
5. Temporal smoothing (5-frame voting) to prevent flicker

### Gesture Classification
- **Fist**: High solidity (≥0.75), 0-1 convexity defects
- **Open Hand**: Low solidity (<0.7), 2+ convexity defects
- **Thumbs Up**: 1 defect, tall aspect ratio (>1.3)

## Screenshots

*Add screenshots here after capturing with `s` key*

## License

MIT
