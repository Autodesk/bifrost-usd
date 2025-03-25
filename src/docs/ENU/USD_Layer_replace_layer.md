# `replace_layer`

This node replaces the sublayer from the stage's root layer at the specified index.

The last element in the list of sublayers is the strongest of the sublayers in the Pixar USD root layer.

## Inputs

### `stage`
The USD stage in which to replace the sublayer. 

### `sublayer_index`
The sublayer index (of the root layer) to replace. 

### `new_layer`
The new layer that will replace the existing one. 

## Outputs

### `out_stage`
The modified USD stage. 

### `success`
Boolean indicating whether the operation was successful.
