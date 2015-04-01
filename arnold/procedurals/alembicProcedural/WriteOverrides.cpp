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

#include "PathUtil.h"
#include "WriteOverrides.h"
#include <ai.h>
#include <sstream>

#include "json/json.h"



void ApplyOverrides(std::string name, AtNode* node, std::vector<std::string> tags, ProcArgs & args)
{
    bool foundInPath = false;
    int pathSize = 0;
    for(std::vector<std::string>::iterator it=args.attributes.begin(); it!=args.attributes.end(); ++it)
    {
        Json::Value overrides;
        if(it->find("/") != std::string::npos)
        {
            bool curFoundInPath = isPathContainsInOtherPath(name, *it);
            if(curFoundInPath)
            {
                foundInPath = true;
                std::string overridePath = *it;
                if(overridePath.length() > pathSize)
                {
                    pathSize = overridePath.length();
                    overrides = args.attributesRoot[*it];
                }
            }
        }
        else if(matchPattern(name,*it)) // based on wildcard expression
        {
            foundInPath = true;
            std::string overridePath = *it;
            if(overridePath.length() > pathSize)
            {   
                pathSize = overridePath.length();
                overrides = args.attributesRoot[*it];
            }
            
        }
        else if(foundInPath == false)
        {
            if (std::find(tags.begin(), tags.end(), *it) != tags.end())
            {
                std::string overridePath = *it;
                if(overridePath.length() > pathSize)
                {
                    pathSize = overridePath.length();
                    overrides = args.attributesRoot[*it];
                }
            }
        }
        if(overrides.size() > 0)
        {
            for( Json::ValueIterator itr = overrides.begin() ; itr != overrides.end() ; itr++ )
            {
                std::string attribute = itr.key().asString();
                
                const AtNodeEntry* nodeEntry = AiNodeGetNodeEntry(node);
                const AtParamEntry* paramEntry = AiNodeEntryLookUpParameter(nodeEntry, attribute.c_str());

                if ( paramEntry != NULL)
                {
                    Json::Value val = args.attributesRoot[*it][itr.key().asString()];
                    if( val.isString() )
                        AiNodeSetStr(node, attribute.c_str(), val.asCString());
                    else if( val.isBool() )
                        AiNodeSetBool(node, attribute.c_str(), val.asBool());
                    else if( val.isInt() )
                    {
                        //make the difference between Byte & int!
                        int typeEntry = AiParamGetType(paramEntry);
                        if(typeEntry == AI_TYPE_BYTE)
                        {
                            if(attribute=="visibility")
                            {
                                AtByte attrViz = val.asInt();
                                // special case, we must determine it against the general viz.
                                AtByte procViz = AiNodeGetByte( args.proceduralNode, "visibility" );
                                AtByte compViz = AI_RAY_ALL;
                                {
                                    compViz &= ~AI_RAY_GLOSSY;
                                    if(procViz > compViz)
                                        procViz &= ~AI_RAY_GLOSSY;
                                    else
                                        attrViz &= ~AI_RAY_GLOSSY;
                                    compViz &= ~AI_RAY_DIFFUSE;
                                    if(procViz > compViz)
                                        procViz &= ~AI_RAY_DIFFUSE;
                                    else
                                        attrViz &= ~AI_RAY_DIFFUSE;
                                    compViz &= ~AI_RAY_REFRACTED;
                                    if(procViz > compViz)
                                        procViz &= ~AI_RAY_REFRACTED;
                                    else
                                        attrViz &= ~AI_RAY_REFRACTED;
                                    compViz &= ~AI_RAY_REFLECTED;
                                    if(procViz > compViz)
                                        procViz &= ~AI_RAY_REFLECTED;
                                    else
                                        attrViz &= ~AI_RAY_REFLECTED;
                                    compViz &= ~AI_RAY_SHADOW;
                                    if(procViz > compViz)
                                        procViz &= ~AI_RAY_SHADOW;
                                    else
                                        attrViz &= ~AI_RAY_SHADOW;
                                    compViz &= ~AI_RAY_CAMERA;
                                    if(procViz > compViz)
                                        procViz &= ~AI_RAY_CAMERA;
                                    else
                                        attrViz &= ~AI_RAY_CAMERA;
                                }

                                AiNodeSetByte(node, attribute.c_str(), attrViz);
                            }
                            else
                                AiNodeSetByte(node, attribute.c_str(), val.asInt());
                        }
                        else
                            AiNodeSetInt(node, attribute.c_str(), val.asInt());
                    }
                    else if( val.isUInt() )
                        AiNodeSetUInt(node, attribute.c_str(), val.asUInt());
                    else if( val.isDouble() )
                        AiNodeSetFlt(node, attribute.c_str(), val.asDouble());
                }
            }
        }
    }
}

AtNode* getShader(std::string name, std::vector<std::string> tags, ProcArgs & args)
{
    bool foundInPath = false;
    AtNode* appliedShader = NULL;
    for(std::map<std::string, AtNode*>::iterator it = args.shaders.begin(); it != args.shaders.end(); ++it)
    {
        //check both path & tag
        if(it->first.find("/") != std::string::npos)
        {
            if(isPathContainsInOtherPath(name, it->first))
            {
                appliedShader = it->second;
                foundInPath = true;
            }
        }
        else if(matchPattern(name,it->first)) // based on wildcard expression
        {
            appliedShader = it->second;
            foundInPath = true;
        }
        else if(foundInPath == false)
        {
            if (std::find(tags.begin(), tags.end(), it->first) != tags.end())
                appliedShader = it->second;
        }
    }

    return appliedShader;
}

void ApplyShaders(std::string name, AtNode* node, std::vector<std::string> tags, ProcArgs & args)
{
    bool foundInPath = false;
    AtNode* appliedShader = getShader(name, tags, args);

    if(appliedShader != NULL)
    {
        std::string newName = std::string(AiNodeGetName(appliedShader)) + std::string("_") + name;
        //AiNodeSetStr(appliedShader, "name", newName.c_str());

        AtArray* shaders = AiArrayAllocate( 1 , 1, AI_TYPE_NODE);
        AiArraySetPtr(shaders, 0, appliedShader);

        AiNodeSetArray(node, "shader", shaders);
        //AiNodeSetPtr(instanceNode, "shader", appliedShader);
    }
    else
    {
        AtArray* shaders = AiNodeGetArray(args.proceduralNode, "shader");
        if (shaders->nelements != 0)
            AiNodeSetArray(node, "shader", AiArrayCopy(shaders));
    }
}