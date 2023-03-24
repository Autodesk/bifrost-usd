## [1.2.0] - xxxx-xx-xx

### Build

 - BIFROST-8086 - fix unit test errors

### Feature

 - BIFROST-6771 - Slider and color picker assignments

 - BIFROST-8319 - Add get_authored_attribute_names node

 - BIFROST-8100 - Add node to get all prim attributes names

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
