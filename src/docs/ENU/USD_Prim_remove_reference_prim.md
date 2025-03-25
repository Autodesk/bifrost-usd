# `remove_reference_prim`

Removes the given prim reference from the given stage.

## Inputs

### `stage`
The USD stage in which to remove the reference prim. 

### `prim_path`
The prim path from where to remove a reference. 

### `reference_layer_identifier`
The file of the reference to remove (empty if internal). 

### `reference_prim_path`
The prim path of the reference to remove. 

### `layer_offset`
The layer time offset. 

### `clear_all`
Removes every reference, ignoring the other parameters. 

## Outputs

### `out_stage`
The modified USD stage. 

### `success`
Boolean indicating whether the operation was successful.
