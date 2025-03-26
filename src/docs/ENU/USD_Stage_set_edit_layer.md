# `set_edit_layer`

This node sets the stage's EditTarget to the specified layer.

## Inputs

### `stage`
The stage in which to set the edit layer. 

### `layer_index`
The sublayer index to set as the stage's EditTarget. The last element in the list of sublayers is the strongest of the sublayers in the Pixar USD root layer. The index -1 identifies the root layer. If the index is not -1 and does not identify an existing sublayer, this node does nothing and the current stage's EditTarget is preserved. 


### `layer_display_name`
Set the stage's EditTarget using the display name of the layer. The display name is the base filename of the identifier.
Note that the layer identifier can not be used as it can change at every execution of the graph when such layer is an anonymous layer (like a new layer created by Bifrost USD). On the other hand, the display name of an anonymous layer won't change per execution. However, if several layers are using the same display name (for example if there is a "look.usd" sublayer from  directory "A" and an other "look.usd" sublayer in directory "B"), then the layer on top of the other one with same display name will be targeted. In such scenario the `layer_index` should be used instead to remove any ambiguity on which layer you want to target.

## Outputs

### `out_stage`
The modified USD stage.