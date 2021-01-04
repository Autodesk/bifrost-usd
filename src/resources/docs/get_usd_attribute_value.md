# `get_usd_attribute_value`

Returns the value of a USD attribute at a specific frame.

## Inputs

### `stage`

Connect a USD stage.

### `prim_path`

The path to the prim with the attribute.

### `attribute_name`

The name of the attribute to get.

### `type`

The Bifrost type that corresponds to the type of the USD attribute, for example, `float3` for Normal3f or Color3f, `string` for Token or Asset, and so on. You can right-click to set the port type or simply connect a value with the type you want. 

### `frame`

The frame at which to get the attribute value.

## Outputs

### `value`

The value of the attribute, if successful. When `success` is `false`, this is the default value for the type, such as 0 or the empty string.

### `success`

Returns `true` if the attribute's value was successfully returned.
