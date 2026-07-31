## [1.4.5] - 2026-07-30 (Bifrost 3.2)

### Build
   - BIFROST-14126 - Disable slow GraphExecutor and DynamicFileFormat unit tests in Valgrind Debug, and make the `reloadLibrary` unit test faster.

### Bugfix
  - BIFROST-14151 - Fix a GraphExecutor test.
  - BIFROST-88228 - Fix the reload library feature in Bifrost Dynamic File Format plugin by ensuring the settings attributes are composed unconditionally during the composition phase.

## [1.4.4] - 2026-07-08 (Bifrost 3.2)

### Build
  - BIFROST-13933 - Fix a warning caused by CMake 4.3.1.
  - BIFROST-13928 - Rename libraries in bifrost-usd-pack:
    - `BifrostUSD` -> `BifrostUsd`
    - `BifrostUSDMayaTranslation` -> `BifrostUsdMayaTranslation`
    - `BifrostUSDNodedefs` -> `BifrostUsdNodedefs`
    - `BifrostUSDTestUtils` -> `BifrostUsdTestUtils`
    - `BifrostUSDWatchpoint` -> `BifrostUsdWatchpoint`
  - BIFROST-88227 - Allow tests to find TBB from vanilla USD.

### Feature
  - BIFROST-13938 - Add `traverse_instances` input port to the _get_prim_children_ node.
  - BIFROST-13919 - Add `mayaUsdPlugInfo.json` so that Bifrost Dynamic Payload plugin is loaded by the Maya USD plugin.
  - BIFROST-14119 - Add overloads of _add_reference_prim_ and _add_payload_prim_ that accept a string layer identifier.
  - BIFROST-14108 - Add new _set_prim_metadata_by_dict_key_ node, and add string-array support to _set_prim_metadata_ and _get_prim_metadata_.
  - BIFROST-14120 - _set_prim_metadata_ and _get_prim_metadata_ are no longer marked as internal.
  - BIFROST-14087 - The UsdTranslator and DynamicFileFormat now use a single _objects_to_stage_ compound for both single-object and multiple-object output cases.
    - Rename `array_of_objects_to_stage` to `objects_to_stage`.
    - Remove the legacy `object_to_stage` function.
  - BIFROST-13584 - Refactor the Bifrost Dynamic File Format plugin to support a new namespace for its fields and attributes, with improved parsing, validation, and error reporting.
    - Add the new `bifrost:` namespace for all Dynamic File Format fields and attributes to avoid conflicts with other plugins and with native USD fields or attributes.
    - Group all known tokens (`bifrostCompound`, `bifrostGlobals`, `bifrostInputs`, `bifrostOutputs`, `bifrostSettings`) so future token additions can be handled generically.
  - BIFROST-13785 - In the Dynamic File Format plugin, remove the requirement to declare any metadata field before using it as a prim attribute. This applies to all supported metadata fields (compound name, globals, graph inputs, graph outputs and settings).
    - Improve error reporting when there are no specified outputs, when they produce no usable Stage, or when the Stage has no default prim.
    - Report type mismatch or unsupported type errors instead of warnings when reading attributes.
    - Move the `reloadLibrary()` call from the read phase to the composition phase so the library is reloaded before plugin-argument fields are composed. This properly handles cases where a compound is added or removed, or where its graph inputs or outputs change.
  - BIFROST-14150 - Add more detailed documentation for the Dynamic File Format in its README.

## [1.4.3] - 2026-04-22 (Bifrost 3.1)

### Feature
  - BIFROST-13540 - Compounds that are used in quick-create on array input ports now have explicit output types set instead of auto-outputs so that auto fan-in can work.
    The affected compounds are: _define_usd_attribute_, _define_usd_display_color_
  - BIFROST-13820 - Add new _get_variant_selection_ node.
  - BIFROST-13858 - Add new _get_prim_parent_ node.
  - BIFROST-13862 - Add new _get_time_sample_ node.
  - BIFROST-13581 - Add new Bifrost Dynamic Payload (experimental).
    - BIFROST-13582 - Polish initial implementation.
    - BIFROST-13804 - Add unit tests.

