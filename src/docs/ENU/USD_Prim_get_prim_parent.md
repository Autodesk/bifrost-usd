# `get_prim_parent`

Returns the parent USD prim of a prim, and its path.

## Inputs

### `prim`
The USD prim.

## Outputs

### `parent`
The parent prim on the same stage, or an invalid prim if the input has no parent (for example the stage pseudo-root).

### `parent_path`
The path of the parent prim. Empty when there is no parent.
