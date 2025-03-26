# `remove_payload_prim`

Removes the given prim payload from the given stage.

## Inputs

### `stage`
The USD stage in which to remove the payload prim. 

### `prim_path`
The prim path from where to remove a reference. 

### `payload_layer_identifier`
The file of the reference to remove (empty if internal) 

### `payload_prim_path`
The prim path of the reference to remove 

### `layer_offset`
The layer time offset 

### `clear_all`
Remove every references, ignoring the other parameters 

## Outputs

### `out_stage`
The modified USD stage. 

### `success`
Boolean indicating whether the operation was successful.