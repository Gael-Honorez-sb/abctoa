/*
 * ArchiveHelper.cpp
 *
 *  Created on: Jan 19, 2012
 *      Author: nozon
 */

#include "ArchiveHelper.h"
//-*****************************************************************************

#include "../PathUtil.h"
#include "boost/foreach.hpp"
using namespace Alembic::AbcGeom;

bool getNamedObj( IObject & iObjTop, const std::string &objectPath)
{

    PathList path;
    TokenizePath( objectPath, path );

    const ObjectHeader *nextChildHeader = &iObjTop.getHeader();
    IObject nextParentObject = iObjTop;

    BOOST_FOREACH(std::string &childName, path)
    {
        nextChildHeader = nextParentObject.getChildHeader( childName );
        if ( nextChildHeader == NULL ) {
            break;
        } else {
            nextParentObject = IObject( nextParentObject, childName );
        }
    }

    if ( nextChildHeader != NULL )
    {
    	iObjTop = nextParentObject;
    	std::cout << "Found : "<<iObjTop.getFullName() << std::endl;
    	return true;
    }



//    const ObjectHeader *ohead = iObjTop.getChildHeader( path[0] );
//    if ( ohead != NULL )
//    {
//        path.erase(path.begin());
//         if ( IXform::matches( *ohead ) )
//         {
//        	IObject obj( iObjTop, ohead->getName() );
//        	std::cout << "Found : "<<obj.getFullName() << std::endl;
//        	iObj = obj;
//        	return true;
//         }
//
//
//    }
    return false;
    //
//	// Return true if found
//
//	//const Alembic::AbcGeom::MetaData &md = iObjTop.getMetaData();
//	if ( (iObjTop.getName() == iName) )
//	{
// 		std::cout << "Found : "<<iObjTop.getFullName() << std::endl;
//		return true;
//	}
//
//	// now the child objects
//	for ( size_t i = 0 ; i < iObjTop.getNumChildren() ; i++ )
//	{
//		if (getNamedObj(IObject( iObjTop, iObjTop.getChildHeader( i ).getName() ), iName, iObj ))
//			return true;
//	}
//
//	return false;

}

void getABCGeos(Alembic::Abc::IObject & iObj,
		std::vector<Alembic::AbcGeom::IObject> & _objs)
{

	size_t numChildren = iObj.getNumChildren();

	for (size_t i=0; i<numChildren; ++i)
	{
		IObject child( iObj.getChild( i ));

		_objs.push_back(child);
//		if ( Alembic::AbcGeom::IPolyMesh::matches(child.getHeader())
//		|| Alembic::AbcGeom::ISubD::matches(child.getHeader())) {
//			std::cout << "find child" << std::endl;

//		}
//
//		if (child.getNumChildren() > 0) {
//			getABCGeos(child, _objs);
//		}
	}
}

//-*****************************************************************************

void getABCXforms(Alembic::Abc::IObject & iObj,
		std::vector<Alembic::AbcGeom::IXform> & _objs)
{

	unsigned int numChildren = iObj.getNumChildren();

	for (unsigned i=0; i<numChildren; ++i)
	{
		IObject child( iObj.getChild( i ));
		if ( Alembic::AbcGeom::IXform::matches(child.getHeader()) ) {
			IXform xf(child, Alembic::Abc::kWrapExisting);
			_objs.push_back(xf);
		}

		if (child.getNumChildren() > 0) {
			getABCXforms(child, _objs);
		}
	}
}

//-*****************************************************************************

void getABCCameras(Alembic::Abc::IObject & iObj,
		std::vector<Alembic::AbcGeom::ICamera> & _objs)
{

	unsigned int numChildren = iObj.getNumChildren();

	for (unsigned i=0; i<numChildren; ++i)
	{
		IObject child( iObj.getChild( i ));
		if ( Alembic::AbcGeom::ICamera::matches(child.getHeader()) ) {
			ICamera cam(child, Alembic::Abc::kWrapExisting);
			_objs.push_back(cam);
		}

		if (child.getNumChildren() > 0) {
			getABCCameras(child, _objs);
		}
	}
}
//-*****************************************************************************

