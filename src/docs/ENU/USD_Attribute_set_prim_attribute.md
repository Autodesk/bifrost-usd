# `set_prim_attribute`

This node sets the attribute value with the specified data.

## Inputs

### `stage`
The USD stage in which to set an attribute. 

### `prim_path`
The path to the prim holding the attribute. 

### `name`
The name of the attribute you want to set. 

### `value`
The data to set the attribute to.

### `use_frame`
If enabled, sets the attribute at the given frame. 

### `frame`
The frame at which to set the attribute data. 

## Outputs

### `out_stage`
The new stage with the modified attribute. 

### `success`
Boolean indicating whether the operation was successful. 
