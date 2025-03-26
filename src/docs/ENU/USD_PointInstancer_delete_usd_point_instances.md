# `delete_usd_point_instances`

Delete point instances from a USD PointInstancer.

## Inputs

### `enable`

Toggle this off to skip the deletion of point instances, and output the unmodified stage instead.

### `stage`

The USD stage holding the PointInstancer prim.

### `point_instancer_path`

The path of a prim (of type PointInstancer) you wish to delete point instances from.

### `invert`

Delete all the point instances not matching the rule of the selected `mode`.

### `mode`

Controls how the point instances are deleted.
- `Random`: Delete point instances based on a probability weight threshold. A random number is generated for each instance, and if the number is less than the specified probability_weight, it is deleted.
- `Inside Geometry`: Delete point instances that are contained within another geometry. For example this could be used to delete point instances that are inside a bounding mesh.
- `By Indices`: Connect an *array<long>* containing the indices of point instances to get direct controls in which points to delete. The point instances indices can be deduce by connecting the *positions* output port of a *get_usd_point_instancer_attributes* node to a *get_array_indices* node.

### `probability_weight`

When the `Random` mode is selected, if the random number generated for a point instance is below this threshold, it will be deleted.

### `seed`

The seed for the random number generation. Different seeds will produce a different set of deleted point instances.

### `geometries`

One or more geometries to be used when the `Inside Geometry` mode is selected.

### `indices`

One or more *long* or *array<long>* to be used when the `By Indices` mode is selected.

## Outputs

### `out_stage`

The modified USD stage.

### `out_prim_path`

The prim path of the modified PointInstancer.

### `new_point_count`

The point instances count of the modified PointInstancer
