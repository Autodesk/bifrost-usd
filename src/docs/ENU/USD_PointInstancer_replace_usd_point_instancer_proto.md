# `replace_usd_point_instancer_proto`

Replace elements in the "protoIndices" attribute of a USD PointInstancer.

## Inputs

### `stage`

The USD stage holding the PointInstancer prim.

### `point_instancer_path`

The path of a prim (of type PointInstancer) you wish to replace prototypes indices from.

### `mode`

Controls how the prototype instance is replaced.
- `Less than`: Point Instances using a proto index smaller than `proto_index` will be updated to use `new_proto_index`.
- `Less or equal than`: Point Instances using a proto index smaller than or equal to `proto_index` will be updated to use `new_proto_index`.
- `Equal to`: Point Instances using a proto index equal to `proto_index` will be updated to use `new_proto_index`.
- `Greater or equal to`: Point Instances using a proto index greater than or equal to `proto_index` will be updated to use `new_proto_index`.
- `Greater than`: Point Instances using a proto index greater than `proto_index` will be updated to use `new_proto_index`.

### `proto_index`

The current proto index.

### `new_proto_index`

The new proto index applied to Point Instances matching the rule set by the `mode` and `proto_index`.

## Outputs

### `out_stage`

The modified USD stage.