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

#include "ArchiveWalker.h"

#include "WriteGeo.h"
#include "WritePoint.h"
#include "WriteCurves.h"
#include "WriteLight.h"

void ArchiveWalker::addArchive(Abc::IArchive iArchive)
{
    ArchiveAndData data;
    data.archive = iArchive;
    mArchives.push_back(data);
}

void ArchiveWalker::getXform(IObject & parent, MatrixSampleMap * xformSamples)
{
    std::auto_ptr<MatrixSampleMap> concatenatedXformSamples;

    if ( IXform::matches( parent.getHeader() ) )
    {
        IXform parentXform(parent, Alembic::Abc::kWrapExisting);
        IXformSchema &xs = parentXform.getSchema();

        if ( xs.getNumOps() > 0 )
        {
            TimeSamplingPtr ts = xs.getTimeSampling();
            size_t numSamples = xs.getNumSamples();

            SampleTimeSet sampleTimes;
            GetRelevantSampleTimes( *args, ts, numSamples, sampleTimes,
                    xformSamples);
            MatrixSampleMap localXformSamples;

            MatrixSampleMap * localXformSamplesToFill = 0;

            concatenatedXformSamples.reset(new MatrixSampleMap);

            if ( !xformSamples )
            {
                // If we don't have parent xform samples, we can fill
                // in the map directly.
                localXformSamplesToFill = concatenatedXformSamples.get();
            }
            else
            {
                //otherwise we need to fill in a temporary map
                localXformSamplesToFill = &localXformSamples;
            }


            for (SampleTimeSet::iterator I = sampleTimes.begin();
                    I != sampleTimes.end(); ++I)
            {
                XformSample sample = parentXform.getSchema().getValue(
                        Abc::ISampleSelector(*I));
                (*localXformSamplesToFill)[(*I)] = sample.getMatrix();
            }
            if ( xformSamples )
            {
                ConcatenateXformSamples(*args,
                        *xformSamples,
                        localXformSamples,
                        *concatenatedXformSamples.get());
            }
            xformSamples = concatenatedXformSamples.get();
        }

    // go upwards
    
    }
    IObject nextParentObject = parent.getParent();
    if ( nextParentObject.valid() )
        getXform(nextParentObject, xformSamples);

}

void ArchiveWalker::exportInstances(WorkUnit & data)
{
    size_t numArchives = mArchives.size();
    for (size_t a = 0; a < numArchives; ++a)
    {
        int objectEnd = std::min(data.end, (int)mArchives[a].instances.size());
        for (int j = data.start; j < objectEnd; ++j)
        {
            IObject object = mArchives[a].instances[j];
            const Abc::ObjectHeader & header = object.getHeader();
            if ( IPolyMesh::matches( header ) )
            {
                // get the parent, which should be a transform
                MatrixSampleMap * xformSamples = NULL;

                Alembic::Abc::IObject parent = object.getParent(); 
               
                getXform(parent, xformSamples);

                IPolyMesh polymesh( object, Alembic::Abc::kWrapExisting );
                ProcessPolyMeshInstance( polymesh, *args, xformSamples);
            }
        }
    }
}

void ArchiveWalker::exportObjects(WorkUnit & data)
{
    size_t numArchives = mArchives.size();
    for (size_t a = 0; a < numArchives; ++a)
    {
        int objectEnd = std::min(data.end, (int)mArchives[a].toExport.size());
        for (int j = data.start; j < objectEnd; ++j)
        {
            IObject object = mArchives[a].toExport[j];
            const Abc::ObjectHeader & header = object.getHeader();
            if ( IPolyMesh::matches( header ) )
            {
                // get the parent, which should be a transform
                //MatrixSampleMap * xformSamples = NULL;

                Alembic::Abc::IObject parent = object.getParent(); 
               
                //getXform(parent, xformSamples);

                IPolyMesh polymesh( object, Alembic::Abc::kWrapExisting );
                ProcessPolyMesh( polymesh, *args);//, xformSamples);
            }
        }
    }
}

void ArchiveWalker::walkObjects(int iArchiveNum, Abc::IObject & iParent)
{
    size_t numChildren = iParent.getNumChildren();
    for (size_t i = 0; i < numChildren; i++)
    {
        const Abc::ObjectHeader & header = iParent.getChildHeader(i);
        Abc::IObject child(iParent, header.getName());

        if ( IPolyMesh::matches( header ) )
        {
            IPolyMesh polymesh( child, Alembic::Abc::kWrapExisting );
            std::string cacheId = GetPolyMeshHash(polymesh, *args);
            

            std::vector<std::string>::iterator It = std::find(g_IObjects.begin(), g_IObjects.end(), cacheId);
               
            if (It != g_IObjects.end())
            {
                AiMsgDebug("Adding %s to instance", child.getFullName().c_str());
                mArchives[iArchiveNum].instances.push_back(child);
            }
            else
            {
                AiMsgDebug("Adding %s to export", child.getFullName().c_str());
                g_IObjects.push_back(cacheId);
                mArchives[iArchiveNum].toExport.push_back(child);
            }
        }


        walkObjects(iArchiveNum, child);
    }
}


void ArchiveWalker::walkArchives(WorkUnit & data)
{
	//printf("walkArchives Work Unit Start %d %d\n", data.start, data.end);

    for (int i = data.start; i < data.end; ++i)
    {
        Abc::IObject top = mArchives[i].archive.getTop();
        walkObjects(i, top);
    }

	//printf("walkArchives Work Unit End %d %d\n", data.start, data.end);
}


unsigned int walkArchivesWrap(void * ptr)
{
    WorkUnit * data = (WorkUnit *)ptr;
    data->walker->walkArchives(*data);
    return 0;
}


unsigned int exportObjectsWrap(void * ptr)
{
    WorkUnit * data = (WorkUnit *)ptr;
    data->walker->exportObjects(*data);
    return 0;
}


unsigned int exportInstancesWrap(void * ptr)
{
    WorkUnit * data = (WorkUnit *)ptr;
    data->walker->exportInstances(*data);
    return 0;
}