# `define_usd_attribute`

Defines a USD attribute and, optionally, its value. The output can be connected to the `attribute_definitions` port of `define_usd_prim` and other nodes to set attributes on the corresponding prims.

## Inputs

### `name`

The name of the attribute.

### `custom`

Declares whether the attribute is user-defined.

## Type

### `type`

The USD type of the attribute.

### `enable_value`

Sets the attribute to the `value` set below. If this is unchecked, the attribute is defined but no value is set.

### `value`

By default, this port is a Bifrost `float` value but you can right-click to set a different type or simply connect a value with the type you want. Use a Bifrost type that is compatible with the USD `type` above, for example,  `float3` for Normal3f or Color3f, `string` for Token or Asset, and so on.

## Connections

### `enable_connection`

Connects the attribute to the specified prim's attribute. If this is unchecked, no connection is made.

### `connection_mode`

<!-- NEEDS DOCS, UNCEAR WHAT THIS DOES -->

### `target_prim`

The path of the prim to connect to.

### `target_attribute`

The attribute of the prim to connect to.

## Primvars

### `enable_primvar`

Declares to renderers that the attribute is associated with geometric primitives and can vary over the surface or volume.

### `interpolation`

Specifies how the attribute should be interpolated across the surface or volume.

## Time Sample

### `frame`

The frame at which to set `value` when `use_frame` is true.

### `use_frame`

Whether to store the frame with the value.

## Outputs

### `attribute_definition`

The defined attribute. Connect this to the `attribute_definitions` port of `define_usd_prim` and other nodes to set attributes on the corresponding prims.
