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

#include "gpuCacheRasterSelect.h"

#include <algorithm>

//==============================================================================
// CLASS RasterSelect
//==============================================================================

#define MAX_RASTER_SELECT_RENDER_SIZE 16

//------------------------------------------------------------------------------
//
RasterSelect::RasterSelect(
    MSelectInfo& selectInfo
) 
    : fSelectInfo(selectInfo),
      fMinZ(std::numeric_limits<float>::max())
{
    M3dView view = fSelectInfo.view();

    view.beginGL();

    unsigned int sxl, syl, sw, sh;
    fSelectInfo.selectRect(sxl, syl, sw, sh);

    unsigned int vxl, vyl, vw, vh;
    view.viewport(vxl, vyl, vw, vh);

    //    Compute a new matrix that, when it is post-multiplied with the
    //    projection matrix, will cause the picking region to fill only
    //    a small region.

    const unsigned int width = (MAX_RASTER_SELECT_RENDER_SIZE < sw) ?
        MAX_RASTER_SELECT_RENDER_SIZE : sw;
    const unsigned int height = (MAX_RASTER_SELECT_RENDER_SIZE < sh) ?
        MAX_RASTER_SELECT_RENDER_SIZE : sh;

    const double sx = double(width) / double(sw);
    const double sy = double(height) / double(sh);

    const double fx = 2.0 / double(vw);
    const double fy = 2.0 / double(vh);
    
    MMatrix  selectMatrix;
    selectMatrix.matrix[0][0] = sx;
    selectMatrix.matrix[1][1] = sy;
    selectMatrix.matrix[3][0] = -1.0 - sx * (fx * (sxl - vxl) - 1.0);
    selectMatrix.matrix[3][1] = -1.0 - sy * (fy * (syl - vyl) - 1.0);
    
        
    MMatrix projMatrix;
    view.projectionMatrix(projMatrix);

    ::glMatrixMode(GL_PROJECTION);
    ::glPushMatrix();
    ::glLoadMatrixd(selectMatrix[0]);
    ::glMultMatrixd(projMatrix[0]);
    ::glMatrixMode(GL_MODELVIEW);

    ::glScissor(vxl, vyl, width, height);
    ::glEnable(GL_SCISSOR_TEST);
    ::glClear(GL_DEPTH_BUFFER_BIT);

    fWasDepthTestEnabled = ::glIsEnabled(GL_DEPTH_TEST);
    if (!fWasDepthTestEnabled) {
        ::glEnable(GL_DEPTH_TEST);
    }
}


//------------------------------------------------------------------------------
//
RasterSelect::~RasterSelect() 
{}


//------------------------------------------------------------------------------
//
void RasterSelect::processEdges(
    CAlembicDatas* geom,
    std::string sceneKey,
    size_t /* numWires */
)
{
    M3dView view = fSelectInfo.view();

    MMatrix projMatrix;
    view.projectionMatrix(projMatrix);
    MMatrix modelViewMatrix;
    view.modelViewMatrix(modelViewMatrix);

    MMatrix localToPort = modelViewMatrix * projMatrix;

    {/*
        Frustum frustum(localToPort.inverse());
        MMatrix xform(modelViewMatrix);
        
        DrawWireframeState state(frustum, seconds);
        DrawWireframeTraversal traveral(state, xform, false, Frustum::kUnknown);
        rootNode->accept(traveral);*/
		geom->abcSceneManager.getScene(sceneKey)->draw(geom->abcSceneState, geom->m_currselectionkey, geom->time, geom->m_params);
    }
}


//------------------------------------------------------------------------------
//
void RasterSelect::processTriangles(
    CAlembicDatas* geom,
    std::string sceneKey,
    size_t /* numTriangles */
)
{
    M3dView view = fSelectInfo.view();

    MMatrix projMatrix;
    view.projectionMatrix(projMatrix);
    MMatrix modelViewMatrix;
    view.modelViewMatrix(modelViewMatrix);

    MMatrix localToPort = modelViewMatrix * projMatrix;

    {/*
        Frustum frustum(localToPort.inverse());
        MMatrix xform(modelViewMatrix);
        
        DrawShadedState state(frustum, seconds);
        DrawShadedTraversal traveral(state, xform, false, Frustum::kUnknown);
        rootNode->accept(traveral);*/
		geom->abcSceneManager.getScene(sceneKey)->draw(geom->abcSceneState, geom->m_currselectionkey, geom->time, geom->m_params);
    }
}


//------------------------------------------------------------------------------
//
void RasterSelect::end()
{
    M3dView view = fSelectInfo.view();
    
    unsigned int sxl, syl, sw, sh;
    fSelectInfo.selectRect(sxl, syl, sw, sh);

    unsigned int vxl, vyl, vw, vh;
    view.viewport(vxl, vyl, vw, vh);

    const unsigned int width = (MAX_RASTER_SELECT_RENDER_SIZE < sw) ?
        MAX_RASTER_SELECT_RENDER_SIZE : sw;
    const unsigned int height = (MAX_RASTER_SELECT_RENDER_SIZE < sh) ?
        MAX_RASTER_SELECT_RENDER_SIZE : sh;

    float* selDepth = new float[MAX_RASTER_SELECT_RENDER_SIZE*
                                MAX_RASTER_SELECT_RENDER_SIZE];
    

    GLint buffer;
    ::glGetIntegerv( GL_READ_BUFFER, &buffer );
    ::glReadBuffer( GL_BACK );
    ::glReadPixels(vxl, vyl, width, height,
                   GL_DEPTH_COMPONENT, GL_FLOAT, (void *)selDepth); 
    ::glReadBuffer( buffer );

    for (unsigned int j=0; j<height; ++j) {
        for (unsigned int i=0; i<width; ++i) {
            const GLfloat depth = selDepth[j*width + i];
            if (depth < 1.0f) {
                fMinZ = std::min(fMinZ, depth);
            }
        }   
    }

    ::glMatrixMode(GL_PROJECTION);
    ::glPopMatrix();
    ::glMatrixMode(GL_MODELVIEW);

    ::glDisable(GL_SCISSOR_TEST);

    if (!fWasDepthTestEnabled) {
        ::glDisable(GL_DEPTH_TEST);        
    }

    view.endGL();
}

//------------------------------------------------------------------------------
//
bool RasterSelect::isSelected() const
{
    return fMinZ != std::numeric_limits<float>::max();
}


//------------------------------------------------------------------------------
//
float RasterSelect::minZ() const
{
    return fMinZ;
}
