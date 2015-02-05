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


#include <maya/MPxCommand.h>
#include <maya/MString.h>

#include <Alembic/AbcCoreFactory/IFactory.h>
#include <Alembic/AbcGeom/Visibility.h>
#include "Alembic/AbcGeom/All.h"
#include "Alembic/Abc/All.h"
#include <json/json.h>
#include <maya/MFileObject.h>

#ifndef ABCGETTAGS_H_
#define ABCGETTAGS_H_

using namespace Alembic::AbcGeom;

class ABCGetTags : public MPxCommand
{
public:
//  ABCHierarchy();
//  virtual ~ABCHierarchy();
    static void     cleanup();
    MStatus doIt( const MArgList& args );
    bool isUndoable() const {return false;};
    static void* creator();

    void visitObject( IObject iObj, Json::Value & results );

    const char *    filename () const {return m_filename.asChar();}
    const char *    path () const {return m_path.asChar();}

    static const MString commandName;

private:
    MString m_filename;
    MString m_path;

};


#endif /* ABCGETTAGS_H_ */
