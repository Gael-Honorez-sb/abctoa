# A startup file to register the ArnoldOpenVDBVolume Op, if this is in 
# $KATANA_RESOURCES/Plugins, it will be exec'd automatically. If its
# in $KATANA_RESOURCES/Startup, it would need to be init.py, or called
# from and existing init.py script.

def registerJsonFile_In():
    """
    Registers the registerJsonFile_In Op using the NodeTypeBuilder.
    This is a helper class that takes care of registering a suitable
    node graph node type, and configuring it to call the supplied method
    in order to build the Op chain that is used to represent the
    node in the Op Graph.
    """

    from Katana import Nodes3DAPI
    from Katana import FnAttribute, FnGeolibServices

    def buildOpChain( node, interface ):
        """
        Configures the Ops to represent the current state of the node.
        The calling mechanism in Node3D takes are of ensuring that
        the underlying Op network is only updated if Op Args and types
        have changed.

        @type node: C{Nodes3DAPI.NodeTypeBuilder.SubdividedSpace}
        @type interface: C{Nodes3DAPI.NodeTypeBuilder.BuildChainInterface}
        @param node: The node to build the Op and OpArgs from (ie: an instance
        of our node).
        @param interface: The interface that will configure the Ops in the
        underlying Op network.
        """

        # Ensure the runtime doesn't error for us if there are no inputs on the
        # Node - as we want to allow it to exist in isolation. The default is 1
        interface.setMinRequiredInputs( 1 )

        # We first need the current time for parameter value queries
        frameTime = interface.getGraphState().getTime()

        # Get the parameters from our node
        celParam        = node.getParameter( "assetLocation" )
        filenameParam = node.getParameter( "filepath" )

        if not filenameParam :
            raise RuntimeError( "Missing node parameters, require "+
                    "'location' and 'filename'" )


        if not celParam:
            return # nothing to operate on




        # Build the Op args from our node
        argsGb = FnAttribute.GroupBuilder()
        argsGb.set( "assetLocation", FnAttribute.StringAttribute(celParam.getValue(frameTime)) )
        argsGb.set( "filepath", FnAttribute.StringAttribute(filenameParam.getValue(frameTime)) )

        interface.appendOp( "AttributeJsonFile_In", argsGb.build() )

        # We want to use the StaticSceneCreate Op to build the parent
        # hierarchy, so that our op only has to worry about generating its
        # children. Its args are somewhat complex, but fortunately, there
        # is a helper class that makes it all much easier.

        #sscb = FnGeolibServices.OpArgsBuilders.StaticSceneCreate()
        #sscb.addSubOpAtLocation( rootLocation, "ArnoldOpenVDBVolume", argsGb.build() )



        #interface.appendOp( "StaticSceneCreate", sscb.build() )


    # Here we need to define the parameters for the node, and register the op
    # chain creation callback function

    nodeBuilder = Nodes3DAPI.NodeTypeBuilder( "AttributeJsonFile_In" )

       # Merge with incoming scene, simply allow the node to have an input. Unless
    # you delete locations in your Op, any existing locations will pass-through.
    nodeBuilder.setInputPortNames( ("in",) )

    # If we wanted to merge with incoming scene, we could simply allow the
    # node to have an input. Unless you delete locations in your Op, any
    # existing locations will pass-through. It is encouraged though to avoid
    # long chains of Ops, as it makes multi-threading and caching less
    # efficient, so for 'Generator' Ops, no input is preferable.
    # nodeBuilder.setInputPortNames( ("in",) )

    # Parameters can be described by a group attribute
    paramGb = FnAttribute.GroupBuilder()
    paramGb.set( "assetLocation", FnAttribute.StringAttribute( "" ) ) 
    paramGb.set( "filepath", FnAttribute.StringAttribute("") )


    nodeBuilder.setParametersTemplateAttr( paramGb.build() )


    nodeBuilder.setHintsForNode( { 'help' : 'Read a Json file and assign Arnold attributes accordingly.' } )

    nodeBuilder.setHintsForParameter( "filepath",
                                      { 'widget' : 'fileInput',
                                        'help' : "Path to the JSON file" } )
    
    nodeBuilder.setHintsForParameter( "assetLocation",
                                      { 'widget' : 'scenegraphLocation',
                                        'help' : "Locations of geometry, material, or network material" } )


    # Register our Op build function
    nodeBuilder.setBuildOpChainFnc( buildOpChain )

    # Create the new Node3D type
    nodeBuilder.build()


registerJsonFile_In()
