# `create_prim_definitions_from_object`

Creates a Bifrost-USD prim definition from a Bifrost object. The compound automatically detects the geometry type (mesh, strands, points, or point instancer) and creates the appropriate prim definition. Connect the output into an `add_to_stage` node downstream to create USD prims on a stage.

## Inputs

### `prim_index`

When the object does not carry a `prim_path` property, if `append_index` is enabled, an integer index is used to generate the default prim path.

### `append_index`

When enabled, the `prim_index` value is appended to the prim name in the generated path.

### `object`

The Bifrost object from which to create the prim definition. The compound automatically detects whether it is a mesh, strands, points, or point instancer and creates the corresponding USD prim definition.

## Time Sample

### `use_frame`

If enabled, sets the geometry data at the given frame.

### `frame`

The frame at which to set the geometry data when `use_frame` is on.

## Outputs

### `prim_definition`

The new prim definition. Connect this into the `prim_definitions` port of an `add_to_stage` node, or to the `children` port of another `define_usd_prim` or similar node.
