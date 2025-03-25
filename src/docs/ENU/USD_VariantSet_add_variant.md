# `add_variant`

This node adds a new variant to the variant set on the given stage.

## Inputs

### `stage`
The USD stage on which to add the variant. 

### `prim_path`
The path to the prim to add variants to. 

### `variant_set_name`
The variant set name to add variants to. 

### `variant_name`
The variant's name. 

### `set_variant_selection`
<!-- NEEDS VETTING -->
If false, still creates the variant set but does not set the variant. 

## Outputs

### `out_stage`
The modified stage. 
