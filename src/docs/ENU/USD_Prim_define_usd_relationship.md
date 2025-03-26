# `define_usd_relationship`

Defines a USD relationship. You can connect the output into the `relationship_definitions` port of `define_usd_prim` and similar nodes. 

## Inputs

### `rel_name`

The name of the relationship to define.

### `custom`

Declares whether the relationship is user-defined.

### `target`

The path to the target.

### `local_path`
<!-- NEEDS VETTING -->
Whether the target path is to be interpreted as being local to the prim that's being defined.

### `target_position`

The list position at which to add the relationship.

## Outputs

### `relationship_definition`

The relationship definition. You can connect the output into the `relationship_definitions` port of `define_usd_prim` and similar nodes.
