# `objects_to_stage`

Creates a USD stage from an array of Bifrost objects. For each object the compound automatically detects the geometry type (mesh, strands, points, or point instancer) and creates the corresponding USD prim definition before adding it to the stage.

The stage hierarchy will follow this pattern:

```
/
 `--root [def Xform] (kind = group)
     `--geo [def Scope] (kind = group)
         |--instancer1 [def PointInstancer]
         |   `--prototypes [def]
         |       `--obj_0 [def Scope]
         |           |--_render [def Mesh]
         |           `--_proxy [def Mesh]
         `--mesh2 [def Mesh]
```

## Inputs

### `objects`

The array of Bifrost objects from which to create the USD stage. You can connect multiple objects by fanning them into this port. The compound automatically detects whether each object is a mesh, strands, points, or point instancer.

### `identifier`

The identifier of the root layer for the new stage, for example `meshes.usd`.

### `default_prim_path`

The path of the default prim for the stage. By default, this is set to `/root`.

## Options

### `purpose`

The USD imageable purpose assigned to the geometry prims. Allowed values are `Default`, `Render`, `Proxy`, and `Guide`.

### `use_frame`

If enabled, sets the geometry data at the given frame.

### `frame`

The frame at which to set the geometry data when `use_frame` is on.

### `varying_topology`

Enable this when the topology of one or more objects changes between frames. When off, only point positions are written as time samples, which is more efficient for deforming geometry with a fixed topology.

## Outputs

### `stage`

The USD stage containing the prims created from the input objects.
