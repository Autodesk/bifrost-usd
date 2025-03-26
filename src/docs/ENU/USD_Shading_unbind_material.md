# `unbind_material`

Unbinds a direct or collection-based binding. The `binding_name` must be specified to unbind a collection.

## Inputs

### `stage`
The stage holding the prim and material. 

### `prim_path`
The path of the prim you want to unbind the material from. For collections, this is the path to the prim holding the collection. 

### `material_purpose`
Unbinds only the specified material purpose. 

### `binding_name`
The binding name of the collection. 

## Outputs

### `out_stage`
The modified USD stage. 

### `success`
Boolean indicating whether the operation was successful.
