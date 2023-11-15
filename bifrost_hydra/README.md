
## Bifrost Hydra (Experimental)
Requires a version of the Bifrost SDK that is not released yet.
Register a new Hydra Generative Procedural plugin capable of running a Bifrost Graph that will output
Bifrost geometries to the Hydra Scene Index without using any scene delegate.
In your Hydra based application (usdview for example), the following variables are needed:
```
export BIFROST_LOCATION=<bifrost path>
export BIFROST_LIB_CONFIG_FILES=<install directory path>/bifrost_hydra_translation.json
export HD_ENABLE_SCENE_INDEX_EMULATION=1
export USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=1
export HDGP_INCLUDE_DEFAULT_RESOLVER=1
```

Here is a USD file example showing how to setup the Generative Procedural for Bifrost.
```
#usda 1.0
(
def Xform "Mia"
{
    def "Head" (
        prepend references = @mia_young.usd@</head>
    )
    {
    }

    def GenerativeProcedural "BifrostGraph" (
        prepend apiSchemas = ["HydraGenerativeProceduralAPI"]
    )
    {
        # The Generative Procedural Plugin name
        token primvars:hdGp:proceduralType = "BifrostGraph"

        # The full name of the Bifrost compound or graph to execute in Hydra
        token primvars:bifrost:graph = "Groom::create_clumpy_hairs"

        # The inputs parameters must use the same names as the Bifrost
        # compound input ports with an additional "primvars:" prefix
        prepend rel primvars:mesh = </Mia/Head/scalp>
        float primvars:hair_count = 9000000
        float primvars:curl_frequency = 6.5

        # The output of the Bifrost compound you want to render in Hydra
        string primvars:bifrost:output = "hairs"
    }
}
```
There are several USD files in `test/BifrostHydra/resources` following such setup.
