//
// Copyright (C) 2014 Nozon.
//
// File: pluginMain.cpp
//
// Author: Gaël Honorez
//

#include "nozAlembicHolderNode.h"
#include "alembicHolderOverride.h"

#include "cmds/ABCHierarchy.h"
#include "cmds/ABCGetTags.h"
#include "json/json.h"

#include <maya/MFnPlugin.h>
#include <maya/MDrawContext.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MDrawRegistry.h>

MString    drawDbClassification("drawdb/geometry/alembicHolder");
MString    drawRegistrantId("AlembicHolderPlugin");

MStatus initializePlugin( MObject obj )
{
    MStatus   status;
    MFnPlugin plugin( obj, "Nozon", "1.0", "Any");


    status = plugin.registerShape( "alembicHolder",
                nozAlembicHolder::id,
                &nozAlembicHolder::creator,
                &nozAlembicHolder::initialize,
                &CAlembicHolderUI::creator,
                &drawDbClassification);

    if (!status) {
        status.perror("registerNode");
        return status;
    }

    status = MHWRender::MDrawRegistry::registerDrawOverrideCreator
    (
                drawDbClassification,
                drawRegistrantId,
               AlembicHolderOverride::Creator
    );

    if (!status) {
        status.perror("registerNodeDrawOverride");
        return status;
    }

    status = plugin.registerCommand( "ABCHierarchy", ABCHierarchy::creator);
    if (!status) {
        status.perror("registerCommand");
        return status;
    }

    status = plugin.registerCommand( "ABCGetTags", ABCGetTags::creator);
    if (!status) {
        status.perror("registerCommand");
        return status;
    }

    return status;
}

MStatus uninitializePlugin( MObject obj)
{
    MStatus   status;
    MFnPlugin plugin( obj );

    status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(
        drawDbClassification,
        drawRegistrantId);
    if (!status) {
        status.perror("deregisterDrawOverrideCreator");
        return status;
    }

    status = plugin.deregisterNode( nozAlembicHolder::id );
    if (!status) {
        status.perror("deregisterNode");
        return status;
    }


    status = plugin.deregisterCommand( "ABCHierarchy" );
    if (!status) {
        status.perror("deregisterCommand");
        return status;
    }
    status = plugin.deregisterCommand( "ABCGetTags" );
    if (!status) {
        status.perror("deregisterCommand");
        return status;
    }

    return status;
}
