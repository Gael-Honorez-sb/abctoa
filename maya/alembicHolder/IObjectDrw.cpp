//-*****************************************************************************
//
// Copyright (c) 2009-2010,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#include "IObjectDrw.h"
#include "IPolyMeshDrw.h"
//#include "ISimpleXformDrw.h"
#include "IXformDrw.h"
#include "IPointsDrw.h"
//#include "ISubDDrw.h"

#include "json/json.h"

namespace AlembicHolder {

//-*****************************************************************************
IObjectDrw::IObjectDrw( IObject &iObj, bool iResetIfNoChildren, std::vector<std::string> path)
  : m_object( iObj )
  , m_minTime( ( chrono_t )FLT_MAX )
  , m_maxTime( ( chrono_t )-FLT_MAX )
  , m_currentTime( ( chrono_t )-FLT_MAX )
{
    // If not valid, just bail.
    if ( !m_object ) { return; }

   if (path.size())
   {
        const ObjectHeader *ohead = m_object.getChildHeader( path[0] );

        if ( ohead!=NULL )
        {
            path.erase(path.begin());
            DrawablePtr dptr;
            if ( IXform::matches( *ohead ) ) {
                IXform xform( m_object, ohead->getName() );
                if ( xform )
                {

                    ICompoundProperty arbGeomParams = xform.getSchema().getArbGeomParams();
                    if ( arbGeomParams != NULL && arbGeomParams.valid() )
                    {
                        std::vector<std::string> tags;
                        if (arbGeomParams.getPropertyHeader("mtoa_constant_tags") != NULL)
                        {
                            const PropertyHeader * tagsHeader = arbGeomParams.getPropertyHeader("mtoa_constant_tags");
                            if (IStringGeomParam::matches( *tagsHeader ))
                            {
                                IStringGeomParam param( arbGeomParams,  "mtoa_constant_tags" );
                                if ( param.valid() )
                                {
                                    IStringGeomParam::prop_type::sample_ptr_type valueSample =
                                                    param.getExpandedValue( m_currentTime ).getVals();

                                    if ( param.getScope() == kConstantScope || param.getScope() == kUnknownScope)
                                    {
                                        Json::Value jtags;
                                        Json::Reader reader;
                                        if(reader.parse(valueSample->get()[0], jtags))
                                            for( Json::ValueIterator itr = jtags.begin() ; itr != jtags.end() ; itr++ )
                                            {

                                                if (jtags[itr.key().asUInt()].asString() == "RENDER" )
                                                {
                                                    cout << "skipping this" << ohead->getName() << endl;
                                                    // we skip this thing
                                                    m_object.reset();
                                                    return;
                                                }
                                            }
                                    }
                                }
                            }
                        }
                    }


                    dptr.reset( new IXformDrw( xform, path ) );
                }
            }
            if ( dptr && dptr->valid() ) {
                m_children.push_back( dptr );
                m_minTime = std::min( m_minTime, dptr->getMinTime() );
                m_maxTime = std::max( m_maxTime, dptr->getMaxTime() );
            }
        }
    }
    else
    {
        // IObject has no explicit time sampling, but its children may.
        size_t numChildren = m_object.getNumChildren();
        for ( size_t i = 0; i < numChildren; ++i )
        {
            const ObjectHeader &ohead = m_object.getChildHeader( i );
            // Decide what to make.
            DrawablePtr dptr;
            if ( IPolyMesh::matches( ohead ) )
            {
                IPolyMesh pmesh( m_object, ohead.getName() );
                if ( pmesh )
                {
                    dptr.reset( new IPolyMeshDrw( pmesh, path ) );
                }
            }
            else if ( IPoints::matches( ohead ) )
            {
                IPoints points( m_object, ohead.getName() );
                if ( points )
                {
                    dptr.reset( new IPointsDrw( points, path ) );
                }
            }
            else if ( IXform::matches( ohead ) )
            {
                IXform xform( m_object, ohead.getName() );
                if ( xform )
                {
                    bool toSkip = false;
                    ICompoundProperty arbGeomParams = xform.getSchema().getArbGeomParams();
                    if ( arbGeomParams != NULL && arbGeomParams.valid() )
                    {
                        std::vector<std::string> tags;
                        if (arbGeomParams.getPropertyHeader("mtoa_constant_tags") != NULL)
                        {
                            const PropertyHeader * tagsHeader = arbGeomParams.getPropertyHeader("mtoa_constant_tags");
                            if (IStringGeomParam::matches( *tagsHeader ))
                            {
                                IStringGeomParam param( arbGeomParams,  "mtoa_constant_tags" );
                                if ( param.valid() )
                                {
                                    IStringGeomParam::prop_type::sample_ptr_type valueSample =
                                                    param.getExpandedValue( m_currentTime ).getVals();

                                    if ( param.getScope() == kConstantScope || param.getScope() == kUnknownScope)
                                    {
                                        Json::Value jtags;
                                        Json::Reader reader;
                                        if(reader.parse(valueSample->get()[0], jtags))
                                            for( Json::ValueIterator itr = jtags.begin() ; itr != jtags.end() ; itr++ )
                                            {

                                                if (jtags[itr.key().asUInt()].asString() == "RENDER" )
                                                {
                                                    // we skip this thing
                                                    toSkip = true;

                                                }
                                            }
                                    }
                                }
                            }
                        }
                    }

                    if(!toSkip)
                        dptr.reset( new IXformDrw( xform, path ) );
                }
            }
    /*        else if ( ISubD::matches( ohead ) )
            {
                ISubD subd( m_object, ohead.getName() );
                if ( subd )
                {
                    dptr.reset( new ISubDDrw( subd ) );
                }
            }
    */        else
            {
                IObject object( m_object, ohead.getName() );
                if ( object )
                {
                    dptr.reset( new IObjectDrw( object, true, path ) );
                }
            }

            if ( dptr && dptr->valid() )
            {
                m_children.push_back( dptr );
                m_minTime = std::min( m_minTime, dptr->getMinTime() );
                m_maxTime = std::max( m_maxTime, dptr->getMaxTime() );
            }
        }
    }

    // If we have no children, just leave.
    if ( m_children.size() == 0 && iResetIfNoChildren )
    {
        m_object.reset();
    }
}

//-*****************************************************************************
IObjectDrw::~IObjectDrw()
{
    // Nothing!
}

//-*****************************************************************************
chrono_t IObjectDrw::getMinTime()
{
    return m_minTime;
}

//-*****************************************************************************
chrono_t IObjectDrw::getMaxTime()
{
    return m_maxTime;
}

//-*****************************************************************************
bool IObjectDrw::valid()
{
    return m_object.valid();
}

//-*****************************************************************************
void IObjectDrw::setTime( chrono_t iTime )
{

	if(m_currentFrame != MAnimControl::currentTime().value())
	{
		for (std::map<double, Box3d>::iterator iter = m_bounds.begin(); iter != m_bounds.end(); ++iter) 
			iter->second.makeEmpty();

		m_bounds.clear();

		m_currentFrame = MAnimControl::currentTime().value();
	}

    if (iTime != m_currentTime)
    {
        m_currentTime = iTime;

        if ( !m_object ) { return; }

        // Object itself has no properties to worry about.

		for ( DrawablePtrVec::iterator iter = m_children.begin();
              iter != m_children.end(); ++iter )
        {
            DrawablePtr dptr = (*iter);
            if ( dptr )
            {
                dptr->setTime( iTime );
                //m_bounds.extendBy( dptr->getBounds() );
            }
        }
    }
}

//-*****************************************************************************
Box3d IObjectDrw::getBounds()
{
	if (m_bounds[m_currentTime].isEmpty())
    {
        for ( DrawablePtrVec::iterator iter = m_children.begin();
              iter != m_children.end(); ++iter )
        {
            DrawablePtr dptr = (*iter);
            if ( dptr )
            {
                m_bounds[m_currentTime].extendBy( dptr->getBounds() );
            }
        }
    }

    return m_bounds[m_currentTime];
}


int IObjectDrw::getNumTriangles()
{
    if ( !m_object ) { return 0; }
    int numTriangles = 0;
    for ( DrawablePtrVec::iterator iter = m_children.begin();
          iter != m_children.end(); ++iter )
    {
        DrawablePtr dptr = (*iter);
        if ( dptr )
        {
            numTriangles += dptr->getNumTriangles();
        }
    }
    return numTriangles;
}

//-*****************************************************************************
void IObjectDrw::draw( const DrawContext &iCtx )
{
    if ( !m_object ) { return; }

    for ( DrawablePtrVec::iterator iter = m_children.begin();
          iter != m_children.end(); ++iter )
    {
        DrawablePtr dptr = (*iter);
        if ( dptr )
        {
            dptr->draw( iCtx );
        }
    }
}

} // End namespace AlembicHolder

