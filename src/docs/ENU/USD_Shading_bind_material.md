# `bind_material`

Binds a material to a prim directly, or to a collection on the prim. The `collection_name` must be specified to bind to a collection. `binding_name` can stay empty.

## Inputs

### `stage`
The stage holding the prim and material. 

### `prim_path`
The path of the prim you want to bind the material to, or on which there is a collection you want to bind the material to. 

### `material_path`
The path of the material. 

### `binding_strength`
The material binding strength. 

### `material_purpose`
If not equal to "All", the binding applies only to the specified material purpose. 

### `collection_prim_path`
The prim path holding the collection. If empty, the prim_path input will be used. 

### `collection_name`
The name of the collection you want to assign the material to. 

### `binding_name`
When binding to a collection, establishes an identity for the binding that is unique on the prim. This is ignored when creating a direct binding. 

## Outputs

### `out_stage`
The modified USD stage. 

### `success`
Boolean indicating whether the operation was successful.
