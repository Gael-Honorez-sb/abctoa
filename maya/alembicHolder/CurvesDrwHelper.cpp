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

#include "CurvesDrwHelper.h"
#include "samplingUtils.h"

namespace AlembicHolder {

static MGLFunctionTable *gGLFT = NULL;


//-*****************************************************************************
CurvesDrwHelper::CurvesDrwHelper()
{
    makeInvalid();
    buffer.clear();
}

//-*****************************************************************************
CurvesDrwHelper::~CurvesDrwHelper()
{
    makeInvalid();
}

//-*****************************************************************************
void CurvesDrwHelper::update( P3fArraySamplePtr iP,
                 Abc::Box3d iBounds, 
                 double alpha )
{

   // Before doing a ton, just have a quick look.

    /*if ( m_pointsP && iP &&
         ( m_pointsP->size() == iP->size() )
		 )
    {
        if ( m_pointsP == iP )
        {
			return;
        }
    }*/

    buffer.clear();

    m_pointsP = iP;




    // Check stuff.
    if ( !m_pointsP )
    {
        std::cerr << "Point update quitting because no input data"
                  << std::endl;
        makeInvalid();
        return;
    }

    // Get the number of each thing.
    size_t numPoints = m_pointsP->size();
    if ( numPoints < 1 )
    {
        // Invalid.
        std::cerr << "Point update quitting because bad arrays"
                  << ", numPoints = " << numPoints
                  << std::endl;
        makeInvalid();
        return;
    }

    std::vector<MGLfloat> v;
	std::vector<MGLuint> vidx;
    {
        for ( size_t p = 0; p < numPoints; ++p )
        {
            const V3f &P = (*m_pointsP)[p];
            if (alpha == 0 ) 
            {
                v.push_back(P.x);
                v.push_back(P.y);
                v.push_back(P.z);
				vidx.push_back(p);
            }
        }
    }

    buffer.genVertexBuffer(v);
    buffer.genIndexBuffer(vidx, MGL_POINTS);

    m_valid = true;



    if ( iBounds.isEmpty() )
    {
        computeBounds();
    }
    else
    {
        m_bounds = iBounds;
    }
    
    // And that's it.
}

void CurvesDrwHelper::draw( const DrawContext & iCtx) const
{

    // Bail if invalid.
    if ( !m_valid || !m_pointsP )
    {
        return;
    }

    buffer.render();

}

//-*****************************************************************************
void CurvesDrwHelper::makeInvalid()
{
    m_pointsP.reset();
    m_valid = false;
    m_bounds.makeEmpty();
}


//-*****************************************************************************
void CurvesDrwHelper::computeBounds()
{
    m_bounds.makeEmpty();
    if ( m_pointsP )
    {
        size_t numPoints = m_pointsP->size();
        for ( size_t p = 0; p < numPoints; ++p )
        {
            const V3f &P = (*m_pointsP)[p];
            m_bounds.extendBy( V3d( P.x, P.y, P.z ) );
        }
    }
}

} // End namespace AlembicHolder
