# `define_usd_geom_subset`

Define a USD GeomSubset storing an array of face ids.

## Inputs

### `path`

The path of the prim definition of type GeomSubset.

### `element_type`

The type of elements this geometry subset includes. Only "face" is supported at the moment.

### `indices`

The set of indices included in this subset. 
The indices need not be sorted, but the same index should not appear more than once.

### `family_name`

The name of the family of subsets that this subset belongs to. 
This is optional and can be used when roundtripping subset between DCC apps.

## Outputs

### `prim_definition`

The new prim definition. You can connect this into the `prim_definitions` port of an `add_to_stage` node, or to the `children` port of another `define_usd_prim` or similar node.