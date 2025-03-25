# `define_usd_preview_surface`

This compound demonstrates one possible way to create a material in Bifrost-USD. It creates a prim that defines a basic material with a `UsdPreviewSurface` shader.

## Inputs

### `path`

The path for the new prim, for example, `/materials/my_material`. If the output of this node is connected to the `children` port of a prim definition downstream, this path will be appended to the path specified there.

## Base

### `diffuse_color`

The diffuse color, or albedo.

### `texture_file_path`

The path to a texture file to use instead of `diffuse_color`.

### `emissive_color`

The color of emitted light.

## Specular

### `clearcoat`

The weight of the clearcoat layer.

### `index_of_refraction`

The index of Refraction for translucent objects and specular objects.

### `metallic`

The degree of metalness. Use 1 for metallic objects and 0 for dielectric objects.

### `opacity`

The opacity of the material.

### `roughness`

The roughness of specular reflections. 0 is perfectly specular and 1 is maximum roughness.

## Additional inputs

### `attribute_definitions`

Optionally, connect one or more attribute definitions for the material. You can right-click on the port and choose **Connect Node** to quickly add and connect a `define_usd_attribute`.

## Outputs

### `material_definition`

The material prim definition. You can connect this into the `prim_definitions` port of an `add_to_stage` node, or to the `children` port of another `define_usd_prim` or similar node. You can also connect this into the `material` port of a `define_usd_prim` or similar node to bind the material to the prim.
