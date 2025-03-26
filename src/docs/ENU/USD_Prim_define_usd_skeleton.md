# `define_usd_skeleton`

Creates a Bifrost-USD prim definition based on a Bifrost skeleton. This creates a USD Skel prim in a stage when you connect the output into an `add_to_stage` node downstream. This node is a specialized compound based on `define_usd_prim`.

## Inputs

### `path`

The path for the new prim, for example, `/car` or `/street/car`. The path entered here will be prefixed to the paths specified by any connected children. Similarly if the output of this node is connected to the `children` port of a prim definition downstream, this path will be appended to the path specified there.

### `skeleton`

Connect a Bifrost skeleton object.

## General

### `applied_schema_names`

Coma-separated names of USD Applied API Schemas.

## Time Sample

### `use_frame`

Whether to set the geometry data at the specified `frame`.

### `frame`

The frame for which to set the geometry data when `use_frame` is on.

## Variant Selection

### `variant_name`

Optionally, enter the name of the variant set for which the prim defines a variant. If this is left empty, the prim is not a member of any variant set.

### `variant_set_name`

The name of the variant defined by this prim.

## Additional inputs

### `reference_definitions`

Optionally, connect one or more reference definitions for the new prim. You can right-click on the port and choose **Connect Node** to quickly add and connect a `define_usd_reference` node.

### `attribute_definitions`

Optionally, connect one or more attribute definitions for the new prim. You can right-click on the port and choose **Connect Node** to quickly add and connect a `define_usd_attribute` node or a common preset like `define_usd_transform` or `define_usd_display_color`.

### `children`

Optionally, connect one or more children for the new prim.  You can right-click on the port and choose **Connect Node** to quickly add and connect any of several common nodes.

### `relationship_definitions`

Optionally, connect one or more relationship definitions for the new prim. You can right-click on the port and choose **Connect Node** to quickly add and connect a `define_usd_relationship` node.

### `material`

Optionally, connect a material definition such as the output from a `define_usd_preview_surface` node.

## Outputs

### `skeleton_definition`

The new prim definition. You can connect this into the `prim_definitions` port of an `add_to_stage` node, or to the `children` port of another `define_usd_prim` or similar node.
