# `inspect_usd_variants`

Create a list of prims representing each variants of a VariantSet from a USD file on disk.

> <span style="color:#FFA07A">From bifrost-usd-lab pack. Included in namespace **USD::Utils**</span>

## Inputs

### `enable`

If not checked, shows the original stage created from the USD file.  
If checked, shows all the variants as different prims.  
Every prims are layout along the X axis (except for the ones not being point based geo).  

### `stage`

The USD stage with at least one prim with a VariantSet.  

### `prim_path`

The path to the prim with a VariantSet in the USD file. 

### `padding`

Extra space to add between each variants along the X axis.  