### Bugfix
  - BIFROST-13592 - In Bifrost Hydra, prevent the Bifrost graph from being re-executed every time a procedural object is selected in Maya Hydra by skipping the re-execution of the graph if no inputs have been changed since the last execution.
  - BIFROST-13506 - Fix intermittent Bifrost USD test failures caused by TBB shutdown.
    - Add a `TBBInitGuard` for proper initialization and termination of TBB when running gtests.
    - Unit tests now link with the TBB shared libraries bundled with Pixar USD instead of the TBB libraries bundled with Bifrost. This prevents intermittent gtest failures during TBB shutdown that were observed in some unit tests on Linux with USD 24.11 and OneTBB 2021.12.

## [1.4.2] - 2026-03-25 (Bifrost 3.0)

### Build
  - BIFROST-12728 – Add support for multiple USD versions for Maya 2026.3.
    - Introduce a new CMake variable, `MULTI_USD_VERSION_SUFFIX`, required to correctly set `PYTHONPATH` for MayaUSD and USD when running tests against a MayaUSD artifact that supports multiple USD versions.
    - When running tests in standalone mode, disable the default `usd_pack` and `usdMayaTranslations` libraries shipped with Bifrost, and instead use the nodedefs and translation table built from the current repository.
    - test_graphs.py changes:
      - On bifcmd test failures, dump the full JSON log file generated by bifcmd to help diagnose the root cause.
      - Replace some assertions with error logging to avoid early termination without sufficient diagnostic output.
      - Add additional logging to assist with debugging failures (`validate_and_display_config_files()`, `dump_path_env_var()`, `print_tree_recursive()`).
  - BIFROST-13183 – Use unique temporary subdirectories for some unit tests, allowing them to be run multiple times in parallel.
  - BIFROST-13367 – Update to OpenUSD 25.11

### Feature
  - BIFROST-13178 – Update USD Watchpoint to support all Data Browser features, including layout, sorting, and filtering.
  - BIFROST-12967 – Update Bifrost Hydra to replace the use of `GraphContainer` and `Job` with `Amino::Executable`.
  - BIFROST-12181 – Mark the `open_layer` node as internal so it is hidden from the node browser.

### Bugfix
  - BIFROST-12119 – USD PointInstancer: Fix the `protoIndices` attribute not matching the Bifrost `point_instance_id` when no prototypes are defined.
    - Preserve the Point Instance ID.
    - Copy Point Color data to USD.
  - BIFROST-13511 – Use the new UFE `PathString` to fix an error when creating a USD material through the `bifrost_usd.component_creator` Python module.
  - BIFROST-13517 - Fix OSX conversion issue with `size_t` type.

## [1.4.1] - 2025-10-15 (Bifrost 2.15)

### Build
 - BIFROST-12652 - Relocate Bifrost USD pack's graphs from `/resources/jsonLibs/graphs` to `/resources/graphs` in the installation folder.
 - BIFROST-12791 - Add `BIFUSD_MAYA_TRANSLATIONS_NAME` variable to override usdMayaTranslations extension loaded from Bifrost install.
 - BIFROST-11881 - Update to OpenUSD 25.5

### Bugfix
 - BIFROST-12866 - Fix cpp2json error when setting MaterialPurpose default value.

## [1.4.0] - 2025-04-30 (Bifrost 2.14)

### Bugfix
 - BIFROST-11582 - Bifrost USD examples plugin menu duplication.
 - BIFROST-11799 - Fix and improve _duplicate_prim_definition_ node.
    Using an array of float3 for position, rotation or scale was giving unexpected result in previous version.
    You can now simply use Bifrost points to set the copy transforms.

   - Backward compatibility break:

      Using array of float3 is not supported anymore

   - New input parameters:

      If a point based Bifrost geometry is connected to the _from_points_ input, the prim is duplicated using the _point_position_,
      _point_orientation_ and _point_scale_ geo properties of the connected geometry.

      If _ignore_source_transform_ is enabled, the source prim definition transform is not applied to the duplicated prim.

      If _uniform_rotation_ is enabled, the rotation is applied uniformly to duplicated prim(s).
      If disabled, the rotation is applied relative to the previous prim.

 - BIFROST-11363 - Undoing the creation of a component material library crashes Maya.


