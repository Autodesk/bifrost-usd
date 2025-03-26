# `define_usd_display_color`

Defines the `primvars:displayColor` attribute. This is a predefined attribute that can be used for previewing. This node is a specialized compound based on `define_usd_attribute`.

## Inputs

### `color`

The display color to set.

## Outputs

### `attribute_definition`

The display color attribute. Connect this to the `attribute_definitions` port of `define_usd_prim` and other nodes to set attributes on the corresponding prims.
