# `create_prim_relationship`

Returns a new copy of the stage with a relationship added to a prim.

## Inputs

### `stage`
The USD stage in which to create a prim relationship. 

### `prim_path`
The prim that will own the relationship. 

### `rel_name`
The relationship name. 

### `custom`
Declares that the new relationship is user-defined. 

### `target`
The relationship target path. 

### `target_position`
The position of the relationship in the list. 

## Outputs

### `out_stage`
The modified USD stage. 

### `success`
Boolean indicating whether the operation was successful.