## [1.3.3] - 2025-03-26 (Bifrost 2.13)

### Build
 - BIFROST-11259 - update to OpenUSD 24.11

### Feature
  - BIFROST-11322 - Add Lookdev workflow
    - New runtime commands to:
         - create_maya_usd_material_library_cmd
         - create_lookdev_stage_from_layers_cmd
         - open_maya_usd_material_library_cmd
         - usd_attribute_quick_look_from_selection_cmd
         - prim_selection_to_string_array_compound_cmd
         - remove_prim_selection_from_string_to_array_compound_cmd
         - select_prims_from_selected_node_cmd
    - New compounds:
         - apply_look_v2 (for 'iterate_on_model_variants' case)
         - apply_usd_material_bindings
         - resolve_bindings
         - create_lookdev_workflow_stage
         - usd_collection_schema
         - usd_string_paths_to_array
    - New UI dialogs to create or open mayaUsdProxyShape from a USD file:

### Bugfix
 - BIFROST-10481 - Fix PointInstancer prototype ordering when using the _define_usd_point_instancer_
 - BIFROST-10481 - The default material option is now disable by default in the _define_usd_point_instancer_
 - BIFROST-11306 - Fix the model exporter for mayaUSD plugin >= 0.30

## [1.3.1] - 2024-10-29 (Bifrost 2.12)

### Build
 - BIFROST-10711 - Prevent older boost from using deprecated templates
 - BIFROST-10578 - Fix sanitizer errors

### Feature
 - BIFROST-10691 - Set target layer using layer or display name instead of index

   - Add layer input to add_to_stage and add_to_stage_in_variant nodes
   - Add layer_display_name input to set_edit_layer node
   - Add USD::Layer::get_layer_display_name Operator

### Bugfix
 - BIFROST-10745 - Fix Bifrost Browser scene "create_point_instancer_from_usd_file_example.ma".

    Since the _scene_info_ node does not prepend a '/' anymore for the scene directory,
    a '/' separator has been added to build the path of the referenced USD file.

 - BIFROST-11053 - Fix error message when opening Data Browser for the first time

## [1.3.0] - 2024-07-29 (Bifrost 2.11)

### Build
 - BIFROST-10233 - Update Bifrost Hydra to support USD 24.05


### Feature
 - BIFROST-10457 - Add _get_prim_attribute_connections_ operator
 - BIFROST-9995 - Add USD Skeleton graphs to the Bifrost Graph Browser
 - BIFROST-10104 - Add new USD icons
 - BIFROST-10105 - BifrostUSDExamples updates

    Create prim type from Maya USD attribute "USD_typeName"
    If the Maya attribute "USD_typeName" is present on the DAG node, it will be used to set the prim type in the imported selection.
 - BIFROST-10105 - Create variants from Maya selection

    Add "Create New Stage with Variants from Maya Selection" menu in Bifrost USD > Create.
    Add "Import Maya Variants to Stage" menu in Bifrost USD > Modify.
 - BIFROST-10016 - Expose applied_schema_names on the "define prim" nodes
 - BIFROST-10356 - New PointInstancer nodes and menu

    New nodes:
      - _delete_point_instances_
      - _get_usd_point_instancer_attributes_
      - _replace_point_instancer_proto_
      - _set_point_instances_invisible_
      - _usd_point_instancer_scope_

    New menus:
      - Bifrost USD > Modify > Hide Selected PointInstances
      - Bifrost USD > Modify > Add Point Instancer Scope

    Move _define_usd_point_instancer_ into _USD::PointInstancer_ namespace.

- Add _create_mesh_from_usd_geom_subset_ compound (Experimental)


### Bugfix
 - EMSUSD-1195 - Disable UFE observer if Maya API version is less than 2025.2 to avoid crash in the BifrostUsdExamples plugin.


## [1.2.3] - 2024-04-17 (Bifrost 2.10)

### Build
 - BIFROST-9342 - Update to Bifrost 2.10 SDK. BifrostHd::Container is now managed by Workspace.


