# `usd_volume`

This node creates a USD volume prim.

## Inputs

### `stage`
The stage in which to create a volume prim. 

### `prim_path`
The path to the volume prim. 

### `file_format`
The volume file format. 

### `field_names`
A list of field names. 

### `file_paths`
A list of file paths pointing to a supported volume file format on disk. 

### `relationship_names`
A list of names used by the renderer to associate individual fields with the named input parameters on the volume. 

### `frame`
The frame at which to evaluate the files. 

## Outputs

### `out_stage`
The modified USD stage. 

### `output`
Boolean indicating whether the operation was successful.
