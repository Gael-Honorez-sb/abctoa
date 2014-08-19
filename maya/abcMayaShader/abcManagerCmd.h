#ifndef FX_MANAGER_CMD_H
#define FX_MANAGER_CMD_H

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>

#include "abcMayaShaderNode.h"

class abcManagerCmd : public MPxCommand {
  public:
    abcManagerCmd() {};
    ~abcManagerCmd() {};

    virtual MStatus doIt( const MArgList &args );
    virtual MStatus undoIt();
 
    virtual MStatus redoIt();
 
    virtual bool isUndoable() const;
 
    virtual bool hasSyntax() const;
 
    static MSyntax mySyntax();

    static void* creator() {return new abcManagerCmd;};
 
    bool isHistoryOn() const;
 
    MString commandString() const;
 
    MStatus setHistoryOn( bool state );
 
    MStatus setCommandString( const MString & );
};



#endif //FX_MANAGER_CMD_H
