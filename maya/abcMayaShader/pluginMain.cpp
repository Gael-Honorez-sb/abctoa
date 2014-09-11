/*Abc Shader Exporter
Copyright (c) 2014, Gaël Honorez, All rights reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library.*/


#include "abcMayaShaderNode.h"
#include "abcContainersExportCmd.h"
#include "abcCacheExportCmd.h"

#include <maya/MFnPlugin.h>

#include <maya/MIOStream.h>
#include <maya/MSceneMessage.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStringResource.h>
#include <maya/MStringResourceId.h>


#ifdef WIN32
#define EXTERN_DECL __declspec( dllexport )
#else
#define EXTERN_DECL extern
#endif



EXTERN_DECL MStatus initializePlugin( MObject obj );
EXTERN_DECL MStatus uninitializePlugin( MObject obj );


#define ARNOLD_CLASSIFY( classification ) (MString("rendernode/arnold/") + classification)
const MString ARNOLD_SWATCH("ArnoldRenderSwatch");
//
// static variables for holding the callback IDs
//
//MCallbackId openCallback;
//MCallbackId importCallback;
//MCallbackId referenceCallback;


MStatus initializePlugin( MObject obj )
//
//    Description:
//        this method is called when the plug-in is loaded into Maya.  It
//        registers all of the services that this plug-in provides with
//        Maya.
//
//    Arguments:
//        obj - a handle to the plug-in object (use MFnPlugin to access it)
//
{
    MStatus   status;
    MFnPlugin plugin( obj, "nozon", "2013", "Any");


    MString classification( "shader/surface/");

    //classification += MString(":") + ARNOLD_CLASSIFY(classification);
    classification += MString(":swatch/") + ARNOLD_SWATCH;

    status = plugin.registerNode( "abcMayaShader", abcMayaShader::id, abcMayaShader::creator,
                                  abcMayaShader::initialize,
                                  MPxNode::kDependNode, &classification);
    if (!status) {
        status.perror("registerNode");
        return status;
    }

    plugin.registerCommand( "abcContainersExport", abcContainersExportCmd::creator, abcContainersExportCmd::mySyntax);

    plugin.registerCommand( "abcCacheExport", abcCacheExportCmd::creator, abcCacheExportCmd::mySyntax);

    return status;
}

MStatus uninitializePlugin( MObject obj)
//
//    Description:
//        this method is called when the plug-in is unloaded from Maya. It
//        deregisters all of the services that it was providing.
//
//    Arguments:
//        obj - a handle to the plug-in object (use MFnPlugin to access it)
//
{
    MStatus   status;
    MFnPlugin plugin( obj );

    status = plugin.deregisterNode( abcMayaShader::id );
    if (!status) {
        status.perror("deregisterNode");
        return status;
    }

    status = plugin.deregisterCommand("abcContainersExport");
    if (!status) {
        status.perror("deregisterCommand");
        return status;
    }

    status = plugin.deregisterCommand("abcCacheExport");
    if (!status) {
        status.perror("deregisterCommand");
        return status;
    }
    return status;
}
