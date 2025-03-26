# `get_material_path`

Returns the material path of a USD prim.

## Inputs

### `prim`
The prim you want to get the material path from. 

### `material_purpose`
Specifies the purpose for which you want to get the material. The binding applies only to the specified material purpose. 

### `compute_bound_material`
Computes the resolved bound material for this prim. 

## Outputs

### `success`
Boolean indicating whether the operation was successful.

### `path`
The path to the material or an empty string if no material is found. 
