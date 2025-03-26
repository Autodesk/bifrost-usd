# `usd_point_instancer_scope`

A diagnostic compound for visualizing and analyzing USD PointInstancer as Bifrost points.

## Inputs

### `stage`

The USD stage holding the PointInstancer prim.

### `point_instancer_path`

The path of a prim (of type PointInstancer) to visualize.

### `local_space`

Display points in local space of the PointInstancer prim.

### `filter_mode`

Controls the prototype instances to visualize.
- `All`: All Point instances will be displayed through the Diagnostic Terminal.
- `Less`: Only Point Instances using a prototype index smaller than `prototype_index` will be displayed through the Diagnostic Terminal.
- `Less or Equal`: Only Point Instances using a prototype index smaller than or equal to `prototype_index` will be displayed through the Diagnostic Terminal.
- `Equal`: Only Point Instances using a prototype index equal to `prototype_index` will be displayed through the Diagnostic Terminal.
- `Greater or Equal`: Only Point Instances using a prototype index greater than or equal to `prototype_index` will be displayed through the Diagnostic Terminal.
- `Greater`: Only Point Instances using a prototype index greater than `prototype_index` will be displayed through the Diagnostic Terminal.

### `prototype_index`

The prototype index used by the `filter_mode`

## Point Shape

### `shape`

Controls which shape the points will be visualized as.

### `default_size`

Controls the size of each visualized point.

## Axis

### `show_axis`

Displays points as axis to visualize their orientation.

### `size`

Controls the axis size.

### `nubbin_size`

Controls the tip size of the axis.

### `stem_size`

Controls the size of the axis stem.

## Outputs

### `points`

Bifrost points with an extra property `point_usd_proto_indices` storing the data of the PointInstancer protoIndices attribute.
You can connect this output to the `points` input of a `create_instances` node.

### `instance_geometries`

Array of Bifrost meshes created from the prototypes of the USD PointInstancer of type *Mesh*.
You can connect this output to the `instance_geometries` input of a `create_instances` node.

### `instance_id_override`

Array of longs created from the protoIndices of the USD PointInstancer.

### `prototype_paths`

Array of strings representing the USD Prim path of each prototype.