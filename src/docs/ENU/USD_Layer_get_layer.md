# `get_layer`

This node returns a sublayer from the stage's root layer. The last element in the list of sublayers is the strongest of the sublayers in the Pixar USD root layer.

## Inputs

### `stage`
The USD stage. 

### `layer_index`
The sublayer index. If -1, the root layer is returned. 

## Outputs

### `layer`
The returned layer. 

