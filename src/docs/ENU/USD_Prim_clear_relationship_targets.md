# `clear_relationship_targets`

Removes all opinions about the target list from the current edit target.

## Inputs

### `stage`
The USD stage in which to clear a relationship target. 

### `prim_path`
The prim that has the relationship. 

### `rel_name`
The relationship name. 

### `remove_spec`
Additionally removes the spec. You should leave the spec if you want to preserve metadata. 

## Outputs

### `out_stage`
The modified USD stage. 

### `success`
Boolean indicating whether the operation was successful.
