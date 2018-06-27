/*Alembic Holder
Copyright (c) 2014, Ga�l Honorez, All rights reserved.
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
#include "SubSceneOverride.h"
#include "version.h"

#include "cmds/ABCGetTags.h"
#include "json/json.h"

#include <maya/MFnPlugin.h>
#include <maya/MDrawContext.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MDrawRegistry.h>
#include <maya/MGlobal.h>

#include <maya/MStringResource.h>
#include <maya/MStringResourceId.h>


#define kPluginId  "alembicHolder"
#define kSelectionMenuItemLabel MStringResourceId( kPluginId, "kSelectionMenuItemLabel", "Alembic Holder")
#define kOutlinerMenuItemLabel MStringResourceId( kPluginId, "kOutlinerMenuItemLabel", "Alembic Holder")


MString    drawDbClassification("drawdb/subscene/alembicHolder");
MString    drawRegistrantId("alembicHolder");

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else 
#undef DLLEXPORT
#define DLLEXPORT __attribute__ ((visibility("default")))
#endif


using namespace AlembicHolder;

DLLEXPORT MStatus initializePlugin( MObject obj )
{
    MStatus   status;
    MFnPlugin plugin( obj, HOLDER_VENDOR, ARCH_VERSION, H_TOSTRING(MAYA_VERSION));


    status = plugin.registerShape( kPluginId,
                nozAlembicHolder::id,
                &nozAlembicHolder::creator,
                &nozAlembicHolder::initialize,
                &CAlembicHolderUI::creator,
                &drawDbClassification);

    if (!status) {
        status.perror("registerNode");
        return status;
    }

    status = MHWRender::MDrawRegistry::registerSubSceneOverrideCreator(
        drawDbClassification, drawRegistrantId, AlembicHolder::SubSceneOverride::creator
    );
    if (!status) {
        status.perror("registerSubSceneOverride");
        return status;
    }

    status = plugin.registerCommand( "ABCGetTags", ABCGetTags::creator);
    if (!status) {
        status.perror("registerCommand");
        return status;
    }

    if(MGlobal::mayaState() == MGlobal::kInteractive)
    {
        int polyMeshProirity = MSelectionMask::getSelectionTypePriority( "polymesh");
        bool flag = MSelectionMask::registerSelectionType( "alembicHolder", polyMeshProirity);
        if (!flag) {
            status.perror("registerSelectionType");
            return MS::kFailure;
        }

        MStatus stat;
        MString selectionGeomMenuItemLabel =
            MStringResource::getString(kSelectionMenuItemLabel, stat);

        MString registerMenuItemCMD = "addSelectTypeItem(\"Surface\",\"alembicHolder\",\"";
        registerMenuItemCMD += selectionGeomMenuItemLabel;
        registerMenuItemCMD += "\")";

        status = MGlobal::executeCommand(registerMenuItemCMD);
        if (!status) {
            status.perror("addSelectTypeItem");
            return status;
        }

        MString outlinerGeomMenuItemLabel = MStringResource::getString(kOutlinerMenuItemLabel, stat);
        MString registerCustomFilterCMD = "addCustomOutlinerFilter(\"alembicHolder\",\"CustomAlembicHolderFilter\",\"";
        registerCustomFilterCMD += outlinerGeomMenuItemLabel;
        registerCustomFilterCMD += "\",\"DefaultSubdivObjectsFilter\")";

        status = MGlobal::executeCommand(registerCustomFilterCMD);
        if (!status) {
            status.perror("addCustomOutlinerFilter");
            return status;
        }



        status = MGlobal::executePythonCommandOnIdle(MString("import alembicHolder.cmds.registerAlembicHolder;alembicHolder.cmds.registerAlembicHolder.registerAlembicHolder()"), false);
        if (!status) 
		{
            status.perror("registerMenu");
            return status;
        }
    }

    SubSceneOverride::initializeShaderTemplates();

    return status;
}

DLLEXPORT MStatus uninitializePlugin( MObject obj)
{
    MStatus   status;
    MFnPlugin plugin( obj );

    SubSceneOverride::releaseShaderTemplates();

    status = MHWRender::MDrawRegistry::deregisterSubSceneOverrideCreator(
        drawDbClassification,
        drawRegistrantId);
    if (!status) {
        status.perror("deregisterSubSceneOverrideCreator");
        return status;
    }

    status = plugin.deregisterNode( nozAlembicHolder::id );
    if (!status) {
        status.perror("deregisterNode");
        return status;
    }

    status = plugin.deregisterCommand( "ABCGetTags" );
    if (!status) {
        status.perror("deregisterCommand");
        return status;
    }

    if(MGlobal::mayaState() == MGlobal::kInteractive)
    {

        MString unregisterCustomFilterCMD = "deleteCustomOutlinerFilter(\"CustomAlembicHolderFilter\")";
        status = MGlobal::executeCommand(unregisterCustomFilterCMD);
        if (!status) {
            status.perror("deleteCustomOutlinerFilter");
            return status;
        }

        MString unregisterMenuItemCMD = "deleteSelectTypeItem(\"Surface\",\"alembicHolder\")";
        status = MGlobal::executeCommand(unregisterMenuItemCMD);
        if (!status) {
            status.perror("deleteSelectTypeItem");
            return status;
        }

        bool flag = MSelectionMask::deregisterSelectionType("alembicHolder");
        if (!flag) {
            status.perror("deregisterSelectionType");
            return MS::kFailure;
        }


        status = MGlobal::executePythonCommandOnIdle(MString("import alembicHolder.cmds.unregisterAlembicHolder;alembicHolder.cmds.unregisterAlembicHolder.unregisterAlembicHolder()"), false);
        if (!status) {
            status.perror("unregisterMenu");
            return status;
        }
    }

    return status;
}
