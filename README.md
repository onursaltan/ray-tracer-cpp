# Ray Tracer

A CPU ray tracer written in C++. Supports spheres, cubes, planes, and OBJ models with Phong shading, reflections, refractions, soft shadows, textures, normal maps, and BVH acceleration.

## Build & Run

```bash
g++ -O2 -o main.exe main.cpp
./main.exe
```

Outputs `out.ppm`. Open it with any image viewer that supports PPM, or convert it with something like ImageMagick.

## Settings (`settings.json`)

```json
{
    "render": {
        "width": 700,
        "height": 700,
        "samplesPerPixel": 4,
        "useBVH": true,
        "shadowSamples": 4,
        "maxBounces": 5
    },
    "animation": {
        "enabled": false,
        "totalFrames": 125,
        "fps": 25
    }
}
```

- **samplesPerPixel** -- anti-aliasing. 1 is fast and noisy, 8+ looks clean.
- **shadowSamples** -- soft shadow quality. 1 gives hard shadows, higher values soften them.
- **maxBounces** -- how deep reflections/refractions go.
- **useBVH** -- keep this on unless you're benchmarking without it.

## Scene (`scene.json`)

The scene file defines the camera, lights, and objects.

### Camera

```json
"camera": {
    "position": [0, 3, 10],
    "target": [0, 1, 0],
    "fov": 55
}
```

### Lights

Three types: `pointLights`, `directionalLights`, `spotLights`.

```json
"pointLights": [{
    "position": [5, 8, 5],
    "color": [1, 1, 1],
    "intensity": 30.0,
    "radius": 0.5
}]
```

`radius` controls soft shadow size. Set to 0 for hard shadows.

Spot lights also take `direction`, `innerAngle`, and `outerAngle` (in degrees).

### Objects

Supported types: `spheres`, `cubes`, `planes`, `models` (OBJ files).

```json
"spheres": [{
    "name": "my_sphere",
    "position": [0, 1, 0],
    "rotation": [0, 0, 0],
    "scale": [1, 1, 1],
    "radius": 1,
    "segments": 32,
    "material": "red"
}]
```

- `segments` on spheres controls mesh resolution.
- `planes` take a `size` field instead of radius.
- `models` take a `filepath` pointing to an `.obj` file.

## Materials (`materials.json`)

Materials are defined by name and referenced in the scene.

```json
"my_material": {
    "diffuse": [0.8, 0.2, 0.2],
    "specular": [0.5, 0.5, 0.5],
    "shininess": 64.0,
    "ambient": 0.1,
    "reflectivity": 0.3,
    "glossiness": 0.8
}
```

For transparent materials, add:

```json
"transparency": 1.0,
"ior": 1.5,
"transmissionColor": [1.0, 1.0, 1.0],
"absorptionDistance": 5.0
```

- **ior** -- index of refraction (1.0 = air, 1.33 = water, 1.5 = glass, 2.42 = diamond)
- **transmissionColor** + **absorptionDistance** -- Beer's law tinting. Lower distance = stronger color.
- **glossiness** -- 1.0 = sharp reflections, lower = blurry.

For textured materials:

```json
"diffuseTexture": "textures/wood.jpg",
"normalTexture": "textures/wood-normal.jpg",
"roughnessTexture": "textures/wood-roughness.jpg",
"textureScale": 2.0
```

There's also a built-in `"checkerboard"` procedural texture you can use as `diffuseTexture`.
