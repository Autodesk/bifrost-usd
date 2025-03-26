# `get_or_create_collection`

Creates or returns a collection on the USD prim.

## Inputs

### `stage`
The stage holding the prim. 

### `prim_path`
The path of the prim you want to add a collection to. 

### `collection_name`
The name of the prim collection to create. 

### `rule`
Specifies how the paths that are included in the collection must be expanded to determine its members. 

### `include_paths`
Includes the given Targets' paths in the collection. 

### `exclude_paths`
Excludes the given Targets' paths from the collection. 

## Outputs

### `out_stage`
The modified USD stage. 

### `success`
Boolean indicating whether the operation was successful.
