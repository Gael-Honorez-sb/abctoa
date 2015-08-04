//-*****************************************************************************
//
// Copyright (c) 2009-2014,
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

#include "getBounds.h"

//-*****************************************************************************
void accumXform( M44d &xf, IObject obj, chrono_t seconds )
{
    if ( IXform::matches( obj.getHeader() ) )
    {
        IXform x( obj, kWrapExisting );
        XformSample xs;
        ISampleSelector sel( seconds );
        x.getSchema().get( xs, sel );
        xf *= xs.getMatrix();
    }
}

//-*****************************************************************************
M44d getFinalMatrix( IObject &iObj, chrono_t seconds )
{
    M44d xf;
    xf.makeIdentity();

    IObject parent = iObj.getParent();

    while ( parent )
    {
        accumXform( xf, parent, seconds );
        parent = parent.getParent();
    }

    return xf;
}

//-*****************************************************************************
void getBounds( IObject iObj, chrono_t seconds, Box3d & g_bounds )
{
    Box3d bnds;
    bnds.makeEmpty();

    M44d xf = getFinalMatrix( iObj, seconds );
    IBox3dProperty boxProp;

    if ( ICurves::matches( iObj.getMetaData() ) )
    {
        ICurves curves( iObj, kWrapExisting );
        ICurvesSchema cs = curves.getSchema();
        boxProp = cs.getSelfBoundsProperty();
    }
    else if ( INuPatch::matches( iObj.getMetaData() ) )
    {
        INuPatch patch( iObj, kWrapExisting );
        INuPatchSchema ps = patch.getSchema();
        boxProp = ps.getSelfBoundsProperty();
    }
    else if ( IPolyMesh::matches( iObj.getMetaData() ) )
    {
        IPolyMesh mesh( iObj, kWrapExisting );
        IPolyMeshSchema ms = mesh.getSchema();
        boxProp = ms.getSelfBoundsProperty();
    }
    else if ( IPoints::matches( iObj.getMetaData() ) )
    {
        IPoints pts( iObj, kWrapExisting );
        IPointsSchema ps = pts.getSchema();
        boxProp = ps.getSelfBoundsProperty();
    }
    else if ( ISubD::matches( iObj.getMetaData() ) )
    {
        ISubD mesh( iObj, kWrapExisting );
        ISubDSchema ms = mesh.getSchema();
        boxProp = ms.getSelfBoundsProperty();
    }

    if ( boxProp.valid() )
    {
        ISampleSelector sel( seconds );
        bnds = boxProp.getValue( sel );
        bnds = Imath::transform( bnds, xf );
        g_bounds.extendBy( bnds );
    }

	
    g_bounds.extendBy( bnds );
}



//-*****************************************************************************
void getBoundingBox(IObject iObj, chrono_t seconds, Box3d & g_bounds )
{

    std::string path = iObj.getFullName();

    const MetaData &md = iObj.getMetaData();

    if ( ICurves::matches( md ) ||
        INuPatch::matches( md ) ||
        IPoints::matches( md ) ||
        IPolyMesh::matches( md ) ||
        ISubDSchema::matches( md ) )
    {
        getBounds( iObj, seconds, g_bounds );

    }

    // now the child objects
    for ( size_t i = 0 ; i < iObj.getNumChildren() ; i++ )
    {
        getBoundingBox( IObject(  iObj, iObj.getChildHeader( i ).getName() ),
                     seconds, g_bounds );
    }
}