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

#include <cstring>
#include <memory>
#include <vector>

#include "ProcArgs.h"
#include "PathUtil.h"
#include "SampleUtil.h"
#include "WriteGeo.h"
#include "WritePoint.h"
#include "WriteCurves.h"
#include "WriteLight.h"
#include "json/json.h"
#include "parseAttributes.h"

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>

#include <Alembic/AbcCoreFactory/IFactory.h>
#include <Alembic/AbcCoreHDF5/ReadWrite.h>
#include <Alembic/AbcCoreOgawa/ReadWrite.h>
#include <Alembic/AbcGeom/Visibility.h>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>

#ifdef WIN32
#include <boost/atomic/atomic.hpp>
#endif



#include <iostream>
#include <fstream>

namespace
{
using namespace Alembic::AbcGeom;

#ifdef WIN32
boost::atomic<bool> schemaInitialized(false);
#endif

boost::mutex gGlobalLock;
#define GLOBAL_LOCK	   boost::mutex::scoped_lock writeLock( gGlobalLock );

typedef std::map<std::string, IObject> FileCache;
FileCache g_fileCache;

typedef std::vector<std::string> LoadedAss;
LoadedAss g_loadedAss;

typedef std::map<std::string, IObject> LoadedAbcShaders;
LoadedAbcShaders g_abcShaders;


// Recursively copy the values of b into a.
void update(Json::Value& a, Json::Value& b) {
    Json::Value::Members memberNames = b.getMemberNames();
    for (Json::Value::Members::const_iterator it = memberNames.begin();
            it != memberNames.end(); ++it)
    {
        const std::string& key = *it;
        if (a[key].isObject()) {
            update(a[key], b[key]);
        } else {
            a[key] = b[key];
        }
    }
}

void WalkObject( IObject & parent, const ObjectHeader &ohead, ProcArgs &args,
             PathList::const_iterator I, PathList::const_iterator E,
                    MatrixSampleMap * xformSamples)
{
    //Accumulate transformation samples and pass along as an argument
    //to WalkObject
    IObject nextParentObject;
    std::auto_ptr<MatrixSampleMap> concatenatedXformSamples;

    if ( IXform::matches( ohead ) )
    {
        IXform xform( parent, ohead.getName() );
        IXformSchema &xs = xform.getSchema();

        IObject child = IObject( parent, ohead.getName() );

        // also check visibility flags

        if (isVisible(child, xs, &args) == false)
        {}
        else if ( args.excludeXform )
        {
            nextParentObject = child;
        }
        else
        {
            if ( xs.getNumOps() > 0 )
            {
                TimeSamplingPtr ts = xs.getTimeSampling();
                size_t numSamples = xs.getNumSamples();

                SampleTimeSet sampleTimes;
                GetRelevantSampleTimes( args, ts, numSamples, sampleTimes,
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
                    XformSample sample = xform.getSchema().getValue(
                            Abc::ISampleSelector(*I));
                    (*localXformSamplesToFill)[(*I)] = sample.getMatrix();
                }
                if ( xformSamples )
                {
                    ConcatenateXformSamples(args,
                            *xformSamples,
                            localXformSamples,
                            *concatenatedXformSamples.get());
                }


                xformSamples = concatenatedXformSamples.get();
            }

            nextParentObject = xform;
        }
    }
    else if ( ISubD::matches( ohead ) )
    {
        ISubD subd( parent, ohead.getName() );
        ProcessSubD( subd, args, xformSamples );

        nextParentObject = subd;

    }
    else if ( IPolyMesh::matches( ohead ) )
    {
        IPolyMesh polymesh( parent, ohead.getName() );
        
        if(isVisibleForArnold(parent, &args)) // check if the object is invisible for arnold. It is there to avoid skipping the whole hierarchy.
            ProcessPolyMesh( polymesh, args, xformSamples);

        nextParentObject = polymesh; 
    }
    else if ( INuPatch::matches( ohead ) )
    {
        INuPatch patch( parent, ohead.getName() );
        // TODO ProcessNuPatch( patch, args );

        nextParentObject = patch;
    }
    else if ( IPoints::matches( ohead ) )
    {
        IPoints points( parent, ohead.getName() );

        if(isVisibleForArnold(parent, &args))
            ProcessPoint( points, args, xformSamples );

        nextParentObject = points;
    }
    else if ( ICurves::matches( ohead ) )
    {
        ICurves curves( parent, ohead.getName() );

        if(isVisibleForArnold(parent, &args))
            ProcessCurves( curves, args, xformSamples );

        nextParentObject = curves;
    }
    else if ( ICamera::matches( ohead ) )
    {
        ICamera camera( parent, ohead.getName() );

        nextParentObject = camera;
    }
    else if ( ILight::matches( ohead ) )
    {
        ILight light( parent, ohead.getName() );
        
        if(isVisibleForArnold(parent, &args)) // check if the object is invisible for arnold. It is there to avoid skipping the whole hierarchy.
            ProcessLight( light, args, xformSamples);

        nextParentObject = light;
    }
    else if ( IFaceSet::matches( ohead ) )
    {
        //don't complain about discovering a faceset upon traversal
    }
    else
    {

        AiMsgError("could not determine type of %s", ohead.getName().c_str());
        AiMsgError("%s has MetaData: %s", ohead.getName().c_str(), ohead.getMetaData().serialize().c_str());
        if ( IXform::matches( ohead ) )
            AiMsgError("but we are matching");
        nextParentObject = parent.getChild(ohead.getName());
    }

    if ( nextParentObject.valid() )
    {
        //std::cerr << nextParentObject.getFullName() << std::endl;

        if ( I == E )
        {
            for ( size_t i = 0; i < nextParentObject.getNumChildren() ; ++i )
            {
                
                WalkObject( nextParentObject,
                            nextParentObject.getChildHeader( i ),
                            args, I, E, xformSamples);
            }
        }
        else
        {
            const ObjectHeader *nextChildHeader =
                nextParentObject.getChildHeader( *I );
            
            if ( nextChildHeader != NULL )
            {
                WalkObject( nextParentObject, *nextChildHeader, args, I+1, E,
                    xformSamples);
            }
        }
    }



}

//-*************************************************************************

int ProcInit( struct AtNode *node, void **user_ptr )
{
#ifdef WIN32
    // DIRTY FIX 
    // magic static* used in the Alembic Schemas are not threadSafe in Visual Studio, so we need to initialized them first.

    if(!schemaInitialized)
    {
        IPolyMesh::getSchemaTitle();
        IPoints::getSchemaTitle();
        ICurves::getSchemaTitle();
        INuPatch::getSchemaTitle();
        IXform::getSchemaTitle();
        ISubD::getSchemaTitle();
        schemaInitialized = true;
    }
#endif

    boost::mutex::scoped_lock writeLock( gGlobalLock );
    writeLock.unlock();

    bool skipJson = false;
    bool skipShaders = false;
    bool skipAttributes = false;
    bool skipDisplacement = false;
    bool skipLayers = false;

    bool customLayer = false;
    std::string layer = "";

    AtNode* options = AiUniverseGetOptions ();
    if (AiNodeLookUpUserParameter(options, "render_layer") != NULL )
    {
        layer = std::string(AiNodeGetStr(options, "render_layer"));
        if(layer != std::string("defaultRenderLayer"))
            customLayer = true;
    }




    if (AiNodeLookUpUserParameter(node, "skipJsonFile") != NULL )
        skipJson = AiNodeGetBool(node, "skipJsonFile");
    if (AiNodeLookUpUserParameter(node, "skipShaders") != NULL )
        skipShaders = AiNodeGetBool(node, "skipShaders");
    if (AiNodeLookUpUserParameter(node, "skipAttributes") != NULL )
        skipAttributes = AiNodeGetBool(node, "skipAttributes");
    if (AiNodeLookUpUserParameter(node, "skipDisplacements") != NULL )
        skipDisplacement = AiNodeGetBool(node, "skipDisplacements");
    if (AiNodeLookUpUserParameter(node, "skipAttributes") != NULL )
        skipAttributes = AiNodeGetBool(node, "skipAttributes");
    if (AiNodeLookUpUserParameter(node, "skipLayers") != NULL )
        skipLayers = AiNodeGetBool(node, "skipLayers");

    ProcArgs * args = new ProcArgs( AiNodeGetStr( node, "data" ) );
    args->proceduralNode = node;

    if (AiNodeLookUpUserParameter(node, "assShaders") !=NULL )
    {
        const char* assfile = AiNodeGetStr(node, "assShaders");
        if(*assfile != 0)
        {
            writeLock.lock();
            // if we don't find the ass file, we can load it. This avoid multiple load of the same file.
            if(std::find(g_loadedAss.begin(), g_loadedAss.end(), std::string(assfile)) == g_loadedAss.end())
            {
                
                if(AiASSLoad(assfile, AI_NODE_SHADER) == 0)
                    g_loadedAss.push_back(std::string(assfile));
                
            }
            writeLock.unlock();

        }
    }

    if (AiNodeLookUpUserParameter(node, "abcShaders") !=NULL )
    {
        const char* abcfile = AiNodeGetStr(node, "abcShaders");

        writeLock.lock();
        FileCache::iterator I = g_abcShaders.find(abcfile);
        if (I != g_abcShaders.end())
        {
            args->useAbcShaders = true;
            args->materialsObject = (*I).second;
        }

        else
        {
            Alembic::AbcCoreFactory::IFactory factory;
            IArchive archive = factory.getArchive(abcfile);
            if (!archive.valid())
            {
                AiMsgWarning ( "Cannot read file %s", abcfile);
            }
            else
            {
                AiMsgDebug ( "reading file %s", abcfile);
                Abc::IObject materialsObject(archive.getTop(), "materials");
                g_abcShaders[args->filename] = materialsObject;
                args->useAbcShaders = true;
                args->materialsObject = materialsObject;
                args->abcShaderFile = abcfile;
            }
        }
        writeLock.unlock();
    }

    // check if we have a UV archive attribute
    if (AiNodeLookUpUserParameter(node, "uvsArchive") !=NULL )
    {
        // if so, we try to load the archive.
        IArchive archive;
        Alembic::AbcCoreFactory::IFactory factory;
        archive = factory.getArchive(AiNodeGetStr(node, "uvsArchive"));
        if (!archive.valid())
        {
            AiMsgWarning ( "Cannot read file %s", AiNodeGetStr(node, "uvsArchive"));
        }
        else
        {
            AiMsgDebug ( "Using UV archive %s", AiNodeGetStr(node, "uvsArchive"));
            args->useUvArchive = true;
            args->uvsRoot = archive.getTop();
        }
    }


    Json::Value jrootShaders;
    Json::Value jrootattributes;
    Json::Value jrootDisplacements;
    Json::Value jrootLayers;

    bool parsingSuccessful = false;

    if (AiNodeLookUpUserParameter(node, "jsonFile") !=NULL && skipJson == false)
    {
        Json::Value jroot;
        Json::Reader reader;
        std::ifstream test(AiNodeGetStr(node, "jsonFile"), std::ifstream::binary);
        parsingSuccessful = reader.parse( test, jroot, false );

        if (AiNodeLookUpUserParameter(node, "secondaryJsonFile") !=NULL)
        {
            std::ifstream test2(AiNodeGetStr(node, "secondaryJsonFile"), std::ifstream::binary);
            Json::Value jroot2;
            if (reader.parse( test2, jroot2, false ))
                update(jroot, jroot2);
        }
        
        if ( parsingSuccessful )
        {
            if(skipShaders == false)
            {
                if(jroot["namespace"].isString())
                    args->ns = jroot["namespace"].asString() + ":";

                jrootShaders = jroot["shaders"];
                if (AiNodeLookUpUserParameter(node, "shadersAssignation") !=NULL)
                {
                    Json::Reader readerOverride;
                    Json::Value jrootShadersattributes;
                    std::vector<std::string> pathattributes;
                    if(readerOverride.parse( AiNodeGetStr(node, "shadersAssignation"), jrootShadersattributes ))
                        if(jrootShadersattributes.size() > 0)
                            jrootShaders = OverrideAssignations(jrootShaders, jrootShadersattributes);
                }
            }


            if(skipAttributes == false)
            {
                jrootattributes = jroot["attributes"];

                if (AiNodeLookUpUserParameter(node, "attributes") !=NULL)
                {
                    Json::Reader readerOverride;
                    Json::Value jrootattributesattributes;

                    if(readerOverride.parse( AiNodeGetStr(node, "attributes"), jrootattributesattributes))
                        OverrideProperties(jrootattributes, jrootattributesattributes);

                }
            }


            if(skipDisplacement == false)
            {
                jrootDisplacements = jroot["displacement"];
                if (AiNodeLookUpUserParameter(node, "displacementsAssignation") !=NULL)
                {
                    Json::Reader readerOverride;
                    Json::Value jrootDisplacementsattributes;

                    if(readerOverride.parse( AiNodeGetStr(node, "displacementsAssignation"), jrootDisplacementsattributes ))
                        if(jrootDisplacementsattributes.size() > 0)
                            jrootDisplacements = OverrideAssignations(jrootDisplacements, jrootDisplacementsattributes);
                }
            }

            if(skipLayers == false && customLayer)
            {
                jrootLayers = jroot["layers"];
                if (AiNodeLookUpUserParameter(node, "layersOverride") !=NULL)
                {
                    Json::Reader readerOverride;
                    Json::Value jrootLayersattributes;

                    if(readerOverride.parse( AiNodeGetStr(node, "layersOverride"), jrootLayersattributes ))
                    {
                        jrootLayers[layer]["removeShaders"] = jrootLayersattributes[layer].get("removeShaders", skipShaders).asBool();
                        jrootLayers[layer]["removeDisplacements"] = jrootLayersattributes[layer].get("removeDisplacements", skipDisplacement).asBool();
                        jrootLayers[layer]["removeProperties"] = jrootLayersattributes[layer].get("removeProperties", skipAttributes).asBool();

                        if(jrootLayersattributes[layer]["shaders"].size() > 0)
                            jrootLayers[layer]["shaders"] = OverrideAssignations(jrootLayers[layer]["shaders"], jrootLayersattributes[layer]["shaders"]);

                        if(jrootLayersattributes[layer]["displacements"].size() > 0)
                            jrootLayers[layer]["displacements"] = OverrideAssignations(jrootLayers[layer]["displacements"], jrootLayersattributes[layer]["displacements"]);

                        if(jrootLayersattributes[layer]["properties"].size() > 0)
                            OverrideProperties(jrootLayers[layer]["properties"], jrootLayersattributes[layer]["properties"]);
                    }
                }
            }

        }
    }

    if(!parsingSuccessful)
    {
        if (customLayer && AiNodeLookUpUserParameter(node, "layersOverride") !=NULL)
        {
            Json::Reader reader;
            bool parsingSuccessful = reader.parse( AiNodeGetStr(node, "layersOverride"), jrootLayers );
        }
        // Check if we have to skip something....
        if( jrootLayers[layer].size() > 0 && customLayer)
        {
            skipShaders = jrootLayers[layer].get("removeShaders", skipShaders).asBool();
            skipDisplacement = jrootLayers[layer].get("removeDisplacements", skipDisplacement).asBool();
            skipAttributes =jrootLayers[layer].get("removeProperties", skipAttributes).asBool();
        }

        if (AiNodeLookUpUserParameter(node, "shadersAssignation") !=NULL && skipShaders == false)
        {
            Json::Reader reader;
            bool parsingSuccessful = reader.parse( AiNodeGetStr(node, "shadersAssignation"), jrootShaders );
        }

        if (AiNodeLookUpUserParameter(node, "attributes") !=NULL  && skipAttributes == false)
        {
            Json::Reader reader;
            bool parsingSuccessful = reader.parse( AiNodeGetStr(node, "attributes"), jrootattributes );
        }
        if (AiNodeLookUpUserParameter(node, "displacementsAssignation") !=NULL  && skipDisplacement == false)
        {
            Json::Reader reader;
            bool parsingSuccessful = reader.parse( AiNodeGetStr(node, "displacementsAssignation"), jrootDisplacements );
        }
    }


    if( jrootLayers[layer].size() > 0 && customLayer)
    {
        if(jrootLayers[layer]["shaders"].size() > 0)
        {
            if(jrootLayers[layer].get("removeShaders", skipShaders).asBool())
                jrootShaders = jrootLayers[layer]["shaders"];
            else
                jrootShaders = OverrideAssignations(jrootShaders, jrootLayers[layer]["shaders"]);
        }

        if(jrootLayers[layer]["displacements"].size() > 0)
        {
            if(jrootLayers[layer].get("removeDisplacements", skipDisplacement).asBool())
                jrootDisplacements = jrootLayers[layer]["displacements"];
            else
                jrootDisplacements = OverrideAssignations(jrootDisplacements, jrootLayers[layer]["displacements"]);
        }

        if(jrootLayers[layer]["properties"].size() > 0)
        {
            if(jrootLayers[layer].get("removeProperties", skipAttributes).asBool())
                jrootattributes = jrootLayers[layer]["properties"];
            else
                OverrideProperties(jrootattributes, jrootLayers[layer]["properties"]);
        }
    }

    // If shaderNamespace attribute is set it has priority
    if (AiNodeLookUpUserParameter(node, "shadersNamespace") !=NULL)
    {
        const char* shadersNamespace = AiNodeGetStr(node, "shadersNamespace");
        if (shadersNamespace && strlen(shadersNamespace))
            args->ns = std::string(shadersNamespace) + ":";
    }

    //Check displacements

    if( jrootDisplacements.size() > 0 )
    {
        args->linkDisplacement = true;
        ParseShaders(jrootDisplacements, args->ns, args->nameprefix, args, 0);
    }


    // Check if we can link shaders or not.
    if( jrootShaders.size() > 0 )
    {
        args->linkShader = true;
        ParseShaders(jrootShaders, args->ns, args->nameprefix, args, 1);
    }


    if( jrootattributes.size() > 0 )
    {
        args->linkAttributes = true;
        args->attributesRoot = jrootattributes;
        for( Json::ValueIterator itr = jrootattributes.begin() ; itr != jrootattributes.end() ; itr++ )
        {
            std::string path = itr.key().asString();
            args->attributes.push_back(path);

        }
        std::sort(args->attributes.begin(), args->attributes.end());
    }


    *user_ptr = args;

    #if (AI_VERSION_ARCH_NUM == 3 && AI_VERSION_MAJOR_NUM < 3) || AI_VERSION_ARCH_NUM < 3
        #error Arnold version 3.3+ required for AlembicArnoldProcedural
    #endif

        if (!AiCheckAPIVersion(AI_VERSION_ARCH, AI_VERSION_MAJOR, AI_VERSION_MINOR))
        {
            std::cout << "AlembicArnoldProcedural compiled with arnold-"
                      << AI_VERSION
                      << " but is running with incompatible arnold-"
                      << AiGetVersion(NULL, NULL, NULL, NULL) << std::endl;
            return 1;
        }

    if ( args->filename.empty() )
    {
        args->usage();
        return 1;
    }


    
    IObject root;

    writeLock.lock();
    FileCache::iterator I = g_fileCache.find(args->filename);
    if (I != g_fileCache.end())
        root = (*I).second;

    else
    {
        Alembic::AbcCoreFactory::IFactory factory;
        IArchive archive = factory.getArchive(args->filename);
        if (!archive.valid())
        {
            AiMsgError ( "Cannot read file %s", args->filename.c_str());
        }
        else
        {
            AiMsgDebug ( "reading file %s", args->filename.c_str());
            g_fileCache[args->filename] = archive.getTop();
            root = archive.getTop();
        }

    }
    writeLock.unlock();

    PathList path;
    TokenizePath( args->objectpath, path );

    try
    {
        if ( path.empty() ) //walk the entire scene
        {
            for ( size_t i = 0; i < root.getNumChildren(); ++i )
            {
                std::vector<std::string> tags;
                WalkObject( root, root.getChildHeader(i), *args,
                            path.end(), path.end(), 0 );
            }
        }
        else //walk to a location + its children
        {
            PathList::const_iterator I = path.begin();

            const ObjectHeader *nextChildHeader =
                    root.getChildHeader( *I );
            if ( nextChildHeader != NULL )
            {
                std::vector<std::string> tags;
                WalkObject( root, *nextChildHeader, *args, I+1,
                        path.end(), 0);
            }
        }
    }
    catch ( const std::exception &e )
    {
        AiMsgError("exception thrown during ProcInit: %s", e.what());
    }
    catch (...)
    {
        AiMsgError("exception thrown");
    }

    return 1;
}

//-*************************************************************************

int ProcCleanup( void *user_ptr )
{
    delete reinterpret_cast<ProcArgs*>( user_ptr );
    return 1;
}

//-*************************************************************************

int ProcNumNodes( void *user_ptr )
{
    ProcArgs * args = reinterpret_cast<ProcArgs*>( user_ptr );
    return (int) args->createdNodes.size();

}

//-*************************************************************************

struct AtNode* ProcGetNode(void *user_ptr, int i)
{

    ProcArgs * args = reinterpret_cast<ProcArgs*>( user_ptr );

    if ( i >= 0 && i < (int) args->createdNodes.size() )
    {
        return args->createdNodes[i];
    }

    return NULL;
}

} //end of anonymous namespace


#ifdef __cplusplus
extern "C"
{
#endif

AI_EXPORT_LIB int ProcLoader(AtProcVtable *vtable)
// vtable passed in by proc_loader macro define
{
   vtable->Init     = ProcInit;
   vtable->Cleanup  = ProcCleanup;
   vtable->NumNodes = ProcNumNodes;
   vtable->GetNode  = ProcGetNode;
   strcpy(vtable->version, AI_VERSION);
   return 1;
}

#ifdef __cplusplus
}
#endif

