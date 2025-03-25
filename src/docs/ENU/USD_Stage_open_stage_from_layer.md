# `open_stage_from_layer`

Opens a stage from a given layer.

## Inputs

### `root_layer`
The layer that will be the root in the stage. 

### `mask`
Loads only the specified prims. If this is empty, all the prims are loaded. 

### `load`
Controls the behavior of USD payload arcs. LoadAll loads all loadable prims, and LoadNone records but does not traverse payload arcs (useful on large scenes to override something without pulling everything). 

### `layer_index`
The sublayer index to set as the stage's EditTarget. The last element in the list of sublayers is the strongest of the sublayers in the Pixar USD root layer. If the sublayer index is -1 or if it does not identify an existing sublayer, the root layer is set as the EditTarget of the opened stage.

## Outputs

### `stage`
The USD stage. 

