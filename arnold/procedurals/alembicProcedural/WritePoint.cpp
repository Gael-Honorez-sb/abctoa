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

#include "WritePoint.h"
#include "WriteTransform.h"
#include "WriteOverrides.h"
#include "parseAttributes.h"

#include "ArbGeomParams.h"
#include "PathUtil.h"


#include <ai.h>
#include <sstream>

#include "json/json.h"

#include <boost/thread.hpp>


//-*****************************************************************************

#if AI_VERSION_ARCH_NUM == 3
    #if AI_VERSION_MAJOR_NUM < 4
        #define AiNodeGetNodeEntry(node)   ((node)->base_node)
    #endif
#endif


//-*****************************************************************************

namespace
{
    // Arnold scene build is single-threaded so we don't have to lock around
    // access to this for now.
    typedef std::map<std::string, AtNode *> NodeCache;
    NodeCache g_pointsCache;

    boost::mutex gGlobalLock;
    #define GLOBAL_LOCK	   boost::mutex::scoped_lock writeLock( gGlobalLock );
}



//-*************************************************************************
// getSampleTimes
// This function fill the sampleTimes timeSet array.

void getSampleTimes(
    IPoints & prim,
    ProcArgs & args,
    SampleTimeSet & sampleTimes
    )
{
    Alembic::AbcGeom::IPointsSchema  &ps = prim.getSchema();
    TimeSamplingPtr ts = ps.getTimeSampling();

    sampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );
}

//-*************************************************************************
// getHash
// This function return the hash of the points, with attributes applied to it.
std::string getHash(
    std::string name,
    std::string originalName,
    IPoints & prim,
    ProcArgs & args,
    SampleTimeSet sampleTimes
    )
{
    Alembic::AbcGeom::IPointsSchema  &ps = prim.getSchema();

    TimeSamplingPtr ts = ps.getTimeSampling();

    std::string cacheId;

    SampleTimeSet singleSampleTimes;
    singleSampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );

    ICompoundProperty arbGeomParams = ps.getArbGeomParams();
    ISampleSelector frameSelector( *singleSampleTimes.begin() );

  //get tags
    std::vector<std::string> tags;
    getAllTags(prim, tags, &args);


    // overrides that can't be applied on instances
    // we create a hash from that.
    std::string hashAttributes("@");
    Json::FastWriter writer;
    Json::Value rootEncode;


    if(args.linkAttributes)
    {
        bool foundInPath = false;
        for(std::vector<std::string>::iterator it=args.attributes.begin(); it!=args.attributes.end(); ++it)
        {
            Json::Value overrides;
            if(it->find("/") != string::npos)
            {
                if(isPathContainsInOtherPath(originalName, *it))
                {
                    overrides = args.attributesRoot[*it];
                    foundInPath = true;
                }

            }
            else if(matchPattern(originalName,*it)) // based on wildcard expression
            {
                overrides = args.attributesRoot[*it];
                foundInPath = true;
            }
            else if(foundInPath == false)
            {
                if (std::find(tags.begin(), tags.end(), *it) != tags.end())
                {
                    overrides = args.attributesRoot[*it];
                }
            }
            if(overrides.size() > 0)
            {
                for( Json::ValueIterator itr = overrides.begin() ; itr != overrides.end() ; itr++ )
                {
                    std::string attribute = itr.key().asString();

                    if (attribute=="mode"
                        || attribute=="min_pixel_width"
                        || attribute=="step_size"
                        || attribute=="invert_normals"
                        )
                    {
                        Json::Value val = args.attributesRoot[*it][itr.key().asString()];
                        rootEncode[attribute]=val;
                    }
                }
            }
        }
    }

    hashAttributes += writer.write(rootEncode);

    std::ostringstream buffer;
    AbcA::ArraySampleKey sampleKey;

    for ( SampleTimeSet::iterator I = sampleTimes.begin();
            I != sampleTimes.end(); ++I )
    {
        ISampleSelector sampleSelector( *I );
        ps.getPositionsProperty().getKey(sampleKey, sampleSelector);

        buffer << GetRelativeSampleTime( args, (*I) ) << ":";
        sampleKey.digest.print(buffer);
        buffer << ":";
    }

    buffer << "@" << hash(hashAttributes);

    cacheId = buffer.str();

    return cacheId;

}

