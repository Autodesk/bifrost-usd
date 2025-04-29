# `duplicate_usd_prim_definition`

Creates copies of Bifrost-USD prim definitions based on a source prim definition.

## Inputs

### `prim_definition`

The source prim definition you want to duplicate.

## Settings

### `use_arc`

Creates a USD arc for each prim copy.

## Arc

### `arc_type`

The type of composition arc.

### `instanceable`

Declares whether the defined prim can be used for instances. Only prims that have `instanceable` explicitly set to `True` can be instanced.
- `None`: No `instanceable` metadata is created.
- `False`: The prim is not a candidate for instancing.
- `True`: The prim is a candidate for instancing. The prim becomes an instance of an implicit prototype when composed on a stage, if it also contains one or more direct composition arcs.

### `duplicate_name`

The name used by the duplicated prim(s). If not provided, the source prim definition name will be used.

### `count`

The number of prim definitions.

### `from_points`

If a point based Bifrost geometry is connected to this input, the prim will be duplicated using its *point_position*, *point_orientation* and *point_scale*.
The `count`, `translation`, `rotation` and `scale` parameters will be ignored.

## Transform

### `ignore_source_transform`

If enable, the source prim definition transform will not be applied to the duplicated prims.


### `translation`

The translation to apply relative to the previous prim.

### `rotation`

The rotation in degrees to apply to each prim based on the `uniform_rotation` parameter.

### `uniform_rotation`

If enable, the rotation is applied uniformly to all duplicated prims.
If disabled, the rotation is applied relative to the previous prim.

### `scale`

The scale to apply to each prim based on the `uniform_scale` parameter.

### `uniform_scale`

If enable, the scale is applied uniformly to all duplicated prims.
If disabled, the scale is applied relative to the previous prim.


## Outputs

### `prim_definitions`

The duplicated prim definitions.

