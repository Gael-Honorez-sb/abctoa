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
#include "../../common/PathUtil.h"
#include <Alembic/AbcCoreFactory/IFactory.h>
//-*****************************************************************************

namespace AlembicHolder {

//-*****************************************************************************
// SCENE CLASS
//-*****************************************************************************

//-*****************************************************************************
Scene::Scene( const std::vector<std::string> &abcFileNames, const std::string &objectPath )
  : m_fileName( abcFileNames )
  , m_objectPath( objectPath )
  , m_minTime( ( chrono_t )FLT_MAX )
  , m_maxTime( ( chrono_t )-FLT_MAX )
{
    Alembic::AbcCoreFactory::IFactory factory;
	factory.setOgawaNumStreams(16);
    m_archive = factory.getArchive(abcFileNames);

	if (!m_archive.valid())
    {
        std::cout << "[nozAlembicHolder] ERROR : Can't open files" << std::endl;
		return ;
    }

    m_topObject = IObject( m_archive, kTop );


    m_selectionPath = "";

    // try to walk to the path
    PathList path;
    TokenizePath( objectPath, "|", path );

    m_drawable.reset( new IObjectDrw( m_topObject, false, path ) );


    ABCA_ASSERT( m_drawable->valid(),
                 "Invalid drawable for archive" );


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


    std::cout << "[nozAlembicHolder] Opened archives: ";
    for (size_t i = 0; i < abcFileNames.size(); i++)
        std::cout << abcFileNames[i] << std::endl;
    std::cout << "Object path : " << objectPath << std::endl;

}

//-*****************************************************************************
void Scene::setTime( chrono_t iSeconds )
{
    ABCA_ASSERT( m_archive && m_topObject &&
                 m_drawable && m_drawable->valid(),
                 "Invalid Scene");

    if ( m_minTime <= m_maxTime && m_curtime != iSeconds)
    {
        m_drawable->setTime( iSeconds );
        ABCA_ASSERT( m_drawable->valid(),
                     "Invalid drawable after setting time to: "
                     << iSeconds );
        m_curtime = iSeconds;
    }

	m_curFrame = MAnimControl::currentTime().value();

    m_bounds = m_drawable->getBounds();
}

void Scene::setSelectionPath(std::string path)
{
    m_selectionPath = path;
}

int Scene::getNumTriangles() const
{
    return m_drawable->getNumTriangles();

}


//-*****************************************************************************
void Scene::draw( SceneState &s_state, std::string selection, chrono_t iSeconds, holderPrms *m_params, bool flippedNormal)
{

   static MGLFunctionTable *gGLFT = NULL;
   if (gGLFT == NULL)
      gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();

   ABCA_ASSERT( m_archive && m_topObject &&
                 m_drawable && m_drawable->valid(),
                 "Invalid Scene");

   // Check if the holder is still at the right frame.
   if (iSeconds != m_curtime)
	   setTime(iSeconds);

    // Get the matrix
    M44d currentMatrix;
    gGLFT->glGetDoublev( MGL_MODELVIEW_MATRIX, ( MGLdouble * )&(currentMatrix[0][0]) );

    DrawContext dctx;
    dctx.setWorldToCamera( currentMatrix );
    dctx.setPointSize( s_state.pointSize );
    dctx.setSelection(selection);
    dctx.setShaderColors(m_params->shaderColors);
    dctx.setNormalFlipped(flippedNormal);
	dctx.setParams(m_params);
	m_drawable->draw( dctx );

}

} // End namespace AlembicHolder