//-*************************************************************************
// getCachePointsdNode
// This function return the the points node if already in the cache.
// Otherwise, return NULL.
AtNode* getCachedPointsNode(std::string cacheId)
{
    GLOBAL_LOCK;
    NodeCache::iterator I = g_pointsCache.find(cacheId);
    if (I != g_pointsCache.end())
        return (*I).second;

    return NULL;
}

AtNode* writePoints(  
    std::string name,
    std::string originalName,
    std::string cacheId,
    IPoints & prim,
    ProcArgs & args,
    SampleTimeSet sampleTimes
    )

{
    GLOBAL_LOCK;

    std::vector<AtPoint> vidxs;
    std::vector<float> radius;

    Alembic::AbcGeom::IPointsSchema  &ps = prim.getSchema();
    TimeSamplingPtr ts = ps.getTimeSampling();

    SampleTimeSet singleSampleTimes;
    singleSampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );

    ICompoundProperty arbGeomParams = ps.getArbGeomParams();
    ISampleSelector frameSelector( *singleSampleTimes.begin() );
 
    //get tags
    
    std::vector<std::string> tags;
    getAllTags(prim, tags, &args);


    AtNode* pointsNode = AiNode( "points" );

    AiNodeSetStr( pointsNode, "name", (name + ":src").c_str() );
    AiNodeSetByte( pointsNode, "visibility", 0 );

    if (!pointsNode)
    {
        AiMsgError("Failed to make points node for %s",
                prim.getFullName().c_str());
        return NULL;
    }

    args.createdNodes.push_back(pointsNode);
    

    float radiusPoint = 1.0f;
    float scaleVelocity = 1.0f/args.fps;
    std::string radiusParam = "pscale";

    if (AiNodeLookUpUserParameter(args.proceduralNode, "radiusPoint") !=NULL )
        radiusPoint = AiNodeGetFlt(args.proceduralNode, "radiusPoint");
    
    

    // Attribute overrides..
    if(args.linkAttributes)
    {
        for(std::vector<std::string>::iterator it=args.attributes.begin(); it!=args.attributes.end(); ++it)
        {
            if(name.find(*it) != string::npos || std::find(tags.begin(), tags.end(), *it) != tags.end() || matchPattern(name,*it))
            {
                const Json::Value overrides = args.attributesRoot[*it];
                if(overrides.size() > 0)
                {
                    for( Json::ValueIterator itr = overrides.begin() ; itr != overrides.end() ; itr++ )
                    {
                        std::string attribute = itr.key().asString();

                        if(attribute == "radius_attribute")
                        {
                            Json::Value val = args.attributesRoot[*it][itr.key().asString()];
                            radiusParam = val.asString();
                        }

                        if(attribute == "radius_multiplier")
                        {
                            Json::Value val = args.attributesRoot[*it][itr.key().asString()];
                            radiusPoint *= val.asDouble();
                        }

                        if(attribute == "velocity_multiplier")
                        {
                            Json::Value val = args.attributesRoot[*it][itr.key().asString()];
                            scaleVelocity *= val.asDouble();
                        }

                        if (attribute=="mode"
                            || attribute=="min_pixel_width"
                            || attribute=="step_size"
                            || attribute=="invert_normals")
                        {
                            //AiMsgDebug("Checking attribute %s for shape %s", attribute.c_str(), name.c_str());
                            // check if the attribute exists ...
                            const AtNodeEntry* nodeEntry = AiNodeGetNodeEntry(pointsNode);
                            const AtParamEntry* paramEntry = AiNodeEntryLookUpParameter(nodeEntry, attribute.c_str());

                            if ( paramEntry != NULL)
                            {
                                //AiMsgDebug("attribute %s exists on shape", attribute.c_str());
                                Json::Value val = args.attributesRoot[*it][itr.key().asString()];
                                if( val.isString() )
                                    AiNodeSetStr(pointsNode, attribute.c_str(), val.asCString());
                                else if( val.isBool() )
                                    AiNodeSetBool(pointsNode, attribute.c_str(), val.asBool());
                                else if( val.isInt() )
                                {
                                    //make the difference between Byte & int!
                                    int typeEntry = AiParamGetType(paramEntry);
                                    if(typeEntry == AI_TYPE_BYTE)
                                        AiNodeSetByte(pointsNode, attribute.c_str(), val.asInt());
                                    else
                                        AiNodeSetInt(pointsNode, attribute.c_str(), val.asInt());
                                }
                                else if( val.isUInt() )
                                    AiNodeSetUInt(pointsNode, attribute.c_str(), val.asUInt());
                                else if( val.isDouble() )
                                    AiNodeSetFlt(pointsNode, attribute.c_str(), val.asDouble());
                            }
                        }
                    }
                }
            }
        }
    }


    bool isFirstSample = true;

    if (AiNodeLookUpUserParameter(args.proceduralNode, "radiusProperty") !=NULL )
        radiusParam = std::string(AiNodeGetStr(args.proceduralNode, "radiusProperty"));


    bool useVelocities = false;
    if ((sampleTimes.size() == 1) && (args.shutterOpen != args.shutterClose))
    {
        // no sample, and motion blur needed, let's try to get velocities.
        if(ps.getVelocitiesProperty().valid())
            useVelocities = true;
    }

    for ( SampleTimeSet::iterator I = sampleTimes.begin();
          I != sampleTimes.end(); ++I, isFirstSample = false)
    {
        ISampleSelector sampleSelector( *I );
        Alembic::AbcGeom::IPointsSchema::Sample sample = ps.getValue( sampleSelector );

        Alembic::Abc::P3fArraySamplePtr v3ptr = sample.getPositions();
        size_t pSize = sample.getPositions()->size();

        // handling radius

        IFloatGeomParam::Sample widthSamp;
        IFloatGeomParam widthParam = ps.getWidthsParam();

        if(!widthParam)
        {
            ICompoundProperty prop = ps.getArbGeomParams();
            if ( prop != NULL && prop.valid() )
            {
                if (prop.getPropertyHeader(radiusParam) != NULL)
                {
                    const PropertyHeader * tagsHeader = prop.getPropertyHeader(radiusParam);
                    if (IFloatGeomParam::matches( *tagsHeader ))
                    {
                        widthParam = IFloatGeomParam( prop,  tagsHeader->getName());
                        widthSamp = widthParam.getExpandedValue( sampleSelector );
                    }
                }
            }
        }
        else
            widthParam.getExpanded(widthSamp, sampleSelector);


        if(useVelocities && isFirstSample)
        {
            
            if (AiNodeLookUpUserParameter(args.proceduralNode, "scaleVelocity") !=NULL )
                scaleVelocity *= AiNodeGetFlt(args.proceduralNode, "scaleVelocity");

            vidxs.resize(pSize*2);
            Alembic::Abc::V3fArraySamplePtr velptr = sample.getVelocities();

            float timeoffset = ((args.frame / args.fps) - ts->getFloorIndex((*I), ps.getNumSamples()).second) * args.fps;

            for ( size_t pId = 0; pId < pSize; ++pId )
            {
                Alembic::Abc::V3f posAtOpen = ((*v3ptr)[pId] + (*velptr)[pId] * scaleVelocity *-timeoffset);
                AtPoint pos1;
                pos1.x = posAtOpen.x;
                pos1.y = posAtOpen.y;
                pos1.z = posAtOpen.z;
                vidxs[pId]= pos1;

                Alembic::Abc::V3f posAtEnd = ((*v3ptr)[pId] + (*velptr)[pId]* scaleVelocity *(1.0f-timeoffset));
                AtPoint pos2;
                pos2.x = posAtEnd.x;
                pos2.y = posAtEnd.y;
                pos2.z = posAtEnd.z;
                vidxs[pId+pSize]= pos2;

                if(widthSamp)
                    radius.push_back((*widthSamp.getVals())[pId] * radiusPoint);
                else
                    radius.push_back(radiusPoint);
            }
        }
        else
            // not motion blur or correctly sampled particles
        {
            for ( size_t pId = 0; pId < pSize; ++pId )
            {
                AtPoint pos;
                pos.x = (*v3ptr)[pId].x;
                pos.y = (*v3ptr)[pId].y;
                pos.z = (*v3ptr)[pId].z;
                vidxs.push_back(pos);
                if(widthSamp)
                    radius.push_back((*widthSamp.getVals())[pId] * radiusPoint);
                else
                    radius.push_back(radiusPoint);
            }
        }
    }

    if(!useVelocities)
    {
        AiNodeSetArray(pointsNode, "points",
                AiArrayConvert( vidxs.size() / sampleTimes.size(),
                        sampleTimes.size(), AI_TYPE_POINT, (void*)(&(vidxs[0]))
                                ));
        AiNodeSetArray(pointsNode, "radius",
                AiArrayConvert( vidxs.size() / sampleTimes.size(),
                        sampleTimes.size(), AI_TYPE_FLOAT, (void*)(&(radius[0]))
                                ));

        if ( sampleTimes.size() > 1 )
        {
            std::vector<float> relativeSampleTimes;
            relativeSampleTimes.reserve( sampleTimes.size() );

            for (SampleTimeSet::const_iterator I = sampleTimes.begin();
                    I != sampleTimes.end(); ++I )
            {
               chrono_t sampleTime = GetRelativeSampleTime( args, (*I) );

                relativeSampleTimes.push_back(sampleTime);

            }

            AiNodeSetArray( pointsNode, "deform_time_samples",
                    AiArrayConvert(relativeSampleTimes.size(), 1,
                            AI_TYPE_FLOAT, &relativeSampleTimes[0]));
        }
    }
    else
    {
        AiNodeSetArray(pointsNode, "points",
                AiArrayConvert( vidxs.size() / 2,
                        2, AI_TYPE_POINT, (void*)(&(vidxs[0]))
                                ));
        AiNodeSetArray(pointsNode, "radius",
                AiArrayConvert( vidxs.size() /2 / sampleTimes.size(),
                        sampleTimes.size(), AI_TYPE_FLOAT, (void*)(&(radius[0]))
                                ));

        AiNodeSetArray( pointsNode, "deform_time_samples",
                    AiArray(2, 1, AI_TYPE_FLOAT, 0.f, 1.f));

    }

    ICompoundProperty arbPointsParams = ps.getArbGeomParams();
    AddArbitraryGeomParams( arbGeomParams, frameSelector, pointsNode );

    g_pointsCache[cacheId] = pointsNode;
    return pointsNode;


}


