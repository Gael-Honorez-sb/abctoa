/*Alembic Arnold Shader
Copyright (c) 2014, Gaël Honorez, All rights reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library.*/


#include "abcshaderutils.h"
#include <Alembic/AbcCoreFactory/IFactory.h>


AI_SHADER_NODE_EXPORT_METHODS(ABCShaderMethods);


namespace Abc =  Alembic::Abc;
namespace Mat = Alembic::AbcMaterial;

enum AbcShaderParams
{
    p_file,
    p_path,
    p_shader
};

struct ShaderData
{
    std::map<std::string,AtNode*> aShaders;
    Mat::IMaterial matObj;
};


node_parameters
{
    AiParameterStr("file", "");
    AiParameterStr("shader", "");
    AiParameterRGB("shaderIn", 1.0f,0,0);

    AiMetaDataSetInt(mds, NULL, "maya.id", 0x70532);
    AiMetaDataSetBool(mds, NULL, "maya.hide", true);



}


node_initialize
{
    AiNodeSetLocalData(node, new ShaderData);
    ShaderData* data = reinterpret_cast<ShaderData*>(AiNodeGetLocalData(node));

    Alembic::Abc::IArchive archive;
    Alembic::AbcCoreFactory::IFactory factory;
    factory.setPolicy(Alembic::Abc::ErrorHandler::kQuietNoopPolicy);
    archive = factory.getArchive(AiNodeGetStr(node, "file"));
    if (!archive.valid())
    {
        AiMsgError("[AbcShader] Cannot read file %s", AiNodeGetStr(node, "file"));
        return;
    }
    Abc::IObject materialsObject(archive.getTop(), "materials");
    if(!materialsObject.valid())
    {
        AiMsgError("[AbcShader] No material library found");
        return;
    }
    const char* shaderFrom = AiNodeGetStr(node, "shader");
    if(shaderFrom == NULL)
    {
        AiMsgError("no material name given");
        return;
    }

    Abc::IObject object = materialsObject.getChild(shaderFrom);
    if (Mat::IMaterial::matches(object.getHeader()))
    {
        Mat::IMaterial matObj(object, Abc::kWrapExisting);
        data->matObj = matObj;
        //first, we create all the nodes.
        for (size_t i = 0, e = matObj.getSchema().getNumNetworkNodes(); i < e; ++i)
        {
            Mat::IMaterialSchema::NetworkNode abcnode = matObj.getSchema().getNetworkNode(i);
            std::string target = "<undefined>";
            abcnode.getTarget(target);
            if(target == "arnold")
            {
                std::string nodeType = "<undefined>";
                abcnode.getNodeType(nodeType);
                AiMsgDebug("Creating %s node named %s", nodeType.c_str(), abcnode.getName().c_str());
                AtNode* aShader = AiNode (nodeType.c_str());

                std::string name = std::string(AiNodeGetStr(node, "name")) + "_" + abcnode.getName();

                AiNodeSetStr (aShader, "name", name.c_str());
                data->aShaders[abcnode.getName()] = aShader;

                // We set the default attributes
                Abc::ICompoundProperty parameters = abcnode.getParameters();
                if (parameters.valid())
                {
                    for (size_t i = 0, e = parameters.getNumProperties(); i < e; ++i)
                    {
                        const Abc::PropertyHeader & header = parameters.getPropertyHeader(i);

                        if (header.getName() == "name")
                            continue;

                        if (header.isArray())
                            setArrayParameter(parameters, header, aShader);

                        else
                            setParameter(parameters, header, aShader);
                    }

                }

            }
        }

        // once every node is created, we can set the connections...
        for (size_t i = 0, e = matObj.getSchema().getNumNetworkNodes(); i < e; ++i)
        {
            Mat::IMaterialSchema::NetworkNode abcnode = matObj.getSchema().getNetworkNode(i);

            std::string target = "<undefined>";
            abcnode.getTarget(target);
            if(target == "arnold")
            {
                size_t numConnections = abcnode.getNumConnections();
                if(numConnections)
                {
                    AiMsgDebug("linking nodes");
                    std::string inputName, connectedNodeName, connectedOutputName;
                    for (size_t j = 0; j < numConnections; ++j)
                    {
                        if (abcnode.getConnection(j, inputName, connectedNodeName, connectedOutputName))
                        {
                            AiMsgDebug("Linking %s.%s to %s.%s", connectedNodeName.c_str(), connectedOutputName.c_str(), abcnode.getName().c_str(), inputName.c_str());
                            AiNodeLinkOutput(data->aShaders[connectedNodeName.c_str()], connectedOutputName.c_str(), data->aShaders[abcnode.getName().c_str()], inputName.c_str());
                        }
                    }

                }
            }
        }

        // Getting the root node now ...
        std::string connectedNodeName = "<undefined>";
        std::string connectedOutputName = "<undefined>";
        if (matObj.getSchema().getNetworkTerminal(
                    "arnold", "surface", connectedNodeName, connectedOutputName))
        {
            AiMsgDebug("Linking %s.%s to root", connectedNodeName.c_str(), connectedOutputName.c_str());
            AiNodeLink(data->aShaders[connectedNodeName.c_str()],  "shaderIn", node);
        }

    }

}

