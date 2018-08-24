
#include "parseAttributes.h"
#include "abcshaderutils.h"
#include "../../../common/PathUtil.h"

#include <pystring.h>

namespace
{
    
    using namespace Alembic::AbcMaterial;
}

void OverrideProperties(Json::Value & jroot, Json::Value jrootAttributes)
{
    for( Json::ValueIterator itr = jrootAttributes.begin() ; itr != jrootAttributes.end() ; itr++ )
    {
        Json::Value paths = jrootAttributes[itr.key().asString()];
        for( Json::ValueIterator overPath = paths.begin() ; overPath != paths.end() ; overPath++ )
        {
            Json::Value attr = paths[overPath.key().asString()];
            jroot[itr.key().asString()][overPath.key().asString()] = attr;
        }
    }
}

Json::Value OverrideAssignations(Json::Value jroot, Json::Value jrootOverrides)
{
    std::vector<std::string> pathOverrides;

    Json::Value newJroot;
    // concatenate both json string.
    for( Json::ValueIterator itr = jrootOverrides.begin() ; itr != jrootOverrides.end() ; itr++ )
    {
        Json::Value tmp = jroot[itr.key().asString()];
        Json::Value paths = jrootOverrides[itr.key().asString()];
        for( Json::ValueIterator shaderPath = paths.begin() ; shaderPath != paths.end() ; shaderPath++ )
        {
            Json::Value val = paths[shaderPath.key().asUInt()];
            pathOverrides.push_back(val.asString());
        }
        if(tmp.size() == 0)
        {
            newJroot[itr.key().asString()] = jrootOverrides[itr.key().asString()];
        }
        else
        {
            Json::Value shaderPaths = jrootOverrides[itr.key().asString()];
            for( Json::ValueIterator itr2 = shaderPaths.begin() ; itr2 != shaderPaths.end() ; itr2++ )
            {
                newJroot[itr.key().asString()].append(jrootOverrides[itr.key().asString()][itr2.key().asUInt()]);
            }
        }
    }
    // Now adding back the original shaders without the ones overriden
    for( Json::ValueIterator itr = jroot.begin() ; itr != jroot.end() ; itr++ )
    {
        Json::Value pathsShader = jroot[itr.key().asString()];
        const Json::Value curval = newJroot[itr.key().asString()];
        for( Json::ValueIterator shaderPathOrig = pathsShader.begin() ; shaderPathOrig != pathsShader.end() ; shaderPathOrig++ )
        {
            Json::Value val = pathsShader[shaderPathOrig.key().asUInt()];
            bool isPresent = (std::find(pathOverrides.begin(), pathOverrides.end(), val.asCString())  != pathOverrides.end());
            if (!isPresent)
                newJroot[itr.key().asString()].append(val);
        }
    }


    return newJroot;
}

