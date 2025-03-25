# `create_usd_stage`

Creates a new USD stage.

## Inputs

### `layer`

The file path under which the stage will be saved. The `usd` (generic), `usda` (ASCII text), and `usdc` (crate binary) formats are supported.

### `file_format`

Whether to save this file as a USD binary or ASCII file. Switching a `.usd` file to write as ascii text without changing the filename can be useful for debugging USD output. 

### `sublayers`

Connect any layers to add as sublayers of the new stage. Note that in Bifrost-USD, the first sublayer (index 0) holds the weakest opinions, which is the opposite of how sublayers are indexed when using the USD Python or C++ API.

### `mask`

 Connect an array of prim paths (strings) to limit stage population to the specified prims, their descendants, and ancestors. Note that since the stage mask is not stored in a USD layer, the Bifrost stage need to be shared in order to see the effect in the host application. If the host is sublayering or referencing the Bifrost stage's root layer, then the mask will not be taken into account.

### `load`

Controls the behavior of USD payload arcs.
- `LoadAll`: All loadable prims are loaded.
- `LoadNone`: Payload arcs are recorded, but not traversed.

### `layer_index`

The index of the target layer.

## Outputs

### `stage`

The new stage.
