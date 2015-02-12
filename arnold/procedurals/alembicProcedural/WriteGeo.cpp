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

#include "WriteGeo.h"
#include "WriteTransform.h"
#include "WriteOverrides.h"
#include "ArbGeomParams.h"
#include "PathUtil.h"
#include "parseAttributes.h"

#include <ai.h>
#include <sstream>

#include "json/json.h"

#include <boost/regex.hpp>
#include <boost/thread.hpp>

//-*****************************************************************************

#if AI_VERSION_ARCH_NUM == 3
    #if AI_VERSION_MAJOR_NUM < 4
        #define AiNodeGetNodeEntry(node)   ((node)->base_node)
    #endif
#endif

//-*****************************************************************************


template <typename geomParamT>
void ProcessIndexedBuiltinParam(
        geomParamT param,
        const SampleTimeSet & sampleTimes,
        std::vector<float> & values,
        std::vector<unsigned int> & idxs,
        size_t elementSize)
{
    if ( !param.valid() ) { return; }

    bool isFirstSample = true;
    for ( SampleTimeSet::iterator I = sampleTimes.begin();
          I != sampleTimes.end(); ++I, isFirstSample = false)
    {
        ISampleSelector sampleSelector( *I );


        switch ( param.getScope() )
        {
        case kVaryingScope:
        case kVertexScope:
        {
            // a value per-point, idxs should be the same as vidxs
            // so we'll leave it empty

            // we'll get the expanded form here
            typename geomParamT::Sample sample = param.getExpandedValue(
                    sampleSelector);

            size_t footprint = sample.getVals()->size() * elementSize;

            values.reserve( values.size() + footprint );
            values.insert( values.end(),
                    (float32_t*) sample.getVals()->get(),
                    ((float32_t*) sample.getVals()->get()) + footprint );

            break;
        }
        case kFacevaryingScope:
        {
            // get the indexed form and feed to nidxs

            typename geomParamT::Sample sample = param.getIndexedValue(
                    sampleSelector);

            if ( isFirstSample )
            {
                idxs.reserve( sample.getIndices()->size() );
                idxs.insert( idxs.end(),
                        sample.getIndices()->get(),
                        sample.getIndices()->get() +
                                sample.getIndices()->size() );
            }

            size_t footprint = sample.getVals()->size() * elementSize;
            values.reserve( values.size() + footprint );
            values.insert( values.end(),
                    (const float32_t*) sample.getVals()->get(),
                    ((const float32_t*) sample.getVals()->get()) + footprint );

            break;
        }
        default:
            break;
        }


    }


}

//-*****************************************************************************

namespace
{
    // Arnold scene build is single-threaded so we don't have to lock around
    // access to this for now.
    typedef std::map<std::string, AtNode *> NodeCache;
    NodeCache g_meshCache;
}


//-*************************************************************************
// This is templated to handle shared behavior of IPolyMesh and ISubD

// We send in our empty sampleTimes and vidxs because polymesh needs those
// for processing animated normal.