bool getNamedXform( IObject iObjTop, const std::string &iName, IXform &iXf )
{
	// Return true if found

	const Alembic::AbcGeom::MetaData &md = iObjTop.getMetaData();
	if ( (iObjTop.getName() == iName) && (IXform::matches( md )) )
	{
		iXf = IXform(iObjTop, kWrapExisting );
		return true;
	}

	// now the child objects
	for ( size_t i = 0 ; i < iObjTop.getNumChildren() ; i++ )
	{
		if (getNamedXform(IObject( iObjTop, iObjTop.getChildHeader( i ).getName() ), iName, iXf ))
			return true;
	}

	return false;

}

//-*****************************************************************************

bool getNamedCamera( IObject iObjTop, const std::string &iName, ICamera &iCam )
{
	// Return true if found

	const Alembic::AbcGeom::MetaData &md = iObjTop.getMetaData();
	if ( (iObjTop.getName() == iName) && (ICamera::matches( md )) )
	{
		iCam = ICamera(iObjTop, kWrapExisting );
		return true;
	}

	// now the child objects
	for ( size_t i = 0 ; i < iObjTop.getNumChildren() ; i++ )
	{
		if (getNamedCamera(IObject( iObjTop, iObjTop.getChildHeader( i ).getName() ), iName, iCam ))
			return true;
	}

	return false;

}
//-*****************************************************************************

void getXformTimeSpan(IXform iXf, chrono_t& first, chrono_t& last) {

	IXformSchema xf = iXf.getSchema();
	TimeSamplingPtr ts = xf.getTimeSampling();
	first = std::min(first, ts->getSampleTime(0) );
	last = std::max(last, ts->getSampleTime(xf.getNumSamples()-1) );
}

//-*****************************************************************************

void getCameraTimeSpan(ICamera iCam, chrono_t& first, chrono_t& last) {

	ICameraSchema cam = iCam.getSchema();
	TimeSamplingPtr ts = cam.getTimeSampling();
	first = std::min(first, ts->getSampleTime(0) );
	last = std::max(last, ts->getSampleTime(cam.getNumSamples()-1) );
}

//-*****************************************************************************

void getPolyMeshTimeSpan(IPolyMesh iPoly, chrono_t& first, chrono_t& last) {

	IPolyMeshSchema mesh = iPoly.getSchema();
	TimeSamplingPtr ts = mesh.getTimeSampling();
	first = std::min(first, ts->getSampleTime(0) );
	last = std::max(last, ts->getSampleTime(mesh.getNumSamples()-1) );
}

//-*****************************************************************************

void getSubDTimeSpan(ISubD iSub, chrono_t& first, chrono_t& last) {

	ISubDSchema mesh = iSub.getSchema();
	TimeSamplingPtr ts = mesh.getTimeSampling();
	first = std::min(first, ts->getSampleTime(0) );
	last = std::max(last, ts->getSampleTime(mesh.getNumSamples()-1) );
}

//-*****************************************************************************

void getABCTimeSpan(IArchive archive, chrono_t& first, chrono_t& last)
{

	if (!archive.valid())
		return;

	IObject archiveTop = archive.getTop();

	unsigned int numChildren = archiveTop.getNumChildren();

	for (unsigned i=0; i<numChildren; ++i)
	{
		IObject child( archiveTop.getChild( i ));

		if ( Alembic::AbcGeom::IPolyMesh::matches(child.getHeader()) ) {

			IPolyMesh iPoly(child, Alembic::Abc::kWrapExisting);
			getPolyMeshTimeSpan(iPoly, first, last);
		}

		else if ( Alembic::AbcGeom::ISubD::matches(child.getHeader()) ) {

			ISubD iSub(child, Alembic::Abc::kWrapExisting);
			getSubDTimeSpan(iSub, first, last);
		}

		else if ( Alembic::AbcGeom::IXform::matches(child.getHeader()) ) {

			IXform iXf(child, Alembic::Abc::kWrapExisting);
			getXformTimeSpan(iXf, first, last);
		}

		else if ( Alembic::AbcGeom::ICamera::matches(child.getHeader()) ) {

			ICamera iCam(child, Alembic::Abc::kWrapExisting);
			getCameraTimeSpan(iCam, first, last);
		}

	}

}
//-*****************************************************************************
