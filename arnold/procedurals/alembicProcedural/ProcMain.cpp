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
#include "getBounds.h"
#include "PathUtil.h"
#include "SampleUtil.h"
#include "WriteGeo.h"
#include "WritePoint.h"
#include "WriteLight.h"
#include "WriteCurves.h"
#include "json/json.h"
#include "parseAttributes.h"
#include "NodeCache.h"

#include "ReadInstancer.h"

#include <Alembic/AbcGeom/All.h>

#include <Alembic/AbcCoreFactory/IFactory.h>
#include <Alembic/AbcCoreOgawa/ReadWrite.h>
#include <Alembic/AbcGeom/Visibility.h>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>


#include <iostream>
#include <fstream>


namespace
{
using namespace Alembic::AbcGeom;


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





void WalkObject( IObject & parent, const ObjectHeader &i_ohead, ProcArgs &args,
             PathList::const_iterator I, PathList::const_iterator E,
                    MatrixSampleMap * xformSamples)
{
    IObject nextParentObject;
	
    std::auto_ptr<MatrixSampleMap> concatenatedXformSamples;

    // Check for instances
    const ObjectHeader& ohead = parent.isChildInstance(i_ohead.getName()) ? parent.getChild(i_ohead.getName()).getHeader() : i_ohead;

    if ( IXform::matches( ohead ) )
    {
        IXform xform( parent, ohead.getName() );
        IXformSchema &xs = xform.getSchema();

        IObject child = IObject( parent, ohead.getName() );

        // also check visibility flags

        if (isVisible(child, xs, &args) == false)
        {
		}
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
//        nextParentObject = parent.getChild(ohead.getName());
    }

    if ( nextParentObject.valid() )
    {
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

struct caches
{
    FileCache* g_fileCache;
    NodeCache* g_nodeCache;
    AtCritSec mycs;
};

bool ProcInitPlugin(void **plugin_user_ptr)
{
#ifdef WIN32
    // DIRTY FIX 
    // magic static* used in the Alembic Schemas are not threadSafe in Visual Studio, so we need to initialized them first.
        IPolyMesh::getSchemaTitle();
        IPoints::getSchemaTitle();
        ICurves::getSchemaTitle();
        INuPatch::getSchemaTitle();
        IXform::getSchemaTitle();
        ISubD::getSchemaTitle();
#endif

        

        caches *g_caches = new caches();
        AiCritSecInitRecursive(&g_caches->mycs);

        g_caches->g_fileCache = new FileCache(g_caches->mycs);
        g_caches->g_nodeCache = new NodeCache(g_caches->mycs);
        *plugin_user_ptr = g_caches;
        return true;
}


bool ProcCleanupPlugin(void *plugin_user_ptr)
{
    caches *g_caches = reinterpret_cast<caches*>( plugin_user_ptr );
    AiCritSecClose(&g_caches->mycs);
    delete g_caches->g_fileCache;
    delete g_caches->g_nodeCache;
    delete g_caches;
    return true;

}


bool ProcInitBounds (AtNode *node, AtBBox *bounds, void **user_ptr)
{
    // check if we have a instancer archive attribute
    if (AiNodeLookUpUserParameter(node, "instancerArchive") !=NULL )
    {
        std::string fileName(AiNodeGetStr(node, "instancerArchive"));
        if(fileName.empty())
        {
            // if we already have BBox, we skip this part.
            if( bounds->min.x != 0.0 || bounds->min.y != 0.0 || bounds->min.z != 0.0 ||
                bounds->max.x != 0.0 || bounds->max.y != 0.0 || bounds->max.z != 0.0
            )
                return true;
        }
    }


    ProcArgs * args = new ProcArgs( AiNodeGetStr( node, "data" ) );
    std::string resultFile = args->filename;
    if (AiNodeLookUpUserParameter(node, "instancerArchive") !=NULL )
    {
        std::string fileName(AiNodeGetStr(node, "instancerArchive"));
        if(!fileName.empty())
        {
            resultFile = fileName;
        }
    }

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

    if (resultFile.empty() )
    {
        args->usage();
        return 1;
    }


    IObject root;
    Alembic::AbcCoreFactory::IFactory factory;
    IArchive archive = factory.getArchive(resultFile);

    
    if (!archive.valid())
    {
        AiMsgError ( "Cannot read file %s", resultFile.c_str());
    }
    else
    {
        AiMsgDebug ( "reading file %s", resultFile.c_str());
        root = archive.getTop();
    }

    Box3d bbox;
    bbox.makeEmpty();
    chrono_t frameTime = args->frame / args->fps;
    chrono_t shutterOpenTime = ( args->frame + args->shutterOpen ) / args->fps;
    chrono_t shutterCloseTime = ( args->frame + args->shutterClose ) / args->fps;

     if ( root.getProperties().getPropertyHeader( ".childBnds" ) != NULL )
     {
         IBox3dProperty childbnds = Alembic::Abc::IBox3dProperty( archive.getTop().getProperties(),
                                   ".childBnds", ErrorHandler::kQuietNoopPolicy);
        

         ISampleSelector m_ss =  ISampleSelector(shutterOpenTime, ISampleSelector::kNearIndex );
         Box3d bboxRoot = childbnds.getValue(m_ss);
         bbox.extendBy(bboxRoot);
         if(shutterOpenTime != shutterCloseTime)
         {
              m_ss =  ISampleSelector(shutterOpenTime, ISampleSelector::kNearIndex );
              bboxRoot = childbnds.getValue(m_ss);
              bbox.extendBy(bboxRoot);
         }
     }
     else
     {
        getBoundingBox(root, shutterOpenTime, bbox);
        if(shutterOpenTime != shutterCloseTime)
            getBoundingBox(root, shutterCloseTime, bbox);
     }

    bounds->min = AiPoint(bbox.min.x, bbox.min.y, bbox.min.z);
    bounds->max = AiPoint(bbox.max.x, bbox.max.y, bbox.max.z);

    AiMsgDebug("bounds->min %f %f %f", bounds->min.x, bounds->min.y, bounds->min.z);
    AiMsgDebug("bounds->max %f %f %f", bounds->max.x, bounds->max.y, bounds->max.z);


    return true;
}

//-*************************************************************************

int ProcInit( struct AtNode *node, void **user_ptr )
{

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
    *user_ptr = args;

    
	if(const char* env_p = std::getenv("PATH_REMAPPING")) // TODO: Read from json file and procedural parameters & merge them all.
	{
		Json::Reader pathRemappingReader;
        Json::Value pathRemappingReaderattributes;

		if(pathRemappingReader.parse( env_p, pathRemappingReaderattributes ))
		{
			for( Json::ValueIterator itr = pathRemappingReaderattributes.begin() ; itr != pathRemappingReaderattributes.end() ; itr++ )
			{
				std::string path1 = itr.key().asString();
				std::string path2 = pathRemappingReaderattributes[itr.key().asString()].asString();
				AiMsgDebug("Remapping %s to %s", path1.c_str(), path2.c_str());
				args->pathRemapping[path1] = path2;
			}
		}
	}
		

    caches *g_cache = reinterpret_cast<caches*>( AiProceduralGetPluginData(node) );
    
    args->proceduralNode = node;
    args->nodeCache = g_cache->g_nodeCache;
    args->lock = g_cache->mycs;
    args->createdNodes = new NodeCollector(args->lock, node);

    if (AiNodeLookUpUserParameter(node, "abcShaders") !=NULL )
    {
        const char* abcfile = AiNodeGetStr(node, "abcShaders");

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
            args->useAbcShaders = true;
            args->materialsObject = materialsObject;
            args->abcShaderFile = abcfile;
        }


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


    // check if we have a instancer archive attribute
    if (AiNodeLookUpUserParameter(node, "instancerArchive") !=NULL )
    {
        std::string fileName(AiNodeGetStr(node, "instancerArchive"));
        if(!fileName.empty())
        {
            // if so, we try to load the archive.
            IArchive archive;
            Alembic::AbcCoreFactory::IFactory factory;
            archive = factory.getArchive(AiNodeGetStr(node, "instancerArchive"));
            if (!archive.valid())
            {
                AiMsgWarning ( "Cannot read file %s", AiNodeGetStr(node, "instancerArchive"));
            }
            else
            {
                AiMsgInfo( "Using Instancer archive %s", AiNodeGetStr(node, "instancerArchive"));

                IObject root = archive.getTop();
                PathList path;

                if ( path.empty() ) //walk the entire scene
                {
                    for ( size_t i = 0; i < root.getNumChildren(); ++i )
                    {
                        std::vector<std::string> tags;
                        WalkObjectForInstancer( root, root.getChildHeader(i), *args,
                                    path.end(), path.end(), 0 );
                    }
                }
            
                return 1;
            }
        }
    }


    std::string fileCacheId = g_cache->g_fileCache->getHash(args->filename, args->shaders, args->displacements, args->attributesRoot, args->frame);

    const std::vector<CachedNodeFile>& createdNodes = g_cache->g_fileCache->getCachedFile(fileCacheId);
    
    if (!createdNodes.empty())
    {
        AiMsgDebug("Found cache of size %i", createdNodes.size());
        for(int i = 0; i <  createdNodes.size(); i++)
        {
            AiMsgDebug("Instancing obj %i", i);
            CachedNodeFile cachedNode = createdNodes[i];
            AtNode *obj = cachedNode.node;
            
            AiMsgDebug("Getting obj %i %s and type %s", i, AiNodeGetName(obj), AiNodeEntryGetName(AiNodeGetNodeEntry (obj)));
            if(AiNodeEntryGetType(AiNodeGetNodeEntry(obj)) == AI_NODE_SHAPE)
            {
                AtNode *instance = AiNode("ginstance");
                AiNodeSetBool(instance, "inherit_xform", false);
                AiNodeSetPtr(instance, "node", obj);
                AiNodeSetArray(instance, "matrix", AiArrayCopy(cachedNode.matrix));
                std::string newName = args->nameprefix + "/" + std::string(AiNodeGetName(obj));
                AiNodeSetStr(instance, "name", newName.c_str());
                
                // Now copy original properties
                AiNodeSetByte(instance, "visibility", AiNodeGetByte(obj, "visibility"));
                AiNodeSetByte(instance, "sidedness", AiNodeGetByte(obj, "sidedness"));
                AiNodeSetBool(instance, "receive_shadows", AiNodeGetBool(obj, "receive_shadows"));
                AiNodeSetBool(instance, "self_shadows", AiNodeGetBool(obj, "self_shadows"));
                AiNodeSetBool(instance, "invert_normals", AiNodeGetBool(obj, "invert_normals"));
                AiNodeSetBool(instance, "opaque", AiNodeGetBool(obj, "opaque"));
                AiNodeSetBool(instance, "matte", AiNodeGetBool(obj, "matte"));
                AiNodeSetBool(instance, "use_light_group", AiNodeGetBool(obj, "use_light_group"));

                AiNodeSetArray(instance, "light_group", AiArrayCopy(AiNodeGetArray(obj, "light_group")));
                AiNodeSetArray(instance, "shadow_group", AiArrayCopy(AiNodeGetArray(obj, "shadow_group")));
                AiNodeSetArray(instance, "transform_time_samples", AiArrayCopy(AiNodeGetArray(obj, "transform_time_samples")));
                AiNodeSetArray(instance, "deform_time_samples", AiArrayCopy(AiNodeGetArray(obj, "deform_time_samples")));
                AiNodeSetArray(instance, "light_group", AiArrayCopy(AiNodeGetArray(obj, "light_group")));

                args->createdNodes->addNode(instance);
            }
            else if (AiNodeEntryGetType(AiNodeGetNodeEntry(obj)) == AI_NODE_LIGHT)
            {
                // AiNodeClone seems to crash arnold when releasing ressources. So we clone the node ourself.

                const AtNodeEntry* nentry = AiNodeGetNodeEntry(obj);
                AtNode* light = AiNode(AiNodeEntryGetName(nentry));

                for (int i = 0; i < AiNodeEntryGetNumParams (nentry); i++)
                {
                    const AtParamEntry* pentry = AiNodeEntryGetParameter (nentry, i);

                    switch(AiParamGetType (pentry))
                    {
                        
                        case AI_TYPE_BYTE:
                            AiNodeSetByte(light, AiParamGetName(pentry), AiNodeGetByte(obj, AiParamGetName(pentry)));
                            break;
                        case AI_TYPE_INT:
                        case AI_TYPE_ENUM:
                            AiNodeSetInt(light, AiParamGetName(pentry), AiNodeGetInt(obj, AiParamGetName(pentry)));
                            break;
                        case AI_TYPE_BOOLEAN:
                            AiNodeSetBool(light, AiParamGetName(pentry), AiNodeGetBool(obj, AiParamGetName(pentry)));
                            break;
                        case AI_TYPE_FLOAT:
                            AiNodeSetFlt(light, AiParamGetName(pentry), AiNodeGetFlt(obj, AiParamGetName(pentry)));
                            break;
                        case AI_TYPE_RGB:
                            {
                                AtRGB col = AiNodeGetRGB(obj, AiParamGetName(pentry));
                                AiNodeSetRGB(light, AiParamGetName(pentry), col.r, col.g, col.b);
                                break;
                            }
                        case AI_TYPE_RGBA:
                            {
                                AtRGBA colRGBA = AiNodeGetRGBA(obj, AiParamGetName(pentry));
                                AiNodeSetRGBA(light, AiParamGetName(pentry), colRGBA.r, colRGBA.g, colRGBA.b, colRGBA.a);
                                break;
                            }
                        case AI_TYPE_VECTOR:
                            {
                                AtVector vec = AiNodeGetVec(obj, AiParamGetName(pentry));
                                AiNodeSetVec(light, AiParamGetName(pentry), vec.x, vec.y, vec.z);
                                break;
                            }
                        case AI_TYPE_POINT:
                            {
                                AtPoint pnt = AiNodeGetPnt(obj, AiParamGetName(pentry));
                                AiNodeSetPnt(light, AiParamGetName(pentry), pnt.x, pnt.y, pnt.z);
                                break;
                            }
                        case AI_TYPE_POINT2:
                            {
                                AtPoint2 pnt2 = AiNodeGetPnt2(obj, AiParamGetName(pentry));
                                AiNodeSetPnt2(light, AiParamGetName(pentry), pnt2.x, pnt2.y);
                                break;
                            }
                        case AI_TYPE_STRING:
                            if(strcmp(AiParamGetName(pentry), "name") != 0)
                                AiNodeSetStr(light, AiParamGetName(pentry), AiNodeGetStr(obj, AiParamGetName(pentry)));
                            break;
                        case AI_TYPE_POINTER:
                        case AI_TYPE_NODE:
                            AiNodeSetPtr(light, AiParamGetName(pentry), AiNodeGetPtr(obj, AiParamGetName(pentry)));
                            break;
                        case AI_TYPE_ARRAY:
                            {
                                if(strcmp(AiParamGetName(pentry), "matrix") != 0)
                                    AiNodeSetArray(light, AiParamGetName(pentry), AiArrayCopy(AiNodeGetArray(obj, AiParamGetName(pentry))));
                                break;
                            }
                        default:
                            break;



                    }

                }
                
                AiNodeSetArray(light, "matrix", AiArrayCopy(cachedNode.matrix));
                std::string newName = args->nameprefix + "/" + std::string(AiNodeGetName(obj));
                AiNodeSetStr(light, "name", newName.c_str());
                args->createdNodes->addNode(light);
            }


        }
        return 1;
    }

    Alembic::AbcCoreFactory::IFactory factory;
    factory.setOgawaNumStreams(8);
    IArchive archive = factory.getArchive(args->filename);
    
    if (!archive.valid())
    {
        AiMsgError ( "Cannot read file %s", args->filename.c_str());
        return 0;
    }
    else
    {
        AiMsgDebug ( "reading file %s", args->filename.c_str());
    }

    IObject root = archive.getTop();
    PathList path;
    TokenizePath( args->objectpath, path );

    //try
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
    /*catch ( const std::exception &e )
    {
        AiMsgError("exception thrown during ProcInit: %s", e.what());
    }
    catch (...)
    {
        AiMsgError("exception thrown");
    }
    */
    //g_cache->g_fileCache->removeFromOpenedFiles(args->filename);
    return 1;
}

//-*************************************************************************

int ProcCleanup( void *user_ptr )
{
    AiMsgDebug("ProcCleanup");
    //delete reinterpret_cast<ProcArgs*>( user_ptr );
    ProcArgs * args = reinterpret_cast<ProcArgs*>( user_ptr );
    if(args != NULL)
    {

        if(args->createdNodes->getNumNodes() > 0)
        {
            caches *g_cache = reinterpret_cast<caches*>( AiProceduralGetPluginData(args->proceduralNode) );

            std::string fileCacheId = g_cache->g_fileCache->getHash(args->filename, args->shaders, args->displacements, args->attributesRoot, args->frame);
            g_cache->g_fileCache->addCache(fileCacheId, args->createdNodes);
        }

        args->shaders.clear();
        args->displacements.clear();
        args->attributes.clear();
        delete args->createdNodes;
        delete args;
    }
    AiMsgDebug("ProcCleanup done");
    
    return 1;
}

//-*************************************************************************

int ProcNumNodes( void *user_ptr )
{

    ProcArgs * args = reinterpret_cast<ProcArgs*>( user_ptr );
    AiMsgDebug("got %i nodes", args->createdNodes->getNumNodes());
    return (int) args->createdNodes->getNumNodes();

}

//-*************************************************************************

AtNode* ProcGetNode(void *user_ptr, int i)
{
    
    AiMsgDebug("Should return node %i", i);
    ProcArgs * args = reinterpret_cast<ProcArgs*>( user_ptr );
    
    AiMsgDebug("Returning node %s", AiNodeGetName(args->createdNodes->getNode(i)));
    return args->createdNodes->getNode(i);

}

} //end of anonymous namespace


#ifdef __cplusplus
extern "C"
{
#endif

AI_EXPORT_LIB int ProcLoader(AtProcVtable *vtable)
// vtable passed in by proc_loader macro define
{
   vtable->Init          = ProcInit;
   vtable->Cleanup       = ProcCleanup;
   vtable->NumNodes      = ProcNumNodes;
   vtable->GetNode       = ProcGetNode;
   vtable->InitPlugin    = ProcInitPlugin;
   vtable->CleanupPlugin = ProcCleanupPlugin;
   vtable->InitBounds    = ProcInitBounds;
   strcpy(vtable->version, AI_VERSION);
   return 1;
}

#ifdef __cplusplus
}
#endif

