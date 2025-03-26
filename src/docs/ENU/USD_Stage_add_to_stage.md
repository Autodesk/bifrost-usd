# `add_to_stage`

Adds one or more Bifrost-USD prim definitions to a new or existing USD stage. This creates USD prims on the stage.

## Inputs

### `stage`

The USD stage to add to. If nothing is connected, an empty stage is created. You can right-click on the port and choose **Connect Node** to quickly add and connect a node that opens or creates a new stage.

### `prim_definitions`

The list of Bifrost-USD prim definitions to add. You can right-click on the port and then choose **Create Node** to quickly add and connect any of several common inputs.

## `Settings`

These settings apply to all added prims.

### `enable`

Toggle this off to skip the addition of prims, and output the unmodified stage instead.

### `parent_path`

The path of a prim to which the connected prims are to be added as children. For example, if this is `/set/geo/` and a connected prim definition has the path `/street/car`, the prim becomes `/set/geo/street/car` on the stage. The leading slash is required for an absolute path.
- If `parent_path` is omitted, the prims are added to the root.
- If one or more prims in the full path do not already exist on the stage, "placeholder" prims are automatically added to the hierarchy using the `def` specifier. The specifier of these can be changed downstream in the graph using another `add_to_stage` node.

### `layer`

The layer to which the prim definitions are to be added. You can use a *BifrostUsd::Layer* or a *string*.
If you connect a *BifrostUsd::Layer* and its layer identifier match one of the sublayer identifiers or the root layer identifier, it will target such layer, else the `layer_index` will be used as a fallback.
If you use a *string*, you can use the layer display name instead of the identifier. If more than one layer in the stage are using such display name, the prims will be added to the layer with the strongest opinion.

### `layer_index`

The index of the layer to which the prim definitions are to be added. Sublayers are indexed consecutively with 0 at the bottom of the stack and each successive layer above the previous. Layer 0 holds the weakest opinions, which is the opposite of how sublayers are ordered when using the USD Python or C++ API. The default of -1 refers to the root layer. The default of -1 refers to the root layer. If you specify an index that does not exist, layer -1 is used.

Note that you cannot specify a layer in a Bifrost-USD stage by name because the stage exists in memory rather than on disk. The identifier comprises a random hash (followed by an optional human-readable tag), and the hash can change each time the graph is executed.

### `use_material_name_or_tag`

Assigns prims with material tags to materials with matching tags. If no material has a matching tag, this option tries to match a prim's tag with a material's name instead. If there is still no match, then no material is assigned.

## Outputs

### `out_stage`

The stage with the added prim definitions.
