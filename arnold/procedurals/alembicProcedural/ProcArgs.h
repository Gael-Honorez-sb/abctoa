//-*****************************************************************************
//
// Copyright (c) 2009-2011,
//  Sony Pictures Imageworks Inc. and
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
// Industrial Light & Magic, nor the names of their contributors may be used
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

#ifndef _Alembic_Arnold_ProcArgs_h_
#define _Alembic_Arnold_ProcArgs_h_

#include <ai.h>
#include <string>
#include <vector>
#include <map>

#include <Alembic/AbcGeom/All.h>

#include "json/json.h"

//-*****************************************************************************

struct ProcArgs
{
    //constructor parses
    ProcArgs( const char * paramStr );

    //copy constructor
    ProcArgs( const ProcArgs &rhs )
    : filename( rhs.filename )
    , nameprefix( rhs.nameprefix )
    , xmlpath ( rhs.xmlpath)
    , objectpath( rhs.objectpath )
    , frame( rhs.frame )
    , fps( rhs.fps )
    , shutterOpen( rhs.shutterOpen )
    , shutterClose( rhs.shutterClose )
    , excludeXform( rhs.excludeXform )
    , inheritXform( rhs.inheritXform )
    , makeInstance( rhs.makeInstance )
    , subdIterations ( rhs.subdIterations )
    , subdivType( rhs.subdivType )
    , subdivAdaptiveMetric (rhs.subdivAdaptiveMetric )
    , subdivPixelError (rhs.subdivPixelError )
    , subdivUvSmoothing (rhs.subdivUvSmoothing )
    , proceduralNode( rhs.proceduralNode )
    {}

    //member variables
    std::string filename;
    std::string nameprefix;
    std::string xmlpath;

    std::string objectpath;
    double frame;
    double fps;
    double shutterOpen;
    double shutterClose;

    bool excludeXform;
    bool inheritXform;

    bool makeInstance;
    int subdIterations;
    int subdivType;

    int subdivAdaptiveMetric;
    float subdivPixelError;
    int subdivUvSmoothing;

    AtNode * proceduralNode;

    std::vector<struct AtNode *> createdNodes;

    bool linkShader;
    bool linkDisplacement;
    bool linkAttributes;

    std::string ns;

    bool useUvArchive;
    Alembic::AbcGeom::IObject uvsRoot;

    std::map<std::string, AtNode*> shaders;
    std::map<std::string, AtNode*> displacements;
    std::vector<std::string> attributes;
    Json::Value attributesRoot;

    bool useAbcShaders;
    Alembic::AbcGeom::IObject materialsObject;
    const char* abcShaderFile;

    void usage();
};

#endif
