# `create_primvar`

This node creates a USD UsdGeomPrimvar on a USD prim.

## Inputs

### `stage`
The USD stage on which to create the primvar. 

### `prim_path`
The prim path on which to add the attribute. 

### `name`
The attribute name. 

### `type_name`
The attribute type. 

### `interpolation`
How the Primvar interpolates over a geometric primitive. 

### `element_size`
Return the "element size" for this Primvar, which is 1 if unauthored.

## Outputs

### `out_stage`
The modified stage. 

### `success`
Boolean indicating whether the operation was successful. 
