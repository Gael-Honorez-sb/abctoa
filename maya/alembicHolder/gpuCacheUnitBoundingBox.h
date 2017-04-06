#ifndef _gpuCacheUnitBoundingBox_h_
#define _gpuCacheUnitBoundingBox_h_

//-
//**************************************************************************/
// Copyright 2012 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
//**************************************************************************/
//+

#include "gpuCacheSample.h"
#include <OpenEXR/IMathBox.h>


namespace AlembicHolder {

//==============================================================================
// CLASS UnitBoundingBox
//==============================================================================

// A unit bounding box and its buffers. (-1,-1,-1) - (1,1,1)
class UnitBoundingBox
{
public:
    // Return a unit bounding box.
    static const MBoundingBox&                    boundingBox();

    // Return the index buffer of a unit bounding box.
    static boost::shared_ptr<const IndexBuffer>&  indices();

    // Return the vertex buffer of a unit bounding box.
    static boost::shared_ptr<const VertexBuffer>& positions();

    // Free the unit bounding box buffers.
    static void                                   clear();

    // Return the transformation matrix of a unit bounding box to the given matrix.
    static MMatrix boundingBoxMatrix(const MBoundingBox& boundingBox);
    static MMatrix boundingBoxMatrix(const Imath::Box3d& boundingBox);

private:
    static boost::shared_ptr<const IndexBuffer>  fBoundingBoxIndices;
    static boost::shared_ptr<const VertexBuffer> fBoundingBoxPositions;
};

} // namespace AlembicHolder

#endif
