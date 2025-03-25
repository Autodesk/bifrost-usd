# `add_reference_prim`

Adds a prim reference in the given stage.

## Inputs

### `stage`
The USD stage in which to add a reference prim. 

### `prim_path`
The prim that will reference something. 

### `reference_layer`
The referenced layer (empty if internal). 

### `reference_prim_path`
The prim that will be referenced. 

### `layer_offset`
The layer time offset. 

### `layer_scale`
The layer offset scale factor. 

### `reference_position`
The position in the reference list. 

### `anchor_path`
The anchor path is the front part of the layer identifier that you don't want to include in the reference list. If an empty or invalid anchor path is provided, nothing from the layer identifier will be removed. 

## Outputs

### `out_stage`
The modified USD stage. 

### `success`
Boolean indicating whether the operation was successful.
