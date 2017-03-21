#ifndef _gpuCacheSelect_h_
#define _gpuCacheSelect_h_

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

#include "nozAlembicHolderNode.h"

/*==============================================================================
 * CLASS Select
 *============================================================================*/

// Abstract API for selecting geometry.
class Select 
{
public:
    typedef unsigned int index_t;

    virtual ~Select();

    // Process edges to determine if they fall within the selection
    // region.
    virtual void processEdges(CAlembicDatas* geom,
                                std::string scenekey,
                              size_t numWires) = 0;

    // Process triangles to determine if they fall within the
    // selection region.
    virtual void processTriangles(CAlembicDatas* geom,
                                    std::string scenekey,
                                  size_t triangles) = 0;

    // End rasterization selection mode.
    // 
    // If a selection hit occurred, minZ() will be set to the depth of
    // the closest selection hit in the range [0..1]. If no selection
    // hit occurred, minZ() will be set to
    // std::numeric_limits<float>::max();
    virtual void end() = 0;

    // Returns whether any primitives actually falls within the
    // selection region. The returned value is undefined if end() has
    // never been called before.
    virtual bool isSelected() const = 0;

    // Returns minimum Z value. The returned value is undefined if
    // end() has never been called before.
    //
    // If a selection hit occurred, minZ() will be set to the depth
    // of the closest selection hit in the range [0..1]. If no
    // selection hit occurred, minZ() will be set to
    // std::numeric_limits<float>::max();
    virtual float minZ() const = 0;
};


#endif
