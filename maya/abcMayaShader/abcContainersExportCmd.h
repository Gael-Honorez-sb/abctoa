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


#ifndef ABC_CONTAINERS_EXPORT_CMD_H
#define ABC_CONTAINERS_EXPORT_CMD_H

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MObjectArray.h>
#include <maya/MPlugArray.h>

#include <maya/MItDag.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MFnContainerNode.h>
#include "abcMayaShaderNode.h"
#include <maya/MAnimControl.h>
#include <maya/MLightLinks.h>


#include <Alembic/Abc/All.h>
#include <Alembic/AbcMaterial/OMaterial.h>
#include "ai.h"
#include "session/ArnoldSession.h"
#include "utils/Universe.h"
#include "translators/NodeTranslator.h"

namespace Abc =  Alembic::Abc;
namespace Mat = Alembic::AbcMaterial;


class abcContainersExportCmd : public MPxCommand {
  public:
    abcContainersExportCmd() {};
    ~abcContainersExportCmd() {};

    virtual MStatus doIt( const MArgList &args );
    virtual MStatus undoIt();

    virtual MStatus redoIt();

    virtual bool isUndoable() const;

    virtual bool hasSyntax() const;

    static MSyntax mySyntax();

    static void* creator() {return new abcContainersExportCmd;};

    bool isHistoryOn() const;

    MString commandString() const;

    MStatus setHistoryOn( bool state );

    MStatus setCommandString( const MString & );

};

#endif //FX_MANAGER_CMD_H
