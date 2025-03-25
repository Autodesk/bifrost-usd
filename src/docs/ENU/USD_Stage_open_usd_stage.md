# `open_usd_stage`

Opens a USD file as a stage. The `usd` (generic), `usda` (ASCII text), and `usdc` (crate binary) formats are supported for reading and writing, and `usdz` files are supported for reading only.

## Inputs

### `file`

The file to open as a stage. 

### `read_only`

Prevents the input file from being overwritten when the modified stage is saved or exported by a node downstream.
- When this is checked, you must create a new stage and then add sublayers from the input stage or reference prims from it to the new stage.
- When this is unchecked, you should specify a different path in `save_file`. 

### `save_file`

When `read_only` is off, this is the path and name of the file that will be used to save the stage by nodes downstream. This value is ignored when `read_only` is on. Make sure that this path is different from the input file to avoid a loop.

### `mask`

 Connect an array of prim paths (strings) to limit stage population to the specified prims, their descendants, and ancestors. Note that since the stage mask is not stored in a USD layer, the Bifrost stage need to be shared in order to see the effect in the host application. If the host is sublayering or referencing the Bifrost stage's root layer, then the mask will not be taken into account.

### `load`

Controls the behavior of USD payload arcs.
- `LoadAll`: All loadable prims are loaded.
- `LoadNone`: Payload arcs are recorded, but not traversed.

## Outputs

### `stage`

The opened stage.
