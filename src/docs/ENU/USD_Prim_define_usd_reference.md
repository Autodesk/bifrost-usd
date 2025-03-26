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

### `anchor_path`
<!--- NEEDS VETTING --->
In the default USD asset resolver `ArDefaultResolver`, references can be relative to an "Anchor" path, which allows references to be relative to a larger structure. If `file` is only the end portion of this path, `anchor_path` is the beginning of it. 

## Outputs

### `reference_definitions`

The reference definition. You can connect this into the `reference_definition` port of `define_usd_prim` and similar nodes.
