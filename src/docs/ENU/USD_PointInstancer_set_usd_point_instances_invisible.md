# `set_usd_point_instances_invisible`

To hide specific instances of a USD PointInstancer by setting its "invisibleIds" attribute.

## Inputs

### `enable`

Toggle this off to skip setting the instances as invisible, and output the unmodified stage instead.

### `stage`

The USD stage holding the PointInstancer prim.

### `prim_path`

The path of a prim (of type PointInstancer) from which the data is returned.

### `invisible_ids`

An array of ids used to hide point instances. To know the value of the point instance ids, you can connect the `positions` output port of a `get_usd_point_instancer_attributes` node to the `array` input port of a `get_array_indices`. The `indices` output port of the `get_array_indices` will return all the ids.
You can build a smaller array containing some of those ids to hide specific instances.

### `replace`

When toggled on, the `ìnvisible_ids` replace the data of any existing "invisibleIds" attribute present on the PointInstancer prim.
When toggled off, it is added to the data to any existing "invisibleIds" attribute present on the PointInstancer prim.

## Outputs

### `out_stage`

The modified USD stage.
