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

### `traverse_instances`
When `true`, instance proxy traversal is enabled: if the prim (or any prim encountered during descent) is an instance, its children are returned as instance proxies rather than being skipped. When `false` (the default), instances are treated as opaque leaves and their children are not returned.

## Outputs

### `children`
The array of children. 

