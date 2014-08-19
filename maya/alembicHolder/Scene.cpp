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

#include "Scene.h"
#include "IObjectDrw.h"
#include "PathUtil.h"
#include <Alembic/AbcCoreFactory/IFactory.h>
#include "boost/foreach.hpp"

//-*****************************************************************************
namespace SimpleAbcViewer {



//-*****************************************************************************
void setMaterials( float o, bool negMatrix = false )
{
   static MGLFunctionTable *gGLFT = NULL;
   if (gGLFT == NULL)
      gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();

    if ( negMatrix )
    {
        MGLfloat mat_front_diffuse[] = { 0.1 * o, 0.1 * o, 0.9 * o, o };
        MGLfloat mat_back_diffuse[] = { 0.9 * o, 0.1 * o, 0.9 * o, o };

        MGLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
        MGLfloat mat_shininess[] = { 100.0 };
//        GLfloat light_position[] = { 20.0, 20.0, 20.0, 0.0 };

        gGLFT->glClearColor( 0.0, 0.0, 0.0, 0.0 );
        gGLFT->glMaterialfv( MGL_FRONT, MGL_DIFFUSE, mat_front_diffuse );
        gGLFT->glMaterialfv( MGL_FRONT, MGL_SPECULAR, mat_specular );
        gGLFT->glMaterialfv( MGL_FRONT, MGL_SHININESS, mat_shininess );

        gGLFT->glMaterialfv( MGL_BACK, MGL_DIFFUSE, mat_back_diffuse );
        gGLFT->glMaterialfv( MGL_BACK, MGL_SPECULAR, mat_specular );
        gGLFT->glMaterialfv( MGL_BACK, MGL_SHININESS, mat_shininess );
    }
    else
    {

        MGLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
        MGLfloat mat_shininess[] = { 100.0 };
//        GLfloat light_position[] = { 20.0, 20.0, 20.0, 0.0 };
        MGLfloat mat_front_emission[] = {0.0, 0.0, 0.0, 0.0 };
        MGLfloat mat_back_emission[] = {o, 0.0, 0.0, o };

        gGLFT->glClearColor( 0.0, 0.0, 0.0, 0.0 );
        gGLFT->glMaterialfv( MGL_FRONT, MGL_EMISSION, mat_front_emission );
        gGLFT->glMaterialfv( MGL_FRONT, MGL_SPECULAR, mat_specular );
        gGLFT->glMaterialfv( MGL_FRONT, MGL_SHININESS, mat_shininess );

        gGLFT->glMaterialfv( MGL_BACK, MGL_EMISSION, mat_back_emission );
        gGLFT->glMaterialfv( MGL_BACK, MGL_SPECULAR, mat_specular );
        gGLFT->glMaterialfv( MGL_BACK, MGL_SHININESS, mat_shininess );

        gGLFT->glColorMaterial(MGL_FRONT_AND_BACK, MGL_DIFFUSE);
        gGLFT->glEnable(MGL_COLOR_MATERIAL);
    }
}

//-*****************************************************************************
// SCENE CLASS
//-*****************************************************************************

//-*****************************************************************************
Scene::Scene( const std::string &abcFileName, const std::string &objectPath )
  : m_fileName( abcFileName )
  , m_objectPath( objectPath )
  , m_minTime( ( chrono_t )FLT_MAX )
  , m_maxTime( ( chrono_t )-FLT_MAX )
{        
    boost::timer Timer;

    
    Alembic::AbcCoreFactory::IFactory factory;
    //factory.(32setOgawaNumStreams);
    m_archive = factory.getArchive(abcFileName );
    m_topObject = IObject( m_archive, kTop );

    m_selectionPath = "";

    // try to walk to the path
    PathList path;
    TokenizePath( objectPath, path );

    m_drawable.reset( new IObjectDrw( m_topObject, false, path ) );


    ABCA_ASSERT( m_drawable->valid(),
                 "Invalid drawable for archive: " << abcFileName );


    m_minTime = m_drawable->getMinTime();
    m_maxTime = m_drawable->getMaxTime();

    if ( m_minTime <= m_maxTime )
    {
        m_drawable->setTime( m_minTime );
    }
    else
    {
        m_minTime = m_maxTime = 0.0;
        m_drawable->setTime( 0.0 );
    }

    ABCA_ASSERT( m_drawable->valid(),
                 "Invalid drawable after reading start time" );



    // Bounds have been formed!
    m_bounds = m_drawable->getBounds();


    std::cout << "[aABCH] Opened archive: " << abcFileName << "|" << objectPath << std::endl;

}

//-*****************************************************************************
void Scene::setTime( chrono_t iSeconds )
{
    ABCA_ASSERT( m_archive && m_topObject &&
                 m_drawable && m_drawable->valid(),
                 "Invalid Scene: " << m_fileName );

    if ( m_minTime <= m_maxTime && m_curtime != iSeconds)
    {
        m_drawable->setTime( iSeconds );
        ABCA_ASSERT( m_drawable->valid(),
                     "Invalid drawable after setting time to: "
                     << iSeconds );
        m_curtime = iSeconds;
    }
    
    m_bounds = m_drawable->getBounds();
}

void Scene::setSelectionPath(std::string path)
{
    m_selectionPath = path;
}

//-*****************************************************************************
void Scene::draw( SceneState &s_state )
{

   static MGLFunctionTable *gGLFT = NULL;
   if (gGLFT == NULL)
      gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();

   ABCA_ASSERT( m_archive && m_topObject &&
                 m_drawable && m_drawable->valid(),
                 "Invalid Scene: " << m_fileName );

//    glDrawBuffer( GL_BACK );
//    s_state.cam.apply();

//    glEnable( GL_LIGHTING );
//    setMaterials( 1.0, true );
    
    // Get the matrix
    M44d currentMatrix;
    gGLFT->glGetDoublev( MGL_MODELVIEW_MATRIX, ( MGLdouble * )&(currentMatrix[0][0]) );
    
    DrawContext dctx;
    dctx.setWorldToCamera( currentMatrix );
    dctx.setPointSize( s_state.pointSize );
    dctx.setSelection("");

    m_drawable->draw( dctx );

}

void Scene::drawOnly( SceneState &s_state, std::string selection )
{
       static MGLFunctionTable *gGLFT = NULL;
       if (gGLFT == NULL)
          gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();

       ABCA_ASSERT( m_archive && m_topObject &&
                     m_drawable && m_drawable->valid(),
                     "Invalid Scene: " << m_fileName );

    //    glDrawBuffer( GL_BACK );
    //    s_state.cam.apply();

    //    glEnable( GL_LIGHTING );
    //    setMaterials( 1.0, true );
    
        // Get the matrix
        M44d currentMatrix;
        gGLFT->glGetDoublev( MGL_MODELVIEW_MATRIX, ( MGLdouble * )&(currentMatrix[0][0]) );
    
        DrawContext dctx;
        dctx.setWorldToCamera( currentMatrix );
        dctx.setPointSize( s_state.pointSize );
        dctx.setSelection(selection);
        m_drawable->draw( dctx );

}


} // End namespace SimpleAbcViewer
