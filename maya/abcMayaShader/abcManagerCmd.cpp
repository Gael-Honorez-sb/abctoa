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
