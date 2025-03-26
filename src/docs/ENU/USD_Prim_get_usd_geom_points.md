# `get_usd_geom_points`

This node outputs the points data of a point-based USD prim at the requested frame.

## Inputs

### `prim`
The USD Prim storing the points. 

### `local_space`
Gets the points positions in local space. When off, the positions are returned in world space. 

### `frame`
The requested frame. 

## Outputs

### `success`
Boolean indicating whether the operation was successful.


### `points`
Array containing all the points. 
