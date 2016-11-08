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

#include "ArchiveHelper.h"
//-*****************************************************************************

#include "../../../common/PathUtil.h"
#include "boost/foreach.hpp"
using namespace Alembic::AbcGeom;


bool isVisible(IObject child, IXformSchema xs, chrono_t currentTime, holderPrms *params)
{
    if(GetVisibility( child, ISampleSelector( currentTime ) ) == kVisibilityHidden)
    {
        // check if the object is not forced to be visible
        std::string name = child.getFullName();

        if(params->linkAttributes)
        {
            for(std::vector<std::string>::iterator it=params->attributes.begin(); it!=params->attributes.end(); ++it)
            {
                Json::Value attributes;
                if (it->find("/") != string::npos && name.find(*it) != string::npos)
                    attributes = params->attributesRoot[*it];

                if(attributes.size() > 0)
                {
                    for( Json::ValueIterator itr = attributes.begin() ; itr != attributes.end() ; itr++ )
                    {
                        std::string attribute = itr.key().asString();
                        if (attribute == "forceVisible")
                        {

                            bool vis = params->attributesRoot[*it][itr.key().asString()].asBool();
                            if(vis)
                                return true;
                            else
                                return false;
                        }
                    }
                }
            }
        }
        return false;
    }

    // Check it's a xform and that xform has a tag "DISPLAY" to skip it.
    ICompoundProperty prop = xs.getArbGeomParams();
    if ( prop != NULL && prop.valid() )
    {
        if (prop.getPropertyHeader("mtoa_constant_tags") != NULL)
        {
            const PropertyHeader * tagsHeader = prop.getPropertyHeader("mtoa_constant_tags");
            if (IStringGeomParam::matches( *tagsHeader ))
            {
                IStringGeomParam param( prop,  "mtoa_constant_tags" );
                if ( param.valid() )
                {
                    IStringGeomParam::prop_type::sample_ptr_type valueSample =
                                    param.getExpandedValue( ISampleSelector( currentTime ) ).getVals();

                    if ( param.getScope() == kConstantScope || param.getScope() == kUnknownScope)
                    {
                        Json::Value jtags;
                        Json::Reader reader;
                        if(reader.parse(valueSample->get()[0], jtags))
                            for( Json::ValueIterator itr = jtags.begin() ; itr != jtags.end() ; itr++ )
                            {

                                if (jtags[itr.key().asUInt()].asString() == "RENDER" )
                                    return false;
                            }
                    }
                }
            }
        }
    }

    return true;
}

bool isVisible(IObject child, chrono_t currentTime, holderPrms* params, bool currentlyVisible)
{
    if(GetVisibility( child, ISampleSelector( currentTime ) ) == kVisibilityHidden || currentlyVisible==false)
    {
        // check if the object is not forced to be visible
        std::string name = child.getFullName();

        if(params->linkAttributes)
        {
            for(std::vector<std::string>::iterator it=params->attributes.begin(); it!=params->attributes.end(); ++it)
            {
                Json::Value attributes;
                if (it->find("/") != string::npos && name.find(*it) != string::npos)
                    attributes = params->attributesRoot[*it];


                if(attributes.size() > 0)
                {
                    for( Json::ValueIterator itr = attributes.begin() ; itr != attributes.end() ; itr++ )
                    {
                        std::string attribute = itr.key().asString();
                        if (attribute == "forceVisible")
                        {

                            bool vis = params->attributesRoot[*it][itr.key().asString()].asBool();
                            if(vis)
                                return true;
                            else
                                return false;
                        }

                    }

                }
            }
        }
        return false;
    }

    return true;
}

bool isVisibleForArnold(IObject child, chrono_t currentTime, holderPrms* params, bool currentlyVisible)
{
    if (!isVisible(child, currentTime, params, currentlyVisible))
        return false;

    unsigned short minVis = AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE|AI_RAY_SUBSURFACE|AI_RAY_REFRACTED|AI_RAY_REFLECTED|AI_RAY_SHADOW|AI_RAY_CAMERA);
    std::string name = child.getFullName();

    if(params->linkAttributes)
    {
        for(std::vector<std::string>::iterator it=params->attributes.begin(); it!=params->attributes.end(); ++it)
        {
                Json::Value attributes;
                if(it->find("/") != string::npos)
                    if(pathContainsOtherPath(name, *it))
					{
                        attributes = params->attributesRoot[*it];
					}

                if(attributes.size() > 0)
                {
                    for( Json::ValueIterator itr = attributes.begin() ; itr != attributes.end() ; itr++ )
                    {
                        std::string attribute = itr.key().asString();
                        if (attribute == "visibility")
                        {

                            unsigned short vis = params->attributesRoot[*it][itr.key().asString()].asInt();
                            if(vis <= minVis)
                            {
                                return false;
                            }

                            break;
                        }

                    }
                }
        }
    }
    return true;

}


bool getNamedObj( IObject & iObjTop, const std::string &objectPath)
{

    PathList path;
    TokenizePath( objectPath, "|", path );

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
        return true;
    }



//    const ObjectHeader *ohead = iObjTop.getChildHeader( path[0] );
//    if ( ohead != NULL )
//    {
//        path.erase(path.begin());
//         if ( IXform::matches( *ohead ) )
//         {
//          IObject obj( iObjTop, ohead->getName() );
//          std::cout << "Found : "<<obj.getFullName() << std::endl;
//          iObj = obj;
//          return true;
//         }
//
//
//    }
    return false;
    //
//  // Return true if found
//
//  //const Alembic::AbcGeom::MetaData &md = iObjTop.getMetaData();
//  if ( (iObjTop.getName() == iName) )
//  {
//      std::cout << "Found : "<<iObjTop.getFullName() << std::endl;
//      return true;
//  }
//
//  // now the child objects
//  for ( size_t i = 0 ; i < iObjTop.getNumChildren() ; i++ )
//  {
//      if (getNamedObj(IObject( iObjTop, iObjTop.getChildHeader( i ).getName() ), iName, iObj ))
//          return true;
//  }
//
//  return false;

}

void getABCGeos(Alembic::Abc::IObject & iObj,
        std::vector<Alembic::AbcGeom::IObject> & _objs)
{

    size_t numChildren = iObj.getNumChildren();

    for (size_t i=0; i<numChildren; ++i)
    {
        IObject child( iObj.getChild( i ));

        _objs.push_back(child);
//      if ( Alembic::AbcGeom::IPolyMesh::matches(child.getHeader())
//      || Alembic::AbcGeom::ISubD::matches(child.getHeader())) {
//          std::cout << "find child" << std::endl;

//      }
//
//      if (child.getNumChildren() > 0) {
//          getABCGeos(child, _objs);
//      }
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
