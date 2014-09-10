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


#ifndef ARCHIVEHELPER_H_
#define ARCHIVEHELPER_H_


//-*****************************************************************************
#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/All.h>
//-*****************************************************************************

using namespace Alembic::AbcGeom;



// Get a list of geometry objects - IPolyMeshes and ISubDs
void getABCGeos(Alembic::Abc::IObject & iObj,
                std::vector<Alembic::AbcGeom::IObject> & _objs);

// Get a list of IXforms
void getABCXforms(Alembic::Abc::IObject & iObj,
                std::vector<Alembic::AbcGeom::IXform> & _objs);

// Get a list of ICameras
void getABCCameras(Alembic::Abc::IObject & iObj,
                std::vector<Alembic::AbcGeom::ICamera> & _objs);

// Find object by name
bool getNamedXform( IObject iObjTop, const std::string &iName, IXform &iXf );
bool getNamedCamera( IObject iObjTop, const std::string &iName, ICamera &iCam );
bool getNamedObj( IObject & iObjTop, const std::string &iName);

// Get the time span of a single IObject, or the whole ABC archive
void getXformTimeSpan(IXform iXf, chrono_t& first, chrono_t& last);
void getCameraTimeSpan(ICamera iCam, chrono_t& first, chrono_t& last);
void getPolyMeshTimeSpan(IPolyMesh iPoly, chrono_t& first, chrono_t& last);
void getSubDTimeSpan(ISubD iSub, chrono_t& first, chrono_t& last);
void getABCTimeSpan(Alembic::Abc::IArchive archive, chrono_t& first, chrono_t& last);


#endif /* ARCHIVEHELPER_H_ */