### Feature
 - BIFROST-9565 - Add nested Variant Sets support.

    Allow to create a VariantSet inside an existing VariantSet on its prim.
    It changes the _set_variant_selection_ operator behavior.
    Previously the "clear" parameter was used to mimic a clear variant set.
    In the new implementation, the "clear" parameter clears the current variant
    selection and adds a new one in current VariantSet.
    To only clear the variant selection without setting a new one, use the
    _clear_variant_selection_ operator.

    This change allows users to create a variant set and a variant by just specifying it in the Variant Selection section of the _define_usd_prim_.
    Creating a _define_usd_variant_set_ on the parent prim is needed only to set the variant selection in a particular variant set.

 - BIFROST-9637 - Add clear_variant_selection operator.
 - BIFROST-9638 - Add "Variant Selection" group in _define_usd_curves_ and _define_usd_point_instancer_ compounds.
 - BIFROST-9611 - Add _get_prim_kind_ operator.
 - BIFROST-8154 - Add _get_prim_attribute_type_ operator.
 - BIFROST-8099 - Add _get_applied_schemas_ operator.
 - BIFROST-9186 - Add _applied_schema_names_ parameter to the _define_usd_prim_ compound.
    You can pass coma-separated names of USD Applied API Schemas.
 - BIFROST-9674 - Add _define_usd_geom_subset_ compound.
 - BIFROST-9686 - Add UI logic on _define_usd_mesh_ to disable "Subdiv" when "Normal Per Vertex" is enabled.
 - BIFROST-9673 - Add USD graphs and scenes to Bifrost Browser.
 - BIFROST-9770 - Add the "Bifrost USD Examples" plugin for Maya showing how Bifrost USD can be used to create high level workflows like:
     - Creating a Bifrost graph dedicated to USD workflows.
     - Importing a Maya hierachy in a Bifrost Graph as USD prims.
     - Creating a USD variant from a Maya hierarchy.
     - Creating a USD Model Component with variants from multiple USD files.

    In the install folder of the project, you will find the _bifrostUSDExamples.mod_ file in "examples/maya_plugin".
    You will need to add its directory path to your _MAYA_MODULE_PATH_ environment variable. An easy way is by editing the Maya.env file in your user preferences as bellow:

    `MAYA_MODULE_PATH=<your Bifrost USD install path>\examples\maya_plugin`


### Bugfix
 - BIFROST-9608 - Fix wrong parameters layout order in the Variant Selection group.

    The _variant_set_name_ and _variant_name_ parameters order is changed in the _define_usd_prim_ and _define_usd_mesh_ compounds.

 - BIFROST-9056 - Collapse ports grouping on the _define_usd_prim_attribute_ compound.
 - BIFROST-9334 - Remove compound duplicate in _save_usd_stage.json_ file.


## [1.2.2] - 2024-03-27 (Bifrost 2.9)

### Build

 - BIFROST-9332 - Update to Bifrost 2.9 SDK
 - BIFROST-8981 - Update to USD 23.11
 - BIFROST-9147 - Make C++17 default

### Feature

 - BIFROST-9354 - USD File Format Option

    Add parameter to set a the USD layer format to ASCII or binary. Useful to save in human readable format and still keep the .usd extension.

 -  BIFROST-9334 - Add _get_edit_layer_ operator

    This node returns the stage's EditTarget layer.


### Bugfix

 - BIFROST 9428 - Fix scalar attribute creation regression when _using add_to_stage_ compound
 - BIFROST-9334 - Fix save_usd_stage compound errors when current edit target is not the root layer
    - Update the _save_usd_stage_ compound. It is now setting the target layer to the root layer before saving and then restore the current target (using the new get_edit_layer operator).


## [1.2.1] - 2023-11-15 (Bifrost 2.8)

### Build

 - BIFROST-9103 - Replace pxr by PXR_NS
 - BIFROST-9093 - Uses cpp2json executable instead of the deprecated amino_cpp2json_foreach one.
 - BIFROST-8182 - Remove -Wno-unused-macros on Windows targets
 - BIFROST-8182 - Clang-tidy support: Add the cmake target "bifrost_usd_clang_tidy" to the build.

