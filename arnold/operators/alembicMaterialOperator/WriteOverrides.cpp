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

#include "../../../common/PathUtil.h"
#include "WriteOverrides.h"
#include <ai.h>
#include <sstream>

#include "json/json.h"
#include "json/value.h"


/*
void ApplyOverrides(const std::string& name, AtNode* node, const std::vector<std::string>& tags, OperatorData & data)
{
    bool foundInPath = false;
    int pathSize = 0;
    for(std::vector<std::string>::iterator it= data.attributes.begin(); it!= data.attributes.end(); ++it)
    {
        Json::Value overrides;
        if(it->find("/") != std::string::npos)
        {
            bool curFoundInPath = pathContainsOtherPath(name, *it);
            if(curFoundInPath)
            {
                foundInPath = true;
                std::string overridePath = *it;
                if(overridePath.length() > pathSize)
                {
                    pathSize = overridePath.length();
                    overrides = data.attributesRoot[*it];
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
                overrides = data.attributesRoot[*it];
            }
            
        }
        //else if(foundInPath == false || pathSize != name.length())
        {
            if (std::find(tags.begin(), tags.end(), *it) != tags.end())
            {
                overrides = data.attributesRoot[*it];
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
                    Json::Value val = data.attributesRoot[*it][itr.key().asString()];
                    if( val.isString() )
                        AiNodeSetStr(node, attribute.c_str(), val.asCString());
                    else if( val.isBool() )
                        AiNodeSetBool(node, attribute.c_str(), val.asBool());
                    else if ( val.type() == Json::realValue )
                        AiNodeSetFlt(node, attribute.c_str(), val.asDouble());
                    else if( val.isInt() )
                    {
                        //make the difference between Byte & int!
                        int typeEntry = AiParamGetType(paramEntry);
                        if(typeEntry == AI_TYPE_BYTE)
                        {
                            if(attribute=="visibility")
                            {
                                uint8_t attrViz = val.asInt();
                                // special case, we must determine it against the general viz.
                                uint8_t procViz = AiNodeGetByte(data.proceduralNode, "visibility" );
                                uint8_t compViz = AI_RAY_ALL;
                                {
                                    compViz &= ~AI_RAY_SUBSURFACE;
                                    if (procViz > compViz)
                                        procViz &= ~AI_RAY_SUBSURFACE;
                                    else
                                        attrViz &= ~AI_RAY_SUBSURFACE;
                                    compViz &= ~AI_RAY_SUBSURFACE;
                                    if(procViz > compViz)
                                        procViz &= ~AI_RAY_SPECULAR_REFLECT;
                                    else
                                        attrViz &= ~AI_RAY_SPECULAR_REFLECT;
                                    compViz &= ~AI_RAY_DIFFUSE_REFLECT;
                                    if(procViz > compViz)
                                        procViz &= ~AI_RAY_DIFFUSE_REFLECT;
                                    else
                                        attrViz &= ~AI_RAY_DIFFUSE_REFLECT;
                                    compViz &= ~AI_RAY_VOLUME;
                                    if(procViz > compViz)
                                        procViz &= ~AI_RAY_VOLUME;
                                    else
                                        attrViz &= ~AI_RAY_VOLUME;
                                    compViz &= ~AI_RAY_SPECULAR_TRANSMIT;
                                    if(procViz > compViz)
                                        procViz &= ~AI_RAY_SPECULAR_TRANSMIT;
                                    else
                                        attrViz &= ~AI_RAY_SPECULAR_TRANSMIT;
                                    compViz &= ~AI_RAY_DIFFUSE_TRANSMIT;
                                    if(procViz > compViz)
                                        procViz &= ~AI_RAY_DIFFUSE_TRANSMIT;
                                    else
                                        attrViz &= ~AI_RAY_DIFFUSE_TRANSMIT;
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
                }
            }
        }
    }
}
*/
AtNode* getShader(const std::string& name, const std::vector<std::string>& tags, OperatorData * data)
{
    bool foundInPath = false;
    int pathSize = 0;
    AtNode* appliedShader = NULL;
    for(std::vector<std::pair<std::string, AtNode*> >::iterator it = data->shaders.begin(); it != data->shaders.end(); ++it)
    {
        //check both path & tag
        if(it->first.find("/") != std::string::npos)
        {
            if(pathContainsOtherPath(name, it->first))
            {
                foundInPath = true;
                std::string shaderPath = it->first;
                if(shaderPath.length() > pathSize)
                {
                    pathSize = shaderPath.length();
                    appliedShader = it->second;
                }
            }
        }
        else if(matchPattern(name,it->first)) // based on wildcard expression
        {
            appliedShader = it->second;
            foundInPath = true;
            std::string shaderPath = it->first;
            if(shaderPath.length() > pathSize)
            {
                pathSize = shaderPath.length();
                appliedShader = it->second;
            }
        }

        else if(foundInPath == false)
        {
            if (std::find(tags.begin(), tags.end(), it->first) != tags.end())
            {
                appliedShader = it->second;
            }
        }
    }

    return appliedShader;
}

AtNode* getShaderByName(const std::string& name, OperatorData * data)
{
    for(std::vector<std::pair<std::string, AtNode*> >::iterator it = data->shaders.begin(); it != data->shaders.end(); ++it)
    {
		if(name.compare(it->first) == 0)
			return it->second;
    }

    return NULL;
}


bool ApplyShaders(const std::string& name, AtNode* node, const std::vector<std::string>& tags, OperatorData * data)
{
    AtNode* appliedShader = getShader(name, tags, data);

    if(ApplyShader(name, node, appliedShader))
    {
        return true;
    }
    else
    {
        return false;
    }
}


bool ApplyShader(const std::string& name, AtNode* node, AtNode* shader)
{
	if(shader != NULL)
	{
		AtArray* shaders = AiArrayAllocate( 1 , 1, AI_TYPE_NODE);
        AiArraySetPtr(shaders, 0, shader);

        AiNodeSetArray(node, "shader", shaders);
        return true;
	}
	return false;

}
