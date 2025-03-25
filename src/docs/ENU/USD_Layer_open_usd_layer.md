# `open_usd_layer`

Opens a USD file as a layer. The `usd` (generic), `usda` (ASCII text), and `usdc` (crate binary) formats are supported for reading and writing, and `usdz` files are supported for reading only.

## Settings

### `file`

The file to open as a layer. 

### `read_only`

Prevents the input file from being overwritten when saved or exported by a node downstream.

### `save_file`

When `read_only` is off, this is the path and name of the file that will be used to save the layer by nodes downstream. This value is ignored when `read_only` is on. Make sure that this path is different from the input file to avoid a loop.

### Output

### `layer`

The opened layer.
