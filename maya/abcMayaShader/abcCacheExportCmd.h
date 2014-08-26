#ifndef ABC_CACHE_EXPORT_CMD_H
#define ABC_CACHE_EXPORT_CMD_H

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MString.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MPlugArray.h>

class abcCacheExportCmd : public MPxCommand {
  public:
    abcCacheExportCmd() {};
    ~abcCacheExportCmd() {};

    virtual MStatus doIt( const MArgList &args );
    virtual MStatus undoIt();

    virtual MStatus redoIt();

    virtual bool isUndoable() const;

    virtual bool hasSyntax() const;

    static MSyntax mySyntax();

    static void* creator() {return new abcCacheExportCmd;};

    bool isHistoryOn() const;

    MString commandString() const;

    MStatus setHistoryOn( bool state );

    MStatus setCommandString( const MString & );

};

#endif //FX_MANAGER_CMD_H
