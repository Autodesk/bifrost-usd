# `get_attribute_metadata`

This node gets the value of an attribute's metadata.

## Inputs

### `stage`
The USD stage. 

### `prim_path`
The path to the prim holding the attribute. 

### `attribute_name`
The name of the attribute. 

### `key`
The metadatum key. 

### `default_and_type`
The type of Bifrost value, and the default value if the metadatum could not be returned. This works similarly to the type on `get_geo_property` and similar nodes.

## Outputs

### `success`
Boolean indicating whether the operation was successful.

### `value`
The metadatum value. 