//-*************************************************************************
// createInstance
// This function create & return a instance node with shaders & attributes applied.
void createInstance(
    std::string name,
    std::string originalName,
    IPoints & prim,
    ProcArgs & args,
    MatrixSampleMap * xformSamples,
    AtNode* points)
{
    Alembic::AbcGeom::IPointsSchema  &ps = prim.getSchema();
    ICompoundProperty arbGeomParams = ps.getArbGeomParams();

    SampleTimeSet singleSampleTimes;
    TimeSamplingPtr ts = ps.getTimeSampling();
    singleSampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );

    ISampleSelector frameSelector( *singleSampleTimes.begin() );

    AtNode* instanceNode = AiNode( "ginstance" );

    AiNodeSetStr( instanceNode, "name", name.c_str() );
    AiNodeSetPtr(instanceNode, "node", points );
    AiNodeSetBool( instanceNode, "inherit_xform", false );
    
    if ( args.proceduralNode )
        AiNodeSetByte( instanceNode, "visibility", AiNodeGetByte( args.proceduralNode, "visibility" ) );
    else
        AiNodeSetByte( instanceNode, "visibility", AI_RAY_ALL );

    // Xform
    ApplyTransformation( instanceNode, xformSamples, args );

    // adding arbitary parameters
    AddArbitraryGeomParams(
            arbGeomParams,
            frameSelector,
            instanceNode );

    // adding attributes on procedural
    AddArbitraryProceduralParams(args.proceduralNode, instanceNode);

    //get tags
    std::vector<std::string> tags;
    getAllTags(prim, tags, &args);

    // Arnold Attribute from json
    if(args.linkAttributes)
        ApplyOverrides(originalName, instanceNode, tags, args);


    // shader assignation
    if (nodeHasParameter( instanceNode, "shader" ))
        ApplyShaders(originalName, instanceNode, tags, args);

    args.createdNodes.push_back(instanceNode);

}

void ProcessPoint( IPoints &points, ProcArgs &args,
                    MatrixSampleMap * xformSamples)
{

    if ( !points.valid() )
        return;

    std::string originalName = points.getFullName();
    std::string name = args.nameprefix + originalName;


    SampleTimeSet sampleTimes;
    getSampleTimes(points, args, sampleTimes);

    std::string cacheId = getHash(name, originalName, points, args, sampleTimes);
    AtNode* pointsNode = getCachedPointsNode(cacheId);

    if(pointsNode == NULL)
    { // We don't have a cache, so we much create this points object.
        pointsNode = writePoints(name, originalName, cacheId, points, args, sampleTimes);
    }

    // we can create the instance, with correct transform, attributes & shaders.
    if(pointsNode != NULL)
        createInstance(name, originalName, points, args, xformSamples, pointsNode);

}

