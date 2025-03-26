# `define_usd_prim`

Creates a Bifrost-USD prim definition. This creates a USD prim in a stage when you connect the output into an `add_to_stage` node downstream. 

## Inputs

### `path`

The path for the new prim, for example, `/car` or `/street/car`. The path entered here will be prefixed to the paths specified by any connected children. Similarly if the output of this node is connected to the `children` port of a prim definition downstream, this path will be appended to the path specified there.

### `variant_set_definitions`

Optionally, connect one or more variant set definitions for the new prim. You can right-click on the port and choose **Connect Node** to quickly add and connect a `define_usd_variant_set` node.

### `reference_definitions`

Optionally, connect one or more reference definitions for the new prim. You can right-click on the port and choose **Connect Node** to quickly add and connect a `define_usd_reference` node.

### `attribute_definitions`

Optionally, connect one or more attribute definitions for the new prim. You can right-click on the port and choose **Connect Node** to quickly add and connect a `define_usd_attribute` node or a common preset like `define_usd_transform` or `define_usd_display_color`.

### `children`

Optionally, connect one or more children for the new prim.  You can right-click on the port and choose **Connect Node** to quickly add and connect any of several common nodes.

### `relationship_definitions`

Optionally, connect one or more relationship definitions for the new prim. You can right-click on the port and choose **Connect Node** to quickly add and connect a `define_usd_relationship` node.

## General

### `type`

The type of the prim, such as: `Scope`, `Xform`, `Capsule`, `Cone`, `Cube`, `Cylinder`, `Sphere`, `Mesh`, `NurbsCurves`, `PointInstancer`, and so on. The complete list of types depends on what has been registered by your renderer and other plug-ins.

The `type` is ignored when `specifier` is `Over` or `Class`. If you leave `type` empty when `specifier` is `Def`, it creates a prim definition whose type can be specified later.

### `specifier`

The specifier for the prim:
- `Def`: Defines a prim.
- `Over`: Can be composed over a prim or PrimSpec.
- `Class`: Creates an abstract prim that other prims can reference.

### `kind`
- `None`: No `kind` metadata is created.
- `Assembly`: A special type of group such as a published asset or reference to a published asset.
- `Component`: A leaf model that cannot contain other models.
- `Group`: Used to group other models.
- `SubComponent`: A specially-identified portion of a component.

### `purpose`

Controls the visibility:
- `None`: No `purpose` metadata is created. In effect, this means that the prim is usually visible.
- `Render`: Visible only when rendering at final quality.
- `Proxy`: Visible only when when performing a lightweight proxy rendering.
- `Guide`: Visible only when explicitly showing guides.

### `active`
- `None`: No `active` metadata is created. By default, the prim is considered active.
- `False`: Deactivates the defined prim and its children. This is useful for pruning unnecessary scene description.
- `True`: Allows the defined prim to be composed and processed normally.

### `instanceable`

Declares whether the defined prim can be used for instances. Only prims that have `instanceable` explicitly set to `True` can be instanced.
- `None`: No `instanceable` metadata is created.
- `False`: The prim is not a candidate for instancing.
- `True`: The prim is a candidate for instancing. The prim becomes an instance of an implicit master when composed on a stage, if it also contains one or more direct composition arcs.

### `applied_schema_names`

Coma-separated names of USD Applied API Schemas.

## Variant Selection

These parameters are optional.

### `variant_set_name`

Optionally, enter the name of the variant set for which the prim defines a variant. If this is left empty, the prim is not a member of any variant set.

### `variant_name`

The name of the variant defined by this prim.

### `material`

Optionally, connect a material definition such as the output from a `define_usd_preview_surface` node.

## Outputs

### `prim_definition`

The new prim definition. You can connect this into the `prim_definitions` port of an `add_to_stage` node, or to the `children` port of another `define_usd_prim` or similar node.
