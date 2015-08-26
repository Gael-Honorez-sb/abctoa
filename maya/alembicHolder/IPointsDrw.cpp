//-*****************************************************************************
//
// Copyright (c) 2009-2011,
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

#include "IPointsDrw.h"
#include "RenderModules.h"
#include "PathUtil.h"
#include "samplingUtils.h"

namespace AlembicHolder {

    static MGLFunctionTable *gGLFT = NULL;

//-*****************************************************************************
IPointsDrw::IPointsDrw( IPoints &iPpoints, std::vector<std::string> path )
  : IObjectDrw( iPpoints, false, path )
  , m_points( iPpoints )
{
    // Get out if problems.
    if ( !m_points.valid() )
    {
        return;
    }

	if ( m_points.getSchema().getNumSamples() > 0 )
    {
        m_points.getSchema().get( m_samp );
    }

    m_boundsProp = m_points.getSchema().getSelfBoundsProperty();

    TimeSamplingPtr iTsmp = m_points.getSchema().getTimeSampling();
    if ( !m_points.getSchema().isConstant() )
    {
        size_t numSamps =  m_points.getSchema().getNumSamples();
        if ( numSamps > 0 )
        {
            chrono_t minTime = iTsmp->getSampleTime( 0 );
            m_minTime = std::min( m_minTime, minTime );
            chrono_t maxTime = iTsmp->getSampleTime( numSamps-1 );
            m_maxTime = std::max( m_maxTime, maxTime );
        }
    }

	m_currentFrame = MAnimControl::currentTime().value();
}

//-*****************************************************************************
IPointsDrw::~IPointsDrw()
{
    // Nothing!
}

//-*****************************************************************************
bool IPointsDrw::valid()
{
    return IObjectDrw::valid() && m_points.valid();
}

//-*****************************************************************************
void IPointsDrw::setTime( chrono_t iSeconds )
{

	// The frame is different. We should clear all the data.
	if(m_currentFrame != MAnimControl::currentTime().value())
	{
		for (std::map<double, PointDrwHelper>::iterator iter = m_drwHelpers.begin(); iter != m_drwHelpers.end(); ++iter) 
			iter->second.makeInvalid();

		for (std::map<double, Box3d>::iterator iter = m_bounds.begin(); iter != m_bounds.end(); ++iter) 
			iter->second.makeEmpty();

		m_drwHelpers.clear();
		m_bounds.clear();

		m_currentFrame = MAnimControl::currentTime().value();
	}

	if(m_drwHelpers.count(iSeconds) == 1)
	{
		if (iSeconds != m_currentTime)
			IObjectDrw::setTime( iSeconds );
		return;
	}



    // Use nearest for now.
	Alembic::AbcGeom::IPointsSchema schema = m_points.getSchema();
	

	m_alpha = 0.0;//getWeightAndIndex(iSeconds, schema.getTimeSampling(), schema.getNumSamples(), m_index, m_ceilIndex);

    m_ss =  ISampleSelector(iSeconds, ISampleSelector::kNearIndex );

    if ( IsAncestorInvisible(m_points,m_ss) ) {
        m_visible = false;
        return;
    }
    else
        m_visible = true;
    if (iSeconds != m_currentTime) {
        IObjectDrw::setTime( iSeconds );
        if ( !valid() )
        {
			m_drwHelpers[iSeconds].makeInvalid();
            return;
        }
        //IPolyMeshSchema::Sample psamp;
        if ( m_points.getSchema().isConstant() )
        {
			m_drwHelpers[iSeconds].setConstant( m_points.getSchema().isConstant() );
        }
        else if ( m_points.getSchema().getNumSamples() > 0 )
        {
            m_points.getSchema().get( m_samp, m_ss );           
        }

        m_bounds[iSeconds].makeEmpty();
        m_needtoupdate = true;
    }
}

void IPointsDrw::updateData()
{
	
Alembic::Abc::P3fArraySamplePtr ceilPoints; 


	P3fArraySamplePtr points = m_samp.getPositions();
    // update the points
    m_drwHelpers[m_currentTime].update( points, getBounds(), m_alpha );

    if ( !m_drwHelpers[m_currentTime].valid() )
    {
        m_points.reset();
        return;
    }
    m_needtoupdate = false;

}

Box3d IPointsDrw::getBounds()
{
    if(m_bounds[m_currentTime].isEmpty())
	{
		m_ss =  ISampleSelector(m_currentTime, ISampleSelector::kNearIndex );
        m_bounds[m_currentTime] = m_boundsProp.getValue( m_ss );
	}
    return m_bounds[m_currentTime];

}

//-*****************************************************************************
void IPointsDrw::draw( const DrawContext &iCtx )
{
    if ( !valid() )
    {
        return;
    }


    if(iCtx.getSelection() != "")
    {
        std::string pathSel = iCtx.getSelection();
        if(!pathInJsonString(m_points.getFullName(), pathSel))
            return;
    }

    if (m_needtoupdate)
        updateData();

    gGLFT->glPushAttrib( MGL_ENABLE_BIT );
    gGLFT->glDisable( MGL_LIGHTING );
	m_drwHelpers[m_currentTime].draw( iCtx );
    gGLFT->glPopAttrib( );
	IObjectDrw::draw( iCtx );
}

} // End namespace AlembicHolder