// The return value is the polymesh node. If instanced, it will be returned
// for the first created instance only.
template <typename primT>
AtNode * ProcessPolyMeshBase(
        primT & prim, ProcArgs & args,
        SampleTimeSet & sampleTimes,
        std::vector<unsigned int> & vidxs,
        MatrixSampleMap * xformSamples,
        const std::string & facesetName = "")
{

    if ( !prim.valid() )
    {
        return NULL;
    }



    typename primT::schema_type  &ps = prim.getSchema();
    
    TimeSamplingPtr ts = ps.getTimeSampling();

    if ( ps.getTopologyVariance() != kHeterogenousTopology )
    {
        GetRelevantSampleTimes( args, ts, ps.getNumSamples(), sampleTimes );
    }
    else
    {
        sampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );
    }

    std::string name = args.nameprefix + prim.getFullName();
    std::string originalName = prim.getFullName();

    AtNode * instanceNode = NULL;

    std::string cacheId;

    SampleTimeSet singleSampleTimes;
    singleSampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );

    ICompoundProperty arbGeomParams = ps.getArbGeomParams();
    ISampleSelector frameSelector( *singleSampleTimes.begin() );

    //get tags
    std::vector<std::string> tags;
    getAllTags(prim, tags, &args);

    // displacement stuff
    AtNode* appliedDisplacement = NULL;

    if(args.linkDisplacement)
    {
        bool foundInPath = false;
        for(std::map<std::string, AtNode*>::iterator it = args.displacements.begin(); it != args.displacements.end(); ++it)
        {
            //check both path & tag
            if(it->first.find("/") != string::npos)
            {
                if(name.find(it->first) != string::npos)
                {
                    appliedDisplacement = it->second;
                    foundInPath = true;
                }

            }
            else if(matchPattern(name,it->first)) // based on wildcard expression
            {
                appliedDisplacement = it->second;
                foundInPath = true;
            }
            else if(foundInPath == false)
            {
                if (std::find(tags.begin(), tags.end(), it->first) != tags.end())
                {
                    appliedDisplacement = it->second;
                }

            }
        }
    }


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
                if(name.find(*it) != string::npos)
                {
                    overrides = args.attributesRoot[*it];
                    foundInPath = true;
                }

            }
            else if(matchPattern(name,*it)) // based on wildcard expression
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

                    if (attribute=="smoothing"
                        || attribute=="subdiv_iterations"
                        || attribute=="subdiv_type"
                        || attribute=="subdiv_adaptive_metric"
                        || attribute=="subdiv_uv_smoothing"
                        || attribute=="subdiv_pixel_error"
                        || attribute=="disp_height"
                        || attribute=="disp_padding"
                        || attribute=="disp_zero_value"
                        || attribute=="disp_autobump"
                        || attribute=="invert_normals")
                    {
                        Json::Value val = args.attributesRoot[*it][itr.key().asString()];

                        rootEncode[attribute]=val;
                    }
                }
            }
        }
    }

    if(appliedDisplacement != NULL)
    {
        rootEncode["disp_shader"] = std::string(AiNodeGetName(appliedDisplacement));

    }

    hashAttributes += writer.write(rootEncode);

    if ( args.makeInstance )
    {
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
        buffer << "@" << facesetName;

        cacheId = buffer.str();

        instanceNode = AiNode( "ginstance" );
        AiNodeSetStr( instanceNode, "name", name.c_str() );
        args.createdNodes.push_back(instanceNode);

        AiNodeSetBool( instanceNode, "inherit_xform", false );

        if ( args.proceduralNode )
        {
            AiNodeSetByte( instanceNode, "visibility",
                    AiNodeGetByte( args.proceduralNode, "visibility" ) );
        }
        else
        {
            AiNodeSetByte( instanceNode, "visibility", AI_RAY_ALL );
        }

        ApplyTransformation( instanceNode, xformSamples, args );

        // adding arbitary parameters
        AddArbitraryGeomParams(
                arbGeomParams,
                frameSelector,
                instanceNode );


        AddArbitraryProceduralParams(args.proceduralNode, instanceNode);

        NodeCache::iterator I = g_meshCache.find(cacheId);
        // parameters overrides
        if(args.linkAttributes)
            ApplyOverrides(name, instanceNode, tags, args);

        // shader assignation
        if (nodeHasParameter( instanceNode, "shader" ) )
        {
            std::vector< std::string > faceSetNames;
            ps.getFaceSetNames(faceSetNames);

            // Managing faceSets.
            if ( faceSetNames.size() > 0 )
            {
                AtArray* shadersArray = AiArrayAllocate( faceSetNames.size() , 1, AI_TYPE_NODE);
                for(int i = 0; i < faceSetNames.size(); i++)
                {
                    if ( ps.hasFaceSet( faceSetNames[i] ) )
                    {
                        std::string faceSetNameForShading = originalName + "/" + faceSetNames[i];
                        AtNode* shaderForFaceSet  = getShader(faceSetNameForShading, tags, args);
                        if(shaderForFaceSet == NULL)
                        {
                            // We can't have a NULL.
                            shaderForFaceSet = AiNode("utility");
                            AiNodeSetStr(shaderForFaceSet, "name", faceSetNameForShading.c_str());
                        }
                         AiArraySetPtr(shadersArray, i, shaderForFaceSet);
                    }
                }
                AiNodeSetArray(instanceNode, "shader", shadersArray);
            }
            else
                ApplyShaders(name, instanceNode, tags, args);
        }
        if (I != g_meshCache.end())
        {
            AiNodeSetPtr(instanceNode, "node", (*I).second );
            return NULL;
        }
    }

    std::vector<AtByte> nsides;
    std::vector<float> vlist;

    std::vector<float> uvlist;
    std::vector<unsigned int> uvidxs;


    // POTENTIAL OPTIMIZATIONS LEFT TO THE READER
    // 1) vlist needn't be copied if it's a single sample
    size_t numSampleTimes = sampleTimes.size();
    bool isFirstSample = true;
    for ( SampleTimeSet::iterator I = sampleTimes.begin();
          I != sampleTimes.end(); ++I, isFirstSample = false)
    {
        ISampleSelector sampleSelector( *I );
        typename primT::schema_type::Sample sample = ps.getValue( sampleSelector );

        if ( isFirstSample )
        {

            size_t numPolys = sample.getFaceCounts()->size();
            nsides.reserve( sample.getFaceCounts()->size() );
            for ( size_t i = 0; i < numPolys; ++i )
            {
                Alembic::Util::int32_t n = sample.getFaceCounts()->get()[i];

                if ( n > 255 )
                {
                    // TODO, warning about unsupported face
                    return NULL;
                }

                nsides.push_back( (AtByte) n );
            }

            size_t vidxSize = sample.getFaceIndices()->size();
            vidxs.reserve( vidxSize );

            unsigned int facePointIndex = 0;
            unsigned int base = 0;

            for (unsigned int i = 0; i < numPolys; ++i)
            {
               // reverse the order of the faces
               int curNum = nsides[i];
               for (int j = 0; j < curNum; ++j, ++facePointIndex)
               {
                  vidxs.push_back((*sample.getFaceIndices())[base+curNum-j-1]);

               }
               base += curNum;
            }
            // old method, BAAAAAAAD
            //vidxs.insert( vidxs.end(), sample.getFaceIndices()->get(),
                    //sample.getFaceIndices()->get() + vidxSize );
        }

        if(numSampleTimes == 1 && (args.shutterOpen != args.shutterClose) && (ps.getVelocitiesProperty().valid()) && isFirstSample )
        {
            float scaleVelocity = 1.0f/args.fps;
            if (AiNodeLookUpUserParameter(args.proceduralNode, "scaleVelocity") !=NULL )
                scaleVelocity *= AiNodeGetFlt(args.proceduralNode, "scaleVelocity");

            Alembic::Abc::V3fArraySamplePtr velptr = sample.getVelocities();
            Alembic::Abc::P3fArraySamplePtr v3ptr = sample.getPositions();

            size_t pSize = sample.getPositions()->size();
            vlist.resize(pSize*3*2);
            numSampleTimes = 2;
            float timeoffset = ((args.frame / args.fps) - ts->getFloorIndex((*I), ps.getNumSamples()).second) * args.fps;
            for ( size_t vId = 0; vId < pSize; ++vId )
            {

                Alembic::Abc::V3f posAtOpen = ((*v3ptr)[vId] + (*velptr)[vId] * scaleVelocity *-timeoffset);
                vlist[3*vId + 0] = posAtOpen.x;
                vlist[3*vId + 1] = posAtOpen.y;
                vlist[3*vId + 2] = posAtOpen.z;

                Alembic::Abc::V3f posAtEnd = ((*v3ptr)[vId] + (*velptr)[vId] * scaleVelocity *(1.0f-timeoffset));
                vlist[3*vId + 3*pSize + 0] = posAtEnd.x;
                vlist[3*vId + 3*pSize + 1] = posAtEnd.y;
                vlist[3*vId + 3*pSize + 2] = posAtEnd.z;
            }
        }
        else
        {
            vlist.reserve( vlist.size() + sample.getPositions()->size() * 3);
            vlist.insert( vlist.end(),
                    (const float32_t*) sample.getPositions()->get(),
                    ((const float32_t*) sample.getPositions()->get()) +
                            sample.getPositions()->size() * 3 );
        }
    }

    bool customUv = false;
    if(args.useUvArchive)
    {
        customUv = true;
        PathList path;
        TokenizePath( prim.getFullName(), path );
        IObject current = args.uvsRoot;
        for ( size_t i = 0; i < path.size(); ++i )
        {
            IObject parent = current;
            current = current.getChild(path[i]);
            if(!current.valid())
            {
                if(i == path.size()-1)
                {
                    current = parent.getChild(path[i]+"Deformed");
                    if(!current.valid())
                        customUv = false;
                }
                else
                    customUv = false;
            }
        }
        if (customUv)
        {
            customUv = false;
            if (IPolyMesh::matches(current.getMetaData()))
            {
                IPolyMesh polymesh( current, kWrapExisting);
                if(polymesh.valid())
                {
                    ProcessIndexedBuiltinParam(
                    polymesh.getSchema().getUVsParam(),
                    singleSampleTimes,
                    uvlist,
                    uvidxs,
                    2);
                    customUv = true;
                }
            }
        }
    }
    if(!customUv)
    {
        ProcessIndexedBuiltinParam(
                ps.getUVsParam(),
                singleSampleTimes,
                uvlist,
                uvidxs,
                2);
    }


    AtNode* meshNode = AiNode( "polymesh" );

    if (!meshNode)
    {
        AiMsgError("Failed to make polymesh node for %s",
                prim.getFullName().c_str());
        return NULL;
    }

    args.createdNodes.push_back(meshNode);

    // Attribute overrides. We assume instance mode all the time here.
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

                        if (attribute=="smoothing"
                            || attribute=="subdiv_iterations"
                            || attribute=="subdiv_type"
                            || attribute=="subdiv_adaptive_metric"
                            || attribute=="subdiv_uv_smoothing"
                            || attribute=="subdiv_pixel_error"
                            || attribute=="disp_height"
                            || attribute=="disp_padding"
                            || attribute=="disp_zero_value"
                            || attribute=="disp_autobump"
                            || attribute=="invert_normals")
                        {
                            //AiMsgDebug("Checking attribute %s for shape %s", attribute.c_str(), name.c_str());
                            // check if the attribute exists ...
                            const AtNodeEntry* nodeEntry = AiNodeGetNodeEntry(meshNode);
                            const AtParamEntry* paramEntry = AiNodeEntryLookUpParameter(nodeEntry, attribute.c_str());

                            if ( paramEntry != NULL)
                            {
                                //AiMsgDebug("attribute %s exists on shape", attribute.c_str());
                                Json::Value val = args.attributesRoot[*it][itr.key().asString()];
                                if( val.isString() )
                                    AiNodeSetStr(meshNode, attribute.c_str(), val.asCString());
                                else if( val.isBool() )
                                    AiNodeSetBool(meshNode, attribute.c_str(), val.asBool());
                                else if( val.isInt() )
                                {
                                    //make the difference between Byte & int!
                                    int typeEntry = AiParamGetType(paramEntry);
                                    if(typeEntry == AI_TYPE_BYTE)
                                        AiNodeSetByte(meshNode, attribute.c_str(), val.asInt());
                                    else
                                        AiNodeSetInt(meshNode, attribute.c_str(), val.asInt());
                                }
                                else if( val.isUInt() )
                                    AiNodeSetUInt(meshNode, attribute.c_str(), val.asUInt());
                                else if( val.isDouble() )
                                    AiNodeSetFlt(meshNode, attribute.c_str(), val.asDouble());
                            }
                        }
                    }
                }
            }
        }
    }


    // displaces assignation

    if(appliedDisplacement!= NULL)
        AiNodeSetPtr(meshNode, "disp_map", appliedDisplacement);

    //making per face shader working
    //if (AiNodeLookUpUserParameter(args.proceduralNode, "faceShaderIdxs") != NULL)
        //AiNodeSetArray(meshNode, "shidxs", AiArrayCopy(AiNodeGetArray(args.proceduralNode, "faceShaderIdxs")));


    if ( instanceNode != NULL)
    {
        AiNodeSetStr( meshNode, "name", (name + ":src").c_str() );
    }
    else
    {
        AiNodeSetStr( meshNode, "name", name.c_str() );
    }

    AiNodeSetArray(meshNode, "vidxs",
            AiArrayConvert(vidxs.size(), 1, AI_TYPE_UINT,
                    (void*)&vidxs[0]));

    AiNodeSetArray(meshNode, "nsides",
            AiArrayConvert(nsides.size(), 1, AI_TYPE_BYTE,
                    &(nsides[0])));


    AiNodeSetArray(meshNode, "vlist",
            AiArrayConvert( vlist.size() / numSampleTimes,
                    numSampleTimes, AI_TYPE_FLOAT, (void*)(&(vlist[0]))
                            ));

    if ( !uvlist.empty() )
    {
        //TODO, option to disable v flipping
//        for (size_t i = 1, e = uvlist.size(); i < e; i += 2)
//        {
//            uvlist[i] = 1.0 - uvlist[i];
//        }
       //int size = uvlist.size();
       AtArray* a_uvlist = AiArrayAllocate( uvlist.size() , 1, AI_TYPE_FLOAT);

       for (unsigned int i = 0; i < uvlist.size() ; ++i)
       {
          AiArraySetFlt(a_uvlist, i, uvlist[i]);
       }

        AiNodeSetArray(meshNode, "uvlist", a_uvlist);

        if ( !uvidxs.empty() )
        {
           // we must invert the idxs

           unsigned int facePointIndex = 0;
           unsigned int base = 0;

           AtArray* uvidxReversed = AiArrayAllocate(uvidxs.size(), 1, AI_TYPE_UINT);

                //AiMsgInfo("sampleTimes.size() %i", sampleTimes.size());

           for (unsigned int i = 0; i < nsides.size() ; ++i)
           {
              int curNum = nsides[i];
              for (int j = 0; j < curNum; ++j, ++facePointIndex)
                 AiArraySetUInt(uvidxReversed, facePointIndex, uvidxs[base+curNum-j-1]);


              base += curNum;
           }

            AiNodeSetArray(meshNode, "uvidxs",
                  uvidxReversed);
        }
        else
        {
            AiNodeSetArray(meshNode, "uvidxs",
                    AiArrayConvert(vidxs.size(), 1, AI_TYPE_UINT,
                            &(vidxs[0])));
        }
    }

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

        AiNodeSetArray( meshNode, "deform_time_samples",
                AiArrayConvert(relativeSampleTimes.size(), 1,
                        AI_TYPE_FLOAT, &relativeSampleTimes[0]));
    }
    else if(numSampleTimes == 2)
    {
        AiNodeSetArray( meshNode, "deform_time_samples",
                AiArray(2, 1, AI_TYPE_FLOAT, 0.f, 1.f));
    }

    // facesets
    std::vector< std::string > faceSetNames;
    ps.getFaceSetNames(faceSetNames);

    if ( faceSetNames.size() > 0 )
    {
        std::vector<AtByte> faceSetArray;
        // By default, we are using all the faces.
        faceSetArray.resize(nsides.size());
        for ( int i = 0; i < (int) faceSetArray.size(); ++i )
            faceSetArray[i] = 0;

        for(int i = 0; i < faceSetNames.size(); i++)
        {
            if ( ps.hasFaceSet( faceSetNames[i] ) )
            {
                std::string faceSetNameForShading = originalName + "/" + faceSetNames[i];
                {
                    IFaceSet faceSet = ps.getFaceSet( faceSetNames[i] );
                    IFaceSetSchema::Sample faceSetSample = faceSet.getSchema().getValue( frameSelector );

                    const int* faceArray((int *)faceSetSample.getFaces()->getData()); 
                    for( int f = 0; f < (int) faceSetSample.getFaces()->size(); f++)
                        faceSetArray[faceArray[f]] = (AtByte*) i;
                }
            }
        }
        AtArray *tmpArray = AiArrayConvert( faceSetArray.size(), 1, AI_TYPE_BYTE, (void *) &faceSetArray[0]);
        AiNodeSetArray( meshNode, "shidxs", tmpArray );
    }

    {
        ICompoundProperty arbGeomParams = ps.getArbGeomParams();
        ISampleSelector frameSelector( *singleSampleTimes.begin() );

        AddArbitraryGeomParams( arbGeomParams, frameSelector, meshNode );

    }

    if ( instanceNode == NULL )
    {
        if ( xformSamples )
        {
            ApplyTransformation( meshNode, xformSamples, args );
        }

        return meshNode;
    }
    else
    {
        AiNodeSetPtr(instanceNode, "node", meshNode );
         AiNodeSetByte( meshNode, "visibility", 0 );
         g_meshCache[cacheId] = meshNode;
        return meshNode;

    }

}

