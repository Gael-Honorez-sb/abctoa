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
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcMaterial/OMaterial.h>
#include "ai.h"
#include "session/ArnoldSession.h"
#include "scene/MayaScene.h"
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
