# `set_relationship_targets`

Sets the target list explicitly to the list provided. Note that this fails if any of the targets are invalid.

## Inputs

### `stage`
The USD stage in which to set relationship targets. 

### `prim_path`
The prim that has the relationship. 

### `rel_name`
The relationship name. 

### `targets`
The relationship target paths. 

## Outputs

### `out_stage`
The modified USD stage. 

### `success`
Boolean indicating whether the operation was successful.
