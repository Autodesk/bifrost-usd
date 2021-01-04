# `create_usd_layer`

Creates a new USD layer.

## Inputs

### `layer`

The file path under which the layer will be saved. The `usd` (generic), `usda` (ASCII text), and `usdc` (crate binary) formats are supported.

### `sublayers`

Connect any layers to add as sublayers of the new layer. Note that in Bifrost-USD, the first sublayer (index 0) holds the weakest opinions, which is the opposite of how sublayers are indexed when using the USD Python or C++ API.

## Output

### `new_layer`

The new layer.