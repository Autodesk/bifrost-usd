# `define_usd_transform`

Defines a USD transform attribute. This node is a specialized compound based on `define_usd_attribute`.

## Inputs

### `translation`

The XYZ positions.

### `rotation`

The Euler angles for the XYZ axes in degrees. The order of rotation is X, Y, Z.

### `scale`

The XYZ scaling factors.

## Outputs

### `attribute_definitions`

The transform attribute definition. You can connect this into the `attribute_definitions` port of `define_usd_prim` and similar nodes.
