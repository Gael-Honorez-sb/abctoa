#include "abcManagerCmd.h"
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>

//
// abcManagerCmd::doIt
//
//  This is the function used to invoke the command. The
// command is not undoable and it does not change any state,
// so it does not use the method to call back throught redoIt.
//////////////////////////////////////////////////////////////////////

MStatus abcManagerCmd::doIt( const MArgList &args) {
  MArgDatabase argData( syntax(), args);
  MString nodeName;
  abcMayaShader *node = NULL;

  //check to see if we have a node
  if (argData.isFlagSet( "-n")) {
    argData.getFlagArgument( "-n", 0, nodeName);
    node = abcMayaShader::findNodeByName(nodeName);
    if (!node) {
      MGlobal::displayError( MString("Node '") +nodeName +"' does not exist");
      return MStatus::kFailure;
    }
  }
/*
  //check if we are in list shader mode
  if (argData.isFlagSet( "-ls")) {
    int pass;
    //are we listing a vertex shader
    if (argData.isFlagSet( "-vs")) {
      argData.getFlagArgument( "-vs", 0, pass);
      if (!node->printVertexShader(pass)) {
        MGlobal::displayError( MString("No vertex shader available for pass ") + pass);
        return MStatus::kFailure;
      }
    }
    //are we listing a vertex shader
    if (argData.isFlagSet( "-ps")) {
      argData.getFlagArgument( "-ps", 0, pass);
      if (!node->printPixelShader(pass)) {
        MGlobal::displayError( MString("No pixel shader available for pass ") + pass);
        return MStatus::kFailure;
      }
    }
  }
  */

  return MStatus::kSuccess;
}

//
//  There is never anything to undo.
//////////////////////////////////////////////////////////////////////
MStatus abcManagerCmd::undoIt(){
  return MStatus::kSuccess;
}

//
//  There is never really anything to redo.
//////////////////////////////////////////////////////////////////////
MStatus abcManagerCmd::redoIt(){
  return MStatus::kSuccess;
}

//
//
//////////////////////////////////////////////////////////////////////
bool abcManagerCmd::isUndoable() const{
  return false;
}

//
//
//////////////////////////////////////////////////////////////////////
bool abcManagerCmd::hasSyntax() const {
  return true;
}

//
//
//////////////////////////////////////////////////////////////////////
MSyntax abcManagerCmd::mySyntax() {
  MSyntax syntax;

  //syntax.addFlag( "-ls", "-listShader");
  syntax.addFlag( "-n", "-node", MSyntax::kString);

  return syntax;
}

//
//
//////////////////////////////////////////////////////////////////////
bool abcManagerCmd::isHistoryOn() const {
  //what is this supposed to do?
  return false;
}

MString abcManagerCmd::commandString() const {
  return MString();
}


MStatus abcManagerCmd::setHistoryOn( bool state ){
  //ignore it for now
  return MStatus::kSuccess;
}

MStatus abcManagerCmd::setCommandString( const MString &str) {
  //ignore it for now
  return MStatus::kSuccess;
}
