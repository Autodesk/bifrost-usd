# `create_usd_component`

A _Component_ is the smallest reference-able USD Model. It can be referenced "as is" in an other stage, or can be combined (with other components) into an _Assembly_ Model that can also be referenced in an other stage.

## This compound is providing following features.

- Create a default prim using the _Model Name_
- Create a _payload_ layer to load the asset only if needed.
- (optional) Create a _Model_ VariantSet to select different modeling variants from the default prim.
- (optional) Create a _Look_ VariantSet to select different look variants from the default prim.
- Compute the _extentsHint_ attributes to display the component as a bounding box.

### Automatic creation of model variants.
If more than one _define_usd_model_variant_ compound is connected to the _model_variants_ input, a VariantSet called _Model_ (by default) will be created on the default prim.
To quickly create such compound, right-click on the _model_variants_ input and choose "Create Node > USD::Model::define_usd_model_variant".


### Automatic creation of look variants.
A look variant stores material bindings and rendering primvars.
If more than one _define_usd_look_variant_ compound is connected to the _look_variants_ input of a _define_usd_model_variant_, a VariantSet called _Look_ (by default) will be created on the default prim.
To quickly create such compound, right-click on the _look_variants_ input and choose "Create Node > USD::Model::define_usd_look_variant".

## Directory and files structure.

For a given USD asset called _Spaceship_, it will looks as following:
```
<your project path>/Spaceship
    Spaceship.usd          <- file to choose when referencing the component.
    payload.usd            <- should not be referenced outside of this directory.
    geo                    <- the geometries directories. This is where you export modelings files.
    mtl                    <- the material library directory. This is where you save your materials.
    bnd                    <- the material bindings and render primvar are stored in files here.
```

If the _version_ parameter is set, it will create additional directories as following:
```
<your project path>/Spaceship
    v001
        Spaceship.usd
        ...
    v002
    ...
```

## Inputs

### `save_model`
Save all the USD files of the component.

if enabled, a cube primitive is shown in the viewport and a prim named "SAVE_IS_ENABLED" is added to the stage.
This is to avoid to forget to disable it when authoring the component, in other word, you should not keep `save_model` "on" all the time.

### `name`
The name of the asset. It must match the name of the directory storing the component files.
For example: `<your project path>/Spaceship`

### `version`
(optional) The name of a sub-directory inside the component directory.

### `model_variants`
Variants to add to the model's VariantSet. See also `default_model_variant` and `look_variant_set_name`.

### `meters_per_unit`
Value of the stage's metadata _meterPerUnit_.

### `enable_relative_path`
Set layer path (of the sublayers/references/payloads) relative to the component layer path.

### `search_mode`
Set the parent directory of the _Component_ directory.

- _Relative to Project_: The host project directory.
- _Relative to Scene_: The host scene directory.
- _Custom_: User defined path.

### `sub_directory`
(optional) You can add an extra directory between the root directory and the component directory.
For example: `<host project dir>/props/<component name>`

### `geometry_scope_name`
The name of the prim of type _Scope_ under which are located the geometries prims.

### `model_variant_set_name`
The VariantSet name for the modeling variants.

### `default_model_variant`
The name of the selected variant in the _Model_ VariantSet.

### `default_look_variant`
The name of the selected variant in the _Look_ VariantSet.

### `material_scope_name`
The name of the prim of type _Scope_ under which are located the materials prims.

### `look_variant_set_name`
The VariantSet name for the look variants.

### `material_library_file`
Path to a USD file storing materials under the default prim _/mtl_.
Such file should be saved in the "\<component name\>/mtl" directory to keep the component self contained.

## Outputs

### `stage`

The component's stage.