AtNode* createNetwork(Alembic::Abc::IObject object, std::string prefix, OperatorData & data)
{
    std::map<std::string,AtNode*> aShaders;
    Mat::IMaterial matObj(object, Alembic::Abc::kWrapExisting);
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

            std::string name = prefix + "_" + abcnode.getName();

            AiNodeSetStr (aShader, "name", name.c_str());
            aShaders[abcnode.getName()] = aShader;

            data.createdNodes->addNode(aShader);

            // We set the default attributes
            ICompoundProperty parameters = abcnode.getParameters();
            if (parameters.valid())
            {
                for (size_t i = 0, e = parameters.getNumProperties(); i < e; ++i)
                {
                    const PropertyHeader & header = parameters.getPropertyHeader(i);

                    if (header.getName() == "name")
                        continue;

                    if (header.isArray())
                        setArrayParameter(parameters, header, aShader, data.pathRemapping);

                    else
                        setParameter(parameters, header, aShader, data.pathRemapping);
                }
            }
        }
    }

    // once every node is created, we can set the connections...
    for (size_t i = 0, e = matObj.getSchema().getNumNetworkNodes(); i < e; ++i)
    {
        Mat::IMaterialSchema::NetworkNode abcnode = matObj.getSchema().getNetworkNode(i);

        std::string target = "<undefined>";
        std::string nodeType = "<undefined>";
        bool nodeArray = false;
        
        std::regex expr ("(.*)\\[([\\d]+)\\]");
        std::smatch what;
        abcnode.getTarget(target);

        std::map<std::string, std::vector<AtNode*> > nodeArrayConnections;
        std::map<std::string, std::vector<AtNode*> >::iterator nodeArrayConnectionsIterator;

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
                        nodeArray = false;
                        if (std::regex_search(inputName, what, expr))
                        {
                            std::string realInputName = what[1];
                            
                            int inputIndex = std::stoi(what[2]);
                            abcnode.getNodeType(nodeType);
                            const AtNodeEntry * nentry = AiNodeEntryLookUp(nodeType.c_str());
                            const AtParamEntry * pentry = AiNodeEntryLookUpParameter(nentry, realInputName.c_str());
                            if(AiParamGetType(pentry) == AI_TYPE_ARRAY)
                            {
                                AtArray* parray = AiNodeGetArray(aShaders[abcnode.getName().c_str()], realInputName.c_str());
                                
                                if(AiArrayGetType(parray) == AI_TYPE_NODE)
                                {
                                    nodeArray = true;
                                    std::vector<AtNode*> connArray;
                                    nodeArrayConnectionsIterator = nodeArrayConnections.find(realInputName);
                                    if(nodeArrayConnectionsIterator != nodeArrayConnections.end())
                                        connArray = nodeArrayConnectionsIterator->second;
                                    
                                    if(connArray.size() < inputIndex+1)
                                        connArray.resize(inputIndex+1);

                                    connArray[inputIndex] = aShaders[connectedNodeName.c_str()];
                                    nodeArrayConnections[realInputName] = connArray;

                                }
                            }
                        }

                        if(!nodeArray)
                        {
                            if(connectedOutputName.length() == 0)
                            {
                                AiMsgDebug("Linking %s to %s.%s", connectedNodeName.c_str(), abcnode.getName().c_str(), inputName.c_str());
                                AiNodeLink(aShaders[connectedNodeName.c_str()], inputName.c_str(), aShaders[abcnode.getName().c_str()]);
                            }
                            else
                            {
                                AiMsgDebug("Linking %s.%s to %s.%s", connectedNodeName.c_str(), connectedOutputName.c_str(), abcnode.getName().c_str(), inputName.c_str());
                                AiNodeLinkOutput(aShaders[connectedNodeName.c_str()], connectedOutputName.c_str(), aShaders[abcnode.getName().c_str()], inputName.c_str());
                            }
                        }
                    }
                }

                for (nodeArrayConnectionsIterator=nodeArrayConnections.begin(); nodeArrayConnectionsIterator!=nodeArrayConnections.end(); ++nodeArrayConnectionsIterator)
                {
                    AtArray* a = AiArray (nodeArrayConnectionsIterator->second.size(), 1, AI_TYPE_NODE);
                    for(int n = 0; n < nodeArrayConnectionsIterator->second.size(); n++)
                    {
                        AiArraySetPtr(a, n, nodeArrayConnectionsIterator->second[n]);

                    }
                    AiNodeSetArray(aShaders[abcnode.getName().c_str()], nodeArrayConnectionsIterator->first.c_str(), a);

                    nodeArrayConnectionsIterator->second.clear();
                }
                nodeArrayConnections.clear();
            }
        }
    }

    // Getting the root node now ...
    std::string connectedNodeName = "<undefined>";
    std::string connectedOutputName = "<undefined>";
    if (matObj.getSchema().getNetworkTerminal(
                "arnold", "surface", connectedNodeName, connectedOutputName))
    {
        AtNode *root = aShaders[connectedNodeName.c_str()];
        if(root)
            AiNodeSetStr(root, "name", prefix.c_str());

        return root;
    }

    return NULL;

}

void ParseShaders(Json::Value jroot, const std::string& ns, const std::string& nameprefix, OperatorData* data, uint8_t type)
{
    // We have to lock here as we need to be sure that another thread is not checking the root while we are creating it here.

    for( Json::ValueIterator itr = jroot.begin() ; itr != jroot.end() ; itr++ )
    {
        
        AiMsgDebug( "[ABC] Parsing shader %s", itr.key().asCString());
        std::string shaderName = ns + itr.key().asString();
        AtNode* shaderNode = AiNodeLookUpByName(shaderName.c_str());
        if(shaderNode == NULL)
        {
            if(data->useAbcShaders)
            {
                std::string originalName = itr.key().asString();
                // we must get rid of .message if it's there
                if(pystring::endswith(originalName, ".message"))
                {
                    originalName = pystring::replace(originalName, ".message", "");
                }

                AiMsgDebug( "[ABC] Create shader %s from ABC", originalName.c_str());

                Alembic::Abc::IObject object = data->materialsObject.getChild(originalName);
                
                if (IMaterial::matches(object.getHeader()))
                    shaderNode = createNetwork(object, shaderName, *data);

            }
            if(shaderNode == NULL)
            {
                // search without custom namespace
                shaderName = itr.key().asString();
                shaderNode = AiNodeLookUpByName(shaderName.c_str());
                if(shaderNode == NULL)
                {
                    AiMsgDebug( "[ABC] Searching shader %s deeper underground...", itr.key().asCString());
                    // look for the same namespace for shaders...
                    std::vector<std::string> strs;
                    pystring::split(nameprefix, strs, ":");
                    if(strs.size() > 1)
                    {
                        strs.pop_back();
                        strs.push_back(itr.key().asString());
                        shaderNode = AiNodeLookUpByName(pystring::join(":", strs).c_str());
                    }
                }
            }
        }
        if(shaderNode != NULL)
        {
            Json::Value paths = jroot[itr.key().asString()];
            AiMsgDebug("[ABC] Shader exists, checking paths. size = %d", paths.size());
            for( Json::ValueIterator itr2 = paths.begin() ; itr2 != paths.end() ; itr2++ )
            {
                Json::Value val = paths[itr2.key().asUInt()];
                AiMsgDebug("[ABC] Adding path %s", val.asCString());
                if(type == 0)
                    data->displacements[val.asString().c_str()] = shaderNode;
                else if(type == 1)
                    data->shaders.push_back(std::pair<std::string, AtNode*>(val.asString().c_str(), shaderNode));

            }
        }
        else
        {
            AiMsgWarning("[ABC] Can't find shader %s", shaderName.c_str());
        }
    }
}