### Feature

 - BIFROST-8077 - Add support for half and matrix attribute types

 - BIFROST-8574 - Add anchor_path parameter on add_reference_prim and add_payload_prim

   By specifying an "anchor_path", the identifier of the referenced layer will not include such anchor path in the reference list.

 - BIFROST-8424 - Add slider and color picker on ops

 - BIFROST-9008 - Add set_layer_permission node

   To be used for the very specific scenario when you need to reference a layer
   that needs to be modified by an other runtime than Bifrost.

   For example, if a stage generated by Bifrost references a "file based" layer
   storing some USD materials who should be authored in the LookdevX Editor,
   it would not work by default. This is because in order to keep the referenced layer
   "file based", the "read only" mode would need to be enabled in the open_layer node.
   If not in read only mode, the opened layer ("file based" layer) would be automatically
   copied into an anonymous layer by Bifrost USD (to avoid side effects) and so would not
   be editable  in LookdevX in a persistent way (as the layer identifier would change at
   every execution of the graph).

   The node graph to open a layer in Bifrost and let it be editable outside should look as following:
   open_layer (with read_only "on") -> set_layer_permission (with read_only "off") -> add_reference_prim

   The open_layer node will call the BifrostUsd::Layer constructor that is "opening or finding" the USD layer
   using the file path as an identifier (and will not create an anonymous layer, because of the read only mode).
   The set_layer_permission will allow the layer to be editable and will not output an anonymous layer.

 - BIFROST-8788 - Use for_each in define_usd_mesh

 - BIFROST-7769 - use getEnv*() functions from public Executor SDK

 - BIFROST-8199 - Use ConfigEnv instead of Object to get config from environment

 - BIFROST-3401 - Use new API for FileUtils::getRelativePath.

### Bugfix

 - BIFROST-9067 - Fix connection order when creating an add_to_stage node 'on the fly'

 - BIFROST-8826 - fix random test failures on Windows

   - Layer::exportToFile() now uses SdfLayer::New() instead of CreateNew().
   This prevents a temporary and default file to be created on the disk
   before the final exported file is actually written. On Windows, when
   Pixar USD is writing an initial file and almost immediately attempts
   to replace its content by renaming another file to such initial file,
   an "access denied" error can occasionally occur; eliminating the
   temporary file on the first place by not calling CreateNew() avoids
   this error.
   - Each unit test file now outputs its exported files into its own unique
   folder, avoiding completely the possibility that two concurrent tests
   from different test files would attempt to export to the same file.
   These unique output folders are deleted as a first phase for each test
   file, allowing tests to assume and check that an output file is not
   already on disk before it is being exported.
   - Some tests were previously just lucky to succeed, as they were
   exporting a root layer and sublayer to disk, but the sublayer was
   actually exported into the default output folder, not into the expected
   folder, and since the sublayer file was already saved to disk by
   another test case, the test assumed that everything went fine. Such
   test cases are now fixed or were removed (if not fixable).
   - Tests do not export initial files first, then replacing these by final
   file content, and finally checking the final file for some expected
   content. This was also occasionally causing "access denied" errors, as
   described above.
   - add README.md in test folder


## [1.2.0] - 2023-05-12 (Bifrost 2.7)

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
    - use the layer's save file path (m_filePath) instead of the sdfLayerIdentifier in the recursive call to Layer::exportToFile().
    - add new more complete unit test for export_layer_to_file() that covers multiple cases for relative and absolute paths to sublayers.
    - remove call to changeDir() that has side effect and is not required anymore for unit tests.


## [1.1.0] - 2023-03-29 (Bifrost 2.7)

### Build

- BIFROST-8068 - do not run test_graphs.py when system processor is not in target arch.

- BIFROST-8073 - update calls to xcrun to support latest xcode/macos

- BIFROST-8065 - add support to build for desired binary architecture on OSX

- BIFROST-7845 - Add test loading all compounds

### Feature

- BIFROST-7955 - Add applied schema nodes
    - add_applied_schema: This node adds the applied API schema name to the apiSchema metadata of the prim
    - remove_applied_schema: This node removes the applied API schema name from the apiSchema metadata of the prim


## [1.0.0] - 2022-12-12 (Bifrost 2.6)

 - Initial release