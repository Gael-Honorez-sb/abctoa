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

namespace AlembicHolder {

    static MGLFunctionTable *gGLFT = NULL;

//-*****************************************************************************
IPointsDrw::IPointsDrw( IPoints &iPmesh, std::vector<std::string> path )
  : IObjectDrw( iPmesh, false, path )
  , m_points( iPmesh )
{
    // Get out if problems.
    if ( !m_points.valid() )
    {
        return;
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
    if (iSeconds != m_currentTime) {

        buffer.clear();

        IObjectDrw::setTime( iSeconds );
        if ( !valid() )
        {
            return;
        }
        // Use nearest for now.
        m_ss =  ISampleSelector(iSeconds, ISampleSelector::kNearIndex );

        m_points.getSchema().get( m_samp, m_ss );
        // Update bounds from positions
        m_bounds.makeEmpty();
        m_needtoupdate = true;

        // If we have a color prop, update it
        /*if ( m_colorProp )
        {
            m_colors = m_colorProp.getValue( ss );
        }

        if ( m_normalProp )
        {
            m_normals = m_normalProp.getValue( ss );
        }*/
    }
}

void IPointsDrw::updateData()
{
    m_positions = m_samp.getPositions();
    if ( m_positions )
    {
        size_t numPoints = m_positions->size();

        std::vector<MGLfloat> v;
        std::vector<MGLuint> vidx;

        for ( size_t p = 0; p < numPoints; ++p )
        {
            const V3f &P = (*m_positions)[p];
            v.push_back(P.x);
            v.push_back(P.y);
            v.push_back(P.z);
            vidx.push_back(p);
        }

        buffer.genVertexBuffer(v);
        buffer.genIndexBuffer(vidx, MGL_POINTS);
    }

    m_needtoupdate = false;
}

Box3d IPointsDrw::getBounds()
{
    if(m_bounds.isEmpty())
        m_bounds = m_boundsProp.getValue( m_ss );

    return m_bounds;

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

    if ( !m_positions || m_positions->size() == 0 )
    {
        IObjectDrw::draw( iCtx );
        return;
    }
    gGLFT->glPushAttrib( MGL_ENABLE_BIT );
    gGLFT->glDisable( MGL_LIGHTING );
    buffer.render();
    IObjectDrw::draw( iCtx );
    gGLFT->glPopAttrib( );
}

} // End namespace AlembicHolder
