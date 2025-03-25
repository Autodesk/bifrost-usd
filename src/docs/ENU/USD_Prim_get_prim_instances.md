# `get_prim_instances`

Returns all prims that are instances of this prototype, if this prim is a prototype prim.

Note that this function will return prims in prototypes for instances that are nested beneath other instances.

## Inputs

### `stage`
The USD stage 

### `proto_prim_path`
The path to a prototype USD prim. 

## Outputs

### `instances_paths`
Returns the paths for all the prototype prims in the stage. 

