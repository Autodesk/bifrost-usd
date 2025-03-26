# `define_usd_prim_hierarchy`

Creates a hierarchy of Bifrost-USD prim definitions.
This creates the USD prim hierarchy in a stage when you connect the output into an `add_to_stage` node downstream.

For example, with *path* set to `/model/geo/grp1/mesh ` and *types* set to `Xform Scope Xform Mesh`, it will create the following USD hierarchy:

```
 `--model [def Xform] (kind = group)
     `--geo [def Scope] (kind = group)
         `--grp1 [def Xform] (kind = group)
             `--mesh [def Mesh] (kind = group)
```

## Inputs

## Hierarchy

### `path`

The full path of your prim hierarchy, for example `/Car/geo/doors/left`.
The path entered here will be prefixed to the paths specified by any connected children. Similarly if the output of this node is connected to the `children` port of a prim definition downstream, this path will be appended to the path specified there.

### `parent_is_scope`

Enforce that the parent of the hierarchy is a *Scope*, even if an other type is set by the `types` input. This is useful when the hierarchy is created by a script translating a hierarchy from the host application which has no such *Scope* type.

### `types`

A string of space separated USD prim types.

### `default_type`

If the number of prim types is smaller than the number of prims, the `default_type` will be used to set the type of the remaining prims.

## Mesh

### `normal_per_vertex`

Stores the normals per vertex, if they exist on the Bifrost mesh.

### `subdivision_scheme`

The subdivision method for the mesh. If `normal_per_vertex` is enabled, the subdivision method is ignored.

### `use_frame`

Whether to set the geometry data at the specified `frame`.

### `frame`

The frame for which to set the geometry data when `use_frame` is on.

### `attribute_definitions`

Optionally, connect one or more attribute definitions for the leaf mesh prim. You can right-click on the port and choose **Connect Node** to quickly add and connect a `define_usd_attribute` node or a common preset like `define_usd_transform` or `define_usd_display_color`.

### `relationship_definitions`

Optionally, connect one or more relationship definitions for the the leaf mesh prim. You can right-click on the port and choose **Connect Node** to quickly add and connect a `define_usd_relationship` node.

### `leaf_mesh`

If a Bifrost mesh is connected, creates the last prim definition of the hierarchy as corresponding USD Mesh.
Overrides the last type of the `types` input by the *Mesh* type.

