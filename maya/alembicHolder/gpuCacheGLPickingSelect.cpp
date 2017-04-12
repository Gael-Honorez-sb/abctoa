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

#include "gpuCacheGLPickingSelect.h"
#include "gpuCacheBoundingBoxVisitor.h"
#include "gpuCacheVBOProxy.h"
#include <boost/shared_array.hpp>
#include <algorithm>

//==============================================================================
// LOCAL FUNCTIONS
//==============================================================================

const unsigned int  MAX_HW_DEPTH_VALUE = 0xffffffff;

//
// Returns the minimal depth of the pick buffer.
//
unsigned int closestElem(unsigned int nbPick, const GLuint* buffPtr)
{
    if (NULL == buffPtr) {
        return 0;
    }

    unsigned int Zdepth = MAX_HW_DEPTH_VALUE;
    for (int index = nbPick ; index > 0 ;--index)
    {
        if (buffPtr[0] &&        // Non void item
            (Zdepth > buffPtr[1]))    // Closer to camera
        {
            Zdepth = buffPtr[1];        // Zmin distance
        }
        buffPtr += buffPtr[0] + 3;    // Next item
    }
    return Zdepth;
}

//==============================================================================
// CLASS GLPickingSelect
//==============================================================================

//------------------------------------------------------------------------------
//
GLPickingSelect::GLPickingSelect(
    MSelectInfo& selectInfo
)
    : fSelectInfo(selectInfo),
      fMinZ(std::numeric_limits<float>::max())
{}


//------------------------------------------------------------------------------
//
GLPickingSelect::~GLPickingSelect()
{}


//------------------------------------------------------------------------------
//
void GLPickingSelect::processEdges(
    CAlembicDatas* geom,
    std::string scenekey,
    const size_t numWires
)
{
    const unsigned int bufferSize = (unsigned int)std::min(numWires,size_t(100000));
    boost::shared_array<GLuint> buffer(new GLuint[bufferSize*4]);

    M3dView view = fSelectInfo.view();
    /*
    MMatrix projMatrix;
    view.projectionMatrix(projMatrix);
    MMatrix modelViewMatrix;
    view.modelViewMatrix(modelViewMatrix);

    MMatrix localToPort = modelViewMatrix * projMatrix;

    view.beginSelect(buffer.get(), bufferSize*4);
    view.pushName(0);
    {
        Frustum frustum(localToPort.inverse());
        MMatrix xform(modelViewMatrix);

        DrawWireframeState state(frustum, seconds);
        DrawWireframeTraversal traveral(state, xform, false, Frustum::kUnknown);
        rootNode->accept(traveral);
    }
    view.popName();
    int nbPick = view.endSelect();

    if (nbPick > 0) {
        unsigned int Zdepth = closestElem(nbPick, buffer.get());
        float depth = float(Zdepth)/MAX_HW_DEPTH_VALUE;
        fMinZ = std::min(depth,fMinZ);
    }*/
}

//------------------------------------------------------------------------------
//
void GLPickingSelect::processTriangles(
    CAlembicDatas* geom,
    std::string sceneKey,
    const size_t numTriangles
)
{
    const unsigned int bufferSize = (unsigned int)std::min(numTriangles,size_t(100000));
    boost::shared_array<GLuint>buffer (new GLuint[bufferSize*4]);

    M3dView view = fSelectInfo.view();
    /*
    MMatrix projMatrix;
    view.projectionMatrix(projMatrix);
    MMatrix modelViewMatrix;
    view.modelViewMatrix(modelViewMatrix);

    MMatrix localToPort = modelViewMatrix * projMatrix;
    */
    view.beginSelect(buffer.get(), bufferSize*4);
    view.pushName(0);
    {/*
        Frustum frustum(localToPort.inverse());
        MMatrix xform(modelViewMatrix);

        DrawShadedState state(frustum, seconds);
        DrawShadedTraversal traveral(state, xform, false, Frustum::kUnknown);
        rootNode->accept(traveral);*/
		geom->abcSceneManager.getScene(sceneKey)->draw(geom->abcSceneState, geom->m_currselectionkey, geom->time, geom->m_params);
    }
    view.popName();
    int nbPick = view.endSelect();

    if (nbPick > 0) {
        unsigned int Zdepth = closestElem(nbPick, buffer.get());
        float depth = float(Zdepth)/MAX_HW_DEPTH_VALUE;
        fMinZ = std::min(depth,fMinZ);
    }
}

//------------------------------------------------------------------------------
//
void GLPickingSelect::processBoundingBox(
    const AlembicHolder::DrawablePtr&    rootNode,
    const double       seconds
)
{
    // Allocate select buffer.
    const unsigned int bufferSize = 12;
    GLuint buffer[12*4];

    // Get the current viewport.
    M3dView view = fSelectInfo.view();

    // Get the bounding box.
    MBoundingBox boundingBox = AlembicHolder::BoundingBoxVisitor::boundingBox(rootNode, seconds);

    // Draw the bounding box.
    view.beginSelect(buffer, bufferSize*4);
    view.pushName(0);
    {
        AlembicHolder::VBOProxy vboProxy;
        vboProxy.drawBoundingBox(boundingBox);
    }
    view.popName();
    int nbPick = view.endSelect();

    // Determine the closest point.
    if (nbPick > 0) {
        unsigned int Zdepth = closestElem(nbPick, buffer);
        float depth = float(Zdepth)/MAX_HW_DEPTH_VALUE;
        fMinZ = std::min(depth,fMinZ);
    }
}


//------------------------------------------------------------------------------
//
void GLPickingSelect::end()
{}


//------------------------------------------------------------------------------
//
bool GLPickingSelect::isSelected() const
{
    return fMinZ != std::numeric_limits<float>::max();
}


//------------------------------------------------------------------------------
//
float GLPickingSelect::minZ() const
{
    return fMinZ;
}
