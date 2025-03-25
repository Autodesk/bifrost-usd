# `create_usd_prim`

Adds a USD prim to the current layer of a stage.

## Inputs

### `stage`

The stage on which to add the prim.

### `path`

The path for the new prim, for example, `/car` or `/street/car`.  

### `type`

The type of the prim, such as: `Scope`, `Xform`, `Capsule`, `Cone`, `Cube`, `Cylinder`, `Sphere`, `Mesh`, `NurbsCurves`, `PointInstancer`, and so on. The complete list of types depends on what has been registered by your renderer and other plug-ins.

## Outputs

### `out_stage`

The modified stage with the added prim.
