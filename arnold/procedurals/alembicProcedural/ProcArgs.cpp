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
#include "ProcArgs.h"

#include <vector>
#include <algorithm>
#include <iostream>

//-*****************************************************************************
//INSERT YOUR OWN TOKENIZATION CODE AND STYLE HERE

ProcArgs::ProcArgs( AtNode *node )
  : frame(0.0)
  , fps(25.0)
  , shutterOpen(0.0)
  , shutterClose(1.0)
  , proceduralNode(node)
  , linkShader(false)
  , linkDisplacement(false)
  , linkAttributes(false)
  , useAbcShaders(false)
{

    // Grab the shutter a camera attached to AiUniverse if present

   AtNode* camera = AiUniverseGetCamera();
   shutterOpen = AiNodeGetFlt(camera, "shutter_start");
   shutterClose = AiNodeGetFlt(camera, "shutter_end");

   AtArray* a_filenames = AiNodeGetArray(node, "fileNames");
   for (uint32_t i = 0; i < AiArrayGetNumElements(a_filenames); i++)
       filenames.push_back(AiArrayGetStr(a_filenames, i).c_str());

   nameprefix = std::string(AiNodeGetStr(node, "namePrefix"));
   objectpath = std::string(AiNodeGetStr(node, "objectPath"));

   frame = AiNodeGetFlt(node, "frame");
   fps = AiNodeGetFlt(node, "fps");

}

