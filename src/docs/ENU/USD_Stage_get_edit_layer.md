# `get_edit_layer`

This node gets the layer targeted by the stage.

## Inputs

### `stage`
The stage from which to get the edit layer. 

### `read_only`
If enabled, the original layer is returned. Since it is not a copy, it can be useful to retrieve the layer index of an in-memory layer by comparing layer identifiers. If you need to modify the returned layer, you must turn it off or connect a set_layer_permission node after this node. 

## Outputs

### `edit_layer`
The returned layer. 

