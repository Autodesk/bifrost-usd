## BifrostUsd Dynamic File Format Plugin

# Overview

The *BifrostUsd DynamicFileFormat* is a Dynamic File Format plugin for OpenUSD that allows users to create USD scene data procedurally using Bifrost Graphs. In combination with the Open USD Payload system, that allows for the generation of scene data dynamically within the context of a prim.

The combination of Dynamic File Format and Payload is called Dynamic Payload in OpenUSD.

 - [Dynamic File Format](https://openusd.org/docs/api/_usd__page__dynamic_file_format.html)
 - [Payload](https://openusd.org/dev/api/class_usd_payloads.html#details)


# Key Features of the BifrostUsd Dynamic Payload:

A Bifrost Graph can be used as a Dynamic Payload to generate USD scene data dynamically.
The graph takes arguments from the Dynamic File Format plugin to procedurally generate a USD layer.
A special argument is used to store the name of the compound definition that will be used as a graph.
Other arguments are used to set options, graph inputs and global variables.

One of the main differentiator of the BifrostUsd Dynamic Payload compared to others is that it is creating USD data procedurally directly. You can use all the Bifrost USD nodes to build and edit prims, attributes, etc. directly within the procedural layer when other systems are translating from their data model to USD data model.
You can also use Bifrost graphs that are not using any Biforst USD nodes at all, for example to create geometry data procedurally using Bifrost geometry nodes. The Bifrost Dynamic File Format will automatically translate such geometry to matching USD data type.


It allows to create an infinite variety of dynamic assets that can be instantiated in a USD scene.
Since the result is a USD layer that is composed with other layers of the stage, that means that user can use standard USD worfklow to override the generated data.
