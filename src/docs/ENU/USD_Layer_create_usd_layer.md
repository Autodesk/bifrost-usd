# `create_usd_layer`

Creates a new USD layer.

## Inputs

### `layer`

The file path under which the layer will be saved. The `usd` (generic), `usda` (ASCII text), and `usdc` (crate binary) formats are supported.

### `file_format`

Whether to create this layer as a USD binary or ASCII file. Switching a `.usd` file to write as ascii text without changing the filename can be useful for debugging USD output. 

### `sublayers`

Connect any layers to add as sublayers of the new layer. Note that in Bifrost-USD, the first sublayer (index 0) holds the weakest opinions, which is the opposite of how sublayers are indexed when using the USD Python or C++ API.

## Outputs

### `new_layer`

The new layer.
