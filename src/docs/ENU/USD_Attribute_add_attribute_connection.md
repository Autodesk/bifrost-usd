# `add_attribute_connection`

Adds a connection from a `source` attribute to the list of connections in the specified `position`.

## Inputs

### `stage`
The USD stage in which to add an attribute connection. 

### `prim_path`
The path to the prim holding the attribute. 

### `attribute_name`
The name of the attribute. 

### `source`
Full path of the source attribute to connect to. 

### `position`
For more details on USD list operations, see USD SDK documentation for `UsdListPosition`. 

## Outputs

### `out_stage`
The modified stage. 

### `success`
Boolean indicating whether the operation was successful. 
