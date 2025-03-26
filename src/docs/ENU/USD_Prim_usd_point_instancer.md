# `usd_point_instancer`

This node creates a point instancer.

## Inputs

### `stage`
The stage in whict to create a point instancer. 

### `prim_path`
The path to the point instancer. 

### `prototypes`
Array containing the paths for all instance prototypes. These are the prims that that you are instancing. 

### `protoindices`
Array containing all instance prototype indices. 

### `positions`
Array containing all instance positions. This array must be the same size as protoindices. 

### `orientations`
Array containing all instance orientations. This array must be either the same size as protoindices or empty. WARNING: USD Array uses halfs instead of floats. Bifrost uses floats so there may be some loss of precision. 

### `scales`
Array containing all instance scales. This array must be either the same size as protoindices or empty. 

### `velocities`
Array containing all instance velocities. This array must be either the same size as protoindices or empty. 

### `accelerations`
Array containing all instance accelerations. This array must be either the same size as protoindices or empty. 

### `angular_velocities`
Array containing all instance angular velocities. This array must be either the same size as protoindices or empty. 

### `invisible_ids`
A list of IDs to be made invisible at evaluation time. 

## Outputs

### `out_stage`
The modified USD stage. 

### `success`
Boolean indicating whether the operation was successful.
