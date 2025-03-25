# `get_prim_metadata`

Gets the value of a prim's metadata.

## Inputs

### `stage`
The USD stage. 

### `path`
The path to the USD prim. 

### `key`
The metadatum key. 

### `default_and_type`
The type of Bifrost value, and the default value if the metadatum could not be returned. This works similarly to the type on `get_geo_property` and similar nodes.

## Outputs

### `success`
Boolean indicating whether the operation was successful.


### `value`
The metadatum value. 
