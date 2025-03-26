# `save_usd_stage`

Saves all modified layers in a Bifrost USD stage to files on disk. The `usd` (generic), `usda` (ASCII text), and `usdc` (crate binary) formats are supported.

## Inputs

### `stage`

Connect the stage to save.

### `enable`

Saves all modified layers. When this is off, the input stage is simply output without saving. You can toggle `enable` on and off again whenever you want to save the USD files, or leave it on if you want to continually save all modifications.

### `file`

The path of the file to save. Leave this empty to use the existing `save_file` path of the input stage.

## Options

### `relative_path`

Stores sublayers using their paths relative the the file of the root layer.

### `default_prim`

The stage's root prim first child will be used as the default prim.

### `enable_meters_per_unit`

Creates the meters per unit stage metadata.

### `meters_per_unit`

The meters per unit value.

### `enable_up_axis`

Creates the up axis stage metadata.

### `up_axis`

The up axis value.

## Outputs

### `out_stage`

Connect this output to an `output` node upstream.
