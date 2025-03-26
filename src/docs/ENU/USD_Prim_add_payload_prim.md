# `add_payload_prim`

Adds a payload arc to a prim.

## Inputs

### `stage`
The USD stage in which to add a payload.

### `prim_path`
The prim that will reference something. 

### `payload_layer`
The payload layer (empty if internal). 

### `payload_prim_path`
The prim that will be payloaded. 

### `layer_offset`
The layer time offset. 

### `layer_scale`
The layer offset scale factor. 

### `payload_position`
The position in the payload list. 

### `anchor_path`
The anchor path is the front part of the layer identifier that you don't want to include in the reference list. If an empty or invalid anchor path is provided, nothing from the layer identifier will be removed. 

## Outputs

### `out_stage`
The USD stage in which to add the payload prim. 

### `success`
Boolean indicating whether the operation was successful. 
