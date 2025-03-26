# `get_forwarded_relationship_targets`

Composes this relationship's ultimate targets, taking into account "relationship forwarding" and returns the list.

## Inputs

### `stage`
The USD stage in which to get relationship targets. 

### `prim_path`
The prim that has the relationship. 

### `rel_name`
The relationship name. 

## Outputs

### `out_stage`
The modified USD stage. 

### `success`
Boolean indicating whether the operation was successful.

### `targets`
The relationship target paths. 
