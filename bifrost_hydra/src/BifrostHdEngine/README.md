# Bifrost Hydra Engine - Executing Bifrost at Hydra Render time

The BifrostHdEngine library is responsible for:
1. Loading specific definitions (the jsons files describing nodes, compounds, types, etc.) into the Bifrost runtime
2. Loading a graph (a graph can be a Bifrost compound or a Bifrost graph)
3. Setting the graph inputs using USD primvars matching their names
4. Executing the graph
5. Accessing to the result, that is the output(s) of the graph

## Engine

The _BifrostHd::Engine_ is instanciated by the _Bifrost Hydra Generative Procedural_.
Its purpose is to simplify the setup of Bifrost at render time, hidding configuration steps, loading of the graph, setting of the inputs, etc.

It is providing four methods:
1. _setInputScene(PXR_NS::HdSceneIndexBaseRefPtr inputScene)_
2. _setInputs(PXR_NS::HdSceneIndexPrim const& prim)_ // the Hydra Generative Procedural prim.
3. _execute(const double frame)_
4. _output()_

## Container

The _BifrostHd::Container_ is the entry point. It is a specialized version of the _BifrostBoardContainer_. See _\<Bifrost install\>/plug-ins/bifrost/sdk/include/bifrostboard_executor/BifrostBoardContainer.h_ for more info.

 _BifrostHd::Container_ implements a very limited set of functionality at the moment since we only need to load a graph, set its inputs and execute at Hydra render time.
So at the moment it does:

1. _loadGraph_  : Load a graph into the container
2. _updateJob_  : Create the BifrostBoardJob and attach the loaded graph to it
3. _executeJob_ : Let the BifrostBoardJob execute the loaded graph

For info, the _BifrostBoardContainer_ got a richer API than that for applications who would like to allow authoring of the graph.

## Parameters

The _BifrostHd::Parameters_ stores the _Pixar VtValues_ comming from the Hydra Generative Procedural of type _BifrostGraph_ that are:
1. The name of the Bifrost graph
2. The primvars used to the set the graph inputs
3. The Bifrost output of the graph you want to render in Hydra

## JobTranslationData

The _BifrostHd::JobTranslationData_ is an object that is used by the _BifrostHd::Container_ to pass the time and parameters
to Bifrost while executing a graph.

## Requirement

The _BifrostHd::Requirement_ describes the name, direction, type and port type for a given input or output of the graph.
It is created by the _BifrostHd::Container_. It is used internally to cache the input value.

## Runtime

The _BifrostHd::Runtime_ is responsible for loading the definition files. It is finding so called "json config files" using the
environment variable _BIFROST_LIB_CONFIG_FILES_ that can points to several json files at differents locations.
For example, in our tests, we load the following json config file _${BIFROST_LOCATION}/resources/standalone_config.json_
and _bifrost-usd/test/BifrostHydra/test_bif_geo_compounds_config.json_ that is loading
the usual Bifrost nodes and compounds plus extra compounds only used by the tests.

## ValueTranslationData

The _BifrostHd::ValueTranslationData_ is used by the translation system to convert a Pixar _VtValue_ to an _Amino::Value_
(a Bifrost graph set its inputs using Amino Values).

## TypeTranslation

At graph execution, for each input values, the _BifrostHd::TypeTranslation_ will call _convertValueFromHost_ that will use a _BifrostHd::ValueTranslationData_ to do the conversion.
For each output values, it will call _convertValueToHost_ to set the output of the graph. Note that at the moment, the output is a Bifrost object and the transaltion to an Hydra prim is done in the _Bifrost Hydra Generative Procedural_. Such design could change since it could convert the output value to an Hydra Data Source Container instead.
