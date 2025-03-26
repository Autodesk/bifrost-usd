# `define_usd_point_instancer`

Defines a USD point instancer prim based on a Bifrost points object. This creates instances in a stage when you connect the output into an `add_to_stage` node downstream. This node is a specialized compound based on `define_usd_prim`.

## Inputs

### `instancer_path`

The path for the new instancer prim, for example, `/rocks` or `/debris/rocks`. If the output of this node is connected to the `children` port of a prim definition downstream, this path will be appended to the path specified there.

### `points`

Connect a Bifrost points object.

### `prototype_definitions`

Connect prim definitions for the prims to be instanced. You can right-click on the port and choose **Connect Node** to quickly add and connect any of several common nodes.

## Selection

### `selection_mode`

Controls how the prototypes are selected. This is ignored when `instance_id_override` is connected.
- `None`: Prototypes are not instanced.
- `Sequential`: The first connected prototype is instanced for point 0, the second for point 1, and so on. The selection wraps around, so that if there are N connections then the first connected prototype is used for point N, and so on.
- `Random`: Randomly assigns a prototype to each point. You can adjust the distribution using `probability_curve` and `seed`.

### `instance_id_override`

Provides direct control over how the prototypes are instanced. You can connect an array of integers matching the number of points, where each element specifies the prototype to use for the corresponding point.

### `probability_curve`

Edit the curve to adjust the distribution when `selection_mode` is `Random`.

### `seed`

The seed used by the random number generator when `selection_mode` is `Random`.

## Default Material

### `create_default_material`

Assigns a default material to the instances.

### `diffuse_color`

The diffuse color of the default material.

### `texture_file_path`

The path to a texture file to use instead of `diffuse_color`.

## Time

### `frame`, `use_frame`
<!-- NEEDS VETTING -->
`frame` and `use_frame` allow you to set the current time on the point instancer, which will effect the time samples the instanced geometry are taken from. 

## Variant Selection

These parameters are optional.

### `variant_set_name`

Optionally, enter the name of the variant set for which the prim defines a variant. If this is left empty, the prim is not a member of any variant set.

### `variant_name`

The name of the variant defined by this prim. 

## Outputs

### `point_instancer_definition`

The new prim definition. You can connect this into the `prim_definitions` port of an `add_to_stage` node, or to the `children` port of another `define_usd_prim` or similar node.
