# `open_stage_from_cache`

Opens from the cache the stage that is associated to the given ID.

## Inputs

### `id`
The ID associated to a stage in the cache. 

### `layer_index`
The sublayer index to set as the stage's EditTarget. The last element in the list of sublayers is the strongest of the sublayers in the Pixar USD root layer. If the sublayer index is -1 or if it does not identify an existing sublayer, the root layer is set as the EditTarget of the opened stage.

## Outputs

### `stage`
The USD stage. 

