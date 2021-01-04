# `define_usd_reference`

Defines a USD reference. You can connect the output into the `reference_definitions` port of `define_usd_prim` and similar nodes.

## Inputs

### `arc_type`

The type of composition arc. 

### `prim_path`

The path to the prim being referenced. 

### `relative_prim_path`

Whether the prim path is relative.

### `layer`

Optionally, connect a layer.

### `layer_offset`

The time offset to apply to animation.

### `layer_scale`

The time scale to apply to animation.

### `position`

The list position at which to add the reference.

## Output

### `reference_definition`

The reference definition. You can connect this into the `reference_definition` port of `define_usd_prim` and similar nodes.