//-*************************************************************************

void ProcessPolyMesh( IPolyMesh &polymesh, ProcArgs &args,
        MatrixSampleMap * xformSamples, const std::string & facesetName )
{
    SampleTimeSet sampleTimes;
    std::vector<unsigned int> vidxs;

    AtNode * meshNode = ProcessPolyMeshBase(
            polymesh, args, sampleTimes, vidxs, xformSamples,
                    facesetName);

    // This is a valid condition for the second instance onward and just
    // means that we don't need to do anything further.
    if ( !meshNode )
    {
        return;
    }


    IPolyMeshSchema &ps = polymesh.getSchema();

    std::vector<float> nlist;
    std::vector<unsigned int> nidxs;

    AiNodeSetBool(meshNode, "smoothing", true);

    if(AiNodeGetInt(meshNode, "subdiv_type") == 0)
    {
        //TODO: better check
        if (AiNodeGetArray(meshNode, "vlist")->nkeys == sampleTimes.size())
        {
            ProcessIndexedBuiltinParam(
                    ps.getNormalsParam(),
                    sampleTimes,
                    nlist,
                    nidxs,
                    3);

        }



        if ( !nlist.empty() )
        {
            AiNodeSetArray(meshNode, "nlist",
                AiArrayConvert( nlist.size() / sampleTimes.size(),
                        sampleTimes.size(), AI_TYPE_FLOAT, (void*)(&(nlist[0]))));

            if ( !nidxs.empty() )
            {

               // we must invert the idxs
               //unsigned int facePointIndex = 0;
               unsigned int base = 0;
               AtArray* nsides = AiNodeGetArray(meshNode, "nsides");
               std::vector<unsigned int> nvidxReversed;
               for (unsigned int i = 0; i < nsides->nelements / nsides->nkeys; ++i)
               {
                  int curNum = AiArrayGetUInt(nsides ,i);

                  for (int j = 0; j < curNum; ++j)
                  {
                      nvidxReversed.push_back(nidxs[base+curNum-j-1]);
                  }
                  base += curNum;
               }
                AiNodeSetArray(meshNode, "nidxs", AiArrayConvert(nvidxReversed.size(), 1, AI_TYPE_UINT, (void*)&nvidxReversed[0]));
            }
            else
            {
                AiNodeSetArray(meshNode, "nidxs",
                        AiArrayConvert(vidxs.size(), 1, AI_TYPE_UINT,
                                &(vidxs[0])));
            }
        }
    }

}

//-*************************************************************************

void ProcessSubD( ISubD &subd, ProcArgs &args,
        MatrixSampleMap * xformSamples, const std::string & facesetName)
{
    SampleTimeSet sampleTimes;
    std::vector<unsigned int> vidxs;

    AtNode * meshNode = ProcessPolyMeshBase(
            subd, args, sampleTimes, vidxs,
                    xformSamples, facesetName );

    // This is a valid condition for the second instance onward and just
    // means that we don't need to do anything further.
    if ( !meshNode )
    {
        return;
    }

    AiNodeSetStr( meshNode, "subdiv_type", "catclark" );
}


