# `get_variant_selection`

This node returns the currently selected variant in a variant set.

## Inputs

### `stage`
The USD stage holding the prim you want to query.

### `prim_path`
The path to the prim with the variant set.

### `variant_set_name`
The name of the variant set for which to get the selection.

## Outputs

### `selection`
The name of the currently selected variant, or empty if no variant is selected.
