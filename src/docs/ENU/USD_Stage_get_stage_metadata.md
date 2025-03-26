# `get_stage_metadata`

This node gets the value of a stage metadatum if the stage's current UsdEditTarget is the root or session layer.

> Todo: Amino does not accept InOut ports on scalars (BIFROST-6207) 

## Inputs

### `stage`
The USD stage. 

### `key`
The metadatum key. 

### `default_and_type`
The type of Bifrost value, and the default value if the metadatum could not be returned. This works similarly to the type on `get_geo_property` and similar nodes.

## Outputs

### `success`
Boolean indicating whether the operation was successful.

### `value`
The metadatum value. 
