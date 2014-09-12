/*Alembic Holder
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


#include "nozAlembicHolderNode.h"
#include "alembicHolderOverride.h"
#include "version.h"

#include "cmds/ABCHierarchy.h"
#include "cmds/ABCGetTags.h"
#include "json/json.h"

#include <maya/MFnPlugin.h>
#include <maya/MDrawContext.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MDrawRegistry.h>
#include <maya/MGlobal.h>



MString    drawDbClassification("drawdb/geometry/alembicHolder");
MString    drawRegistrantId("AlembicHolderPlugin");

#ifdef WIN32
#define EXTERN_DECL __declspec( dllexport )
#else
#define EXTERN_DECL extern
#endif

EXTERN_DECL MStatus initializePlugin( MObject obj );
EXTERN_DECL MStatus uninitializePlugin( MObject obj );


MStatus initializePlugin( MObject obj )
{
    MStatus   status;
    MFnPlugin plugin( obj, HOLDER_VENDOR, ARCH_VERSION, MAYA_VERSION);


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

    if(MGlobal::mayaState() == MGlobal::kInteractive)
    {
        status = MGlobal::executePythonCommand(MString("import alembicHolder.cmds.registerAlembicHolder;alembicHolder.cmds.registerAlembicHolder.registerAlembicHolder()"), true, false);
        if (!status) {
            status.perror("registerMenu");
            return status;
        }
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

    if(MGlobal::mayaState() == MGlobal::kInteractive)
    {
        status = MGlobal::executePythonCommand(MString("import alembicHolder.cmds.unregisterAlembicHolder;alembicHolder.cmds.unregisterAlembicHolder.unregisterAlembicHolder()"), true, false);
        if (!status) {
            status.perror("unregisterMenu");
            return status;
        }
    }

    return status;
}
