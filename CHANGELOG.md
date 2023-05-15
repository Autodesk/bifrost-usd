## [1.2.0] - 2023-05-12

### Build

 - BIFROST-8086 - fix unit test errors

### Feature

 - BIFROST 8452 - Add color support to the _read_usd_curve_ and improve _define_usd_curves_ UI
    - Add option in _read_usd_curves_ to import _displayColor_ attribute as _point_color_ geo property.
    - Add Combo Box in _define_usd_curves_ to set basis and type parameters.

 - BIFROST-8452 - Add option in _read_usd_meshes_ to import _displayColor_ attribute as _point_color_ geo property

 - BIFROST-6771 - Add soft min-max sliders and color pickers widgets in following compounds
    - _define_usd_display_color_
    - _define_usd_point_instancer_
    - _define_usd_preview_surface_
    - _define_usd_transform_
    - _duplicate_usd_prim_definition_

 - BIFROST-8319 - Add _get_authored_attribute_names_ node, to get all authored attributes names 

 - BIFROST-8100 - Add _get_all_attribute_names_ node, to get all prim attributes names

### Bugfix

 - BIFROST-8426 - Inconsistent UI in define_usd_prim

 - BIFROST-8273 - fix sublayers not saved if relative_path is on
   - Use the layer's save file path (m_filePath) instead of the sdfLayerIdentifier in the recursive call to Layer::exportToFile().
   - Add new more complete unit test for export_layer_to_file() that covers multiple cases for relative and absolute paths to sublayers.
   - removed call to changeDir() that has side effect and is not required anymore for unit tests.


## [1.1.0] - 2023-03-29

### Build

- BIFROST-8068 - do not run test_graphs.py when system processor is not in target arch.

- BIFROST-8073 - update calls to xcrun to support latest xcode/macos

- BIFROST-8065 - add support to build for desired binary architecture on OSX

- BIFROST-7845 - Add test loading all compounds

### Feature

- BIFROST-7955 - Add applied schema nodes
   - add_applied_schema: This node adds the applied API schema name to the apiSchema metadata of the prim
   - remove_applied_schema: This node removes the applied API schema name from the apiSchema metadata of the prim


### Bugfix

## [1.0.0] - 2022-12-12

 - Initial release
- 2022-12-12

 - Initial release
