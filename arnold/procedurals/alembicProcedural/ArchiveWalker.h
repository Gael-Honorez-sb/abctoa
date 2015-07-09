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

#ifndef _Alembic_Arnold_ArchiveWalker_h_
#define _Alembic_Arnold_ArchiveWalker_h_

#include "ProcArgs.h"
#include "SampleUtil.h"

#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/AbcGeom/All.h>

#include <algorithm>
#include <vector>

//using namespace Alembic;
using namespace Alembic::AbcGeom;

namespace
{
    // Arnold scene build is single-threaded so we don't have to lock around
    // access to this for now.
    typedef std::vector<std::string> IObjects;
    IObjects g_IObjects;

}

struct ArchiveAndData
{
    Abc::IArchive archive;
    std::vector< Abc::IObject > instances;
    std::vector< Abc::IObject > toExport;
};

class ArchiveWalker;

struct WorkUnit
{
    int start;
    int end;
    int archive;
    ArchiveWalker * walker;
};

class ArchiveWalker
{
    public:

        ProcArgs *args;
        std::vector< ArchiveAndData > mArchives;

        void addArchive(Abc::IArchive iArchive);
        void exportObjects(WorkUnit & data);
        void exportInstances(WorkUnit & data);
        void walkObjects(int iArchiveNum, Abc::IObject & iParent);
        void walkArchives(WorkUnit & data);
        void getXform(IObject & parent, MatrixSampleMap * xformSamples);
        //size_t getNumObjects(int iArchiveNum) { return mArchives[iArchiveNum].objects.size(); };
        

};

unsigned int walkArchivesWrap(void * ptr);
unsigned int exportObjectsWrap(void * ptr);
unsigned int exportInstancesWrap(void * ptr);

#endif