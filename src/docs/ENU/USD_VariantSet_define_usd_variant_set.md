# `define_usd_variant_set`

Defines a USD variant set. Connect the output into the `variant_set_definitions` port of `define_usd_prim` or a similar node. For each variant, you can create another prim definition with the corresponding `variant_set_name` and its specific `variant name`, and then connect that into the `children` port of the same `define_usd_prim` or a similar node.

## Inputs

### `variant_set_name`

The name for the variant set.

### `selection`

The name of the variant that will be the default selection.

## Outputs

### `variant_set_definition`

The variant set definition.