node_update
{
    ShaderData* data = reinterpret_cast<ShaderData*>(AiNodeGetLocalData(node));

    // We have to over-write the parameters that need it.
    std::vector<std::string> mappingNames;
    data->matObj.getSchema().getNetworkInterfaceParameterMappingNames(mappingNames);
    for (std::vector<std::string>::iterator I = mappingNames.begin(); I != mappingNames.end(); ++I)
    {

        std::string mapToNodeName;
        std::string mapToParamName;

        if (data->matObj.getSchema().getNetworkInterfaceParameterMapping((*I), mapToNodeName, mapToParamName))
        {
            std::string interfaceName = *I;

            Alembic::AbcMaterial::IMaterialSchema::NetworkNode abcNode = data->matObj.getSchema().getNetworkNode(mapToNodeName);
            if (!abcNode.valid())
                continue;


            if (data->aShaders.count(abcNode.getName()) == 0)
                continue;

            AtNode* aShader = data->aShaders[abcNode.getName()];

            Alembic::Abc::ICompoundProperty props = abcNode.getParameters();
            if (!props.valid())
                continue;

            if (props.getNumProperties() > 0)
            {

                Alembic::AbcCoreAbstract::PropertyHeader header = *props.getPropertyHeader(mapToParamName);
                const AtUserParamEntry* type = AiNodeLookUpUserParameter(node, interfaceName.c_str());
                if(type)
                {
                    if (AiUserParamGetType(type) == AI_TYPE_NODE)
                    {
                        AtNode *linked = (AtNode*)AiNodeGetPtr(node, interfaceName.c_str());
                        if (linked)
                        {
                            if(!AiNodeIsLinked (aShader, header.getName().c_str()))
                                AiNodeLink(linked, header.getName().c_str(), aShader);
                            else
                            {
                                AtNode* oldLink = AiNodeGetLink (aShader, header.getName().c_str());
                                if(oldLink != linked)
                                {
                                    AiNodeUnlink (aShader, header.getName().c_str());
                                    AiNodeLink(linked, header.getName().c_str(), aShader);
                                }

                            }
                        }
                        else
                            AiMsgDebug("shader is not linked %s", header.getName().c_str());
                    }
                    else
                        setUserParameter(node, interfaceName, header, aShader);
                }


            }

        }
    }

}

node_finish
{
    ShaderData* data = reinterpret_cast<ShaderData*>(AiNodeGetLocalData(node));
    delete data;
}

shader_evaluate
{
    ShaderData* data = reinterpret_cast<ShaderData*>(AiNodeGetLocalData(node));

    sg->out.RGB = AiShaderEvalParamRGB(p_shader);
}

