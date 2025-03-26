# `get_prim_children`

Returns the array of children of a prim.

## Inputs

### `stage`
The USD stage. 

### `prim_path`
The USD prim path. 

### `descendant_mode`
<!-- NEEDS VETTING -->
Children are the immediate children one level down in the hierarchy, descendants are all of the children, grandchildren, etc. at all levels of hierarchy. 

- UsdPrimChildren: Return this prim's active, loaded, defined, non-abstract children.
- UsdPrimAllChildren: Return all this prim's children.
- UsdPrimDescendants: Return this prim's active, loaded, defined, non-abstract descendants.
- UsdPrimAllDescendants: Return all this prim's descendants.

## Outputs

### `children`
The array of children. 

