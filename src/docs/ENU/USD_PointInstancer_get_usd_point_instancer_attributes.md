# `get_usd_point_instancer_attributes`

Returns data used by each instance of a USD PointInstancer.

## Inputs

### `stage`

The USD stage holding the PointInstancer prim.

### `prim_path`

The path of a prim (of type PointInstancer) from which the data is returned.

## Outputs

### `scales`

The per-instance scale applied to each instance, before any rotation is applied.

### `orientations`

The per-instance orientation of each instance relative to its prototype's origin, represented as a unit length quaternion.

### `positions`

The per-instance position.

### `proto_indices`

The per-instance index into prototypes relationship that identifies what geometry should be selected for each instance.


### `prototype_paths`

The prim path of every prototype used by the PointInstancer.