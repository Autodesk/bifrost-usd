# `set_prim_metadata_by_dict_key`

Sets the value of a sub-element of a dictionary-valued prim metadatum, addressed by a key path inside the dictionary.

## Inputs

### `stage`
The stage in which to set the metadatum. 

### `path`
The path to the USD prim. 

### `key`
The top-level metadatum key (for example `customData` or `assetInfo`). 

### `key_path`
A ':'-separated path identifying the entry to set inside the dictionary stored at `key`. For example, `myDict:mySubDict:myEntry` addresses `myEntry` nested two dictionaries deep. 

### `value`
The value to author at `key_path` inside the dictionary at `key`. 

## Outputs

### `out_stage`
The modified USD stage. 

### `success`
Boolean indicating whether the operation was successful.
