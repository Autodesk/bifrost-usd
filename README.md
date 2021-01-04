Bifrost USD
==========

A library of nodes to create USD-based scene graphs in Bifrost.


## About the Project

This project uses the [Bifrost SDK](https://help.autodesk.com/view/BIFROST/ENU/?guid=Bifrost_DevHelp_BifrostSDKAminoAPI_html) to implement the USD nodes to 
manipulate data types like `layer`, `stage`, `prim` and `attribute` and leverage the [Bifrost Graph Editor](https://help.autodesk.com/view/BIFROST/ENU/?guid=Bifrost_Common_working_in_bifrost_html) to create and author USD scene graphs procedurally.

For Maya users, the project also includes a Maya plugin allowing to visualize USD stages created in Bifrost using the Maya USD proxy shape.

## Building

You must provide the following variables to the cmake command:

* BIFUSD_PACKAGE_NAME : By default Bifrost will load the USD Pack, named `usd_pack`, from its installation directory. You must use a different name for your custom build of the pack.
* CMAKE_BUILD_TYPE : Debug, Release, RelWithDebInfo
* CMAKE_INSTALL_PREFIX : Where you want to install the bifrost-usd-pack library
* BIFROST_LOCATION : The full path to the root directory of your Bifrost installation that contains, among others, the `sdk` subdirectory. The USD Pack will be built against this `sdk` subdirectory of your Bifrost installation.
* USD_LOCATION : The full path to the root directory of your USD library installation that contains, among others, the `cmake`, `include`, `lib`, `plugin` and `share` subdirectories.
* MAYA_RUNTIME_LOCATION : (optional) The full path to the root directory of your Maya development installation that contains, among others, the `bin`, `include` and `lib` subdirectories.
* BIFUSD_OSX_ACTIVE_SDK : (optional) Can be set to choose a specific macOS SDK. If not set, the currently available SDK will be used.
* BIFUSD_OSX_MIN_OS : (optional) Can be set to choose the minimum macOS deployement target. If not set, macOS 11.0 will be used.

__Python 3 version is assumed.__

__If your build uses MAYA_RUNTIME_LOCATION, you will need to use the same USD version than MayaUSD plugin.__


cmake configure example:

Note: we use the Ninja generator which is a single configuration generator.

```
 cmake -G Ninja -S <this project path> -B <build directory path> \
            -DCMAKE_MAKE_PROGRAM=<ninja executable pathname>
            -DBIFUSD_PACKAGE_NAME="studioname_usd_pack" \
            -DCMAKE_BUILD_TYPE="RelWithDebInfo" \
            -DCMAKE_INSTALL_PREFIX=<install directory path> \
            -DBIFROST_LOCATION=<bifrost path> \
            -DUSD_LOCATION="<library root path>/usd/20.11" \
            -DMAYA_RUNTIME_LOCATION=<Maya install path>
```

cmake build and install example:
```
cmake --build <build directory path> --target install
```

To build doxygen doc (require doxygen 1.8.14):
```
cmake --build <build directory path> --target usd_pack_apidoc
```

## Testing

```
cd <build directory path>/test
rm `find . -name "*.usd"`
ctest . -V
```

## Versioning
The USD Pack version is set in the cmake/version.info file. Such version number is prepended to your USD Pack name.

## Loading the USD Pack in Bifrost Extension for Maya
You will need to disable the USD Pack shipped with Bifrost by setting the BIFROST_DISABLE_PACKS environment variable to "usd_pack" (`export BIFROST_DISABLE_PACKS=usd_pack`).
Then you will need to set `BIFROST_LIB_CONFIG_FILES` to `<your install path>/<BIFUSD_PACKAGE_NAME>-1.0.0/plugin_config.json`
in order to get the USD nodes from your USD Pack build available from the tab menu in the Bifrost Graph Editor.

## If you did not build with -DMAYA_RUNTIME_LOCATION
If you built without `-DMAYA_RUNTIME_LOCATION` then you can load the USD nodes in a Bifrost environment just by setting `BIFROST_LIB_CONFIG_FILES` to `<your install path>/usd_pack-1.0.0/usd_pack/usd_pack_config.json`.
