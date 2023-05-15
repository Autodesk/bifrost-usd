# `define_usd_curves`

Creates a Bifrost-USD curve definition based on strands. This creates a USD `BasisCurves` prim in a stage when you connect the output into an `add_to_stage` node downstream. This node is a specialized compound based on `define_usd_prim`.

## Inputs

### `strands`

Connect a Bifrost strands object.

### `path`

Enter the path for the new prim, for example, `/mustache` or `/hair/mustache`. The path entered here will be prefixed to the paths specified by any connected children. Similarly if the output of this node is connected to the `children` port of a prim definition downstream, this path will be appended to the path specified there.

## `reference_definitions`

Optionally, connect one or more reference definitions for the new prim. You can right-click on the port and choose **Connect Node** to quickly add and connect a `define_usd_reference` node.

## `attribute_definitions`

Optionally, connect one or more attribute definitions for the new prim. You can right-click on the port and choose **Connect Node** to quickly add and connect a `define_usd_attribute` node or a common preset like `define_usd_transform` or `define_usd_display_color`.

## `children`

Optionally, connect one or more children for the new prim.  You can right-click on the port and choose **Connect Node** to quickly add and connect any of several common nodes.

## `relationship definitions`

Optionally, connect one or more relationship definitions for the new prim. You can right-click on the port and choose **Connect Node** to quickly add and connect a `define_usd_relationship` node.

## `basis`

The basis specifies the vstep and matrix used for cubic interpolation.
Only used when `type` is not linear. It is used to interpolate the vertices of the cubic basis curves. Each individual curve is composed of one or more segments. Each segment is defined by four vertices for cubic curves and two vertices for linear curves.

Allowed Values:	bezier, bspline, catmullRom

## `type`

Linear curves interpolate linearly between two vertices.
Cubic curves use a basis matrix with four vertices to interpolate a segment.

Allowed Values: linear, cubic

## `material`

Optionally, connect a material definition such as the output from a `define_usd_preview_surface` node.

## Output

### `curves_definition`

The new prim definition. You can connect this into the `prim_definitions` port of an `add_to_stage` node, or to the `children` port of another `define_usd_prim` or similar node.
