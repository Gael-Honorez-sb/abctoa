#include <ai.h>
#include <json/json.h>
#include <Alembic/Abc/All.h>
#include <iostream>
#include <fstream>

#include "operatorData.h"
#include "parseAttributes.h"
#include "WriteOverrides.h"

// Recursively copy the values of b into a.
void update(Json::Value& a, Json::Value& b) {
    Json::Value::Members memberNames = b.getMemberNames();
    for (Json::Value::Members::const_iterator it = memberNames.begin();
        it != memberNames.end(); ++it)
    {
        const std::string& key = *it;
        if (a[key].isObject()) {
            update(a[key], b[key]);
        }
        else {
            a[key] = b[key];
        }
    }
}

AI_OPERATOR_NODE_EXPORT_METHODS(alembicMaterialOperatorMethods);

node_parameters
{
    AiParameterStr("abcShaders", "");
    AiParameterStr("jsonFile", "");
    AiParameterStr("secondaryJsonFile", "");
    AiParameterStr("selection", "");
    AiParameterStr("shadersAssignation", "");
    AiParameterStr("displacementsAssignation", "");
    

    AiParameterStr("attributes", "");

    AiParameterBool("skipShaders", false);
    AiParameterBool("skipAttributes", false);
    AiParameterBool("skipLayers", false);
    AiParameterBool("skipDisplacements", false);

}

operator_init
{
    OperatorData * data = new OperatorData();
    data->createdNodes = new NodeCollector(op);

    AiNodeSetLocalData(op, data);
    
    if (const char* env_p = std::getenv("PATH_REMAPPING")) // TODO: Read from json file and procedural parameters & merge them all.
    {
        Json::Reader pathRemappingReader;
        Json::Value pathRemappingReaderOverrides;

        if (pathRemappingReader.parse(env_p, pathRemappingReaderOverrides))
        {
            for (Json::ValueIterator itr = pathRemappingReaderOverrides.begin(); itr != pathRemappingReaderOverrides.end(); itr++)
            {
                std::string path1 = itr.key().asString();
                std::string path2 = pathRemappingReaderOverrides[itr.key().asString()].asString();
                AiMsgDebug("Remapping %s to %s", path1.c_str(), path2.c_str());
                data->pathRemapping[path1] = path2;
            }
        }
    }


    bool skipShaders = false;
    bool skipAttributes = false;
    bool skipDisplacement = false;
    bool skipLayers = false;

    Json::Value jrootShaders;
    Json::Value jrootAttributes;
    Json::Value jrootDisplacements;
    Json::Value jrootLayers;

    skipShaders = AiNodeGetBool(op, "skipShaders");
    skipAttributes = AiNodeGetBool(op, "skipAttributes");
    skipDisplacement = AiNodeGetBool(op, "skipDisplacements");
    skipLayers = AiNodeGetBool(op, "skipLayers");

    AtString jsonFile = AiNodeGetStr(op, "jsonFile");
    AtString secondaryJsonFile = AiNodeGetStr(op, "secondaryJsonFile");
    AtString shadersAssignation = AiNodeGetStr(op, "shadersAssignation");
    AtString attributes = AiNodeGetStr(op, "attributes");
    AtString displacementsAssignation = AiNodeGetStr(op, "displacementsAssignation");

    data->useShaderAssignationAttribute = false;

    if (jsonFile.empty() == false)
    {

        Json::Value jroot;
        Json::Reader reader;
        std::ifstream test(jsonFile.c_str(), std::ifstream::binary);
        bool parsingSuccessful = reader.parse(test, jroot, false);

        if (secondaryJsonFile.empty() == false)
        {
            std::ifstream test2(secondaryJsonFile.c_str(), std::ifstream::binary);
            Json::Value jroot2;
            if (reader.parse(test2, jroot2, false))
                update(jroot, jroot2);
        }

        if (parsingSuccessful)
        {
            if (skipShaders == false)
            {
                if (jroot["namespace"].isString())
                    data->ns = jroot["namespace"].asString() + ":";


                if (jroot["shadersAttribute"].isString())
                {
                    data->shaderAssignationAttribute = jroot["shadersAttribute"].asString();
                    data->useShaderAssignationAttribute = true;
                }

                jrootShaders = jroot["shaders"];
                if (shadersAssignation.empty() == false)
                {
                    Json::Reader readerOverride;
                    Json::Value jrootShadersOverrides;
                    std::vector<std::string> pathOverrides;
                    if (readerOverride.parse(shadersAssignation.c_str(), jrootShadersOverrides))
                        if (jrootShadersOverrides.size() > 0)
                            jrootShaders = OverrideAssignations(jrootShaders, jrootShadersOverrides);
                }
            }


            if (skipAttributes == false)
            {
                jrootAttributes = jroot["attributes"];

                if (attributes.empty() == false)
                {
                    Json::Reader readerOverride;
                    Json::Value jrootAttributesOverrides;

                    if (readerOverride.parse(attributes.c_str(), jrootAttributesOverrides))
                        OverrideProperties(jrootAttributes, jrootAttributesOverrides);

                }
            }


            if (skipDisplacement == false)
            {
                jrootDisplacements = jroot["displacement"];
                if (displacementsAssignation.empty() == false)
                {
                    Json::Reader readerOverride;
                    Json::Value jrootDisplacementsOverrides;

                    if (readerOverride.parse(displacementsAssignation.c_str(), jrootDisplacementsOverrides))
                        if (jrootDisplacementsOverrides.size() > 0)
                            jrootDisplacements = OverrideAssignations(jrootDisplacements, jrootDisplacementsOverrides);
                }
            }

            /*if (skipLayers == false && customLayer)
            {
                jrootLayers = jroot["layers"];
                if (layersOverride.empty() == false)
                {
                    Json::Reader readerOverride;
                    Json::Value jrootLayersOverrides;

                    if (readerOverride.parse(layersOverride.c_str(), jrootLayersOverrides))
                    {
                        jrootLayers[layer]["removeShaders"] = jrootLayersOverrides[layer].get("removeShaders", skipShaders).asBool();
                        jrootLayers[layer]["removeDisplacements"] = jrootLayersOverrides[layer].get("removeDisplacements", skipDisplacement).asBool();
                        jrootLayers[layer]["removeProperties"] = jrootLayersOverrides[layer].get("removeProperties", skipAttributes).asBool();

                        if (jrootLayersOverrides[layer]["shaders"].size() > 0)
                            jrootLayers[layer]["shaders"] = OverrideAssignations(jrootLayers[layer]["shaders"], jrootLayersOverrides[layer]["shaders"]);

                        if (jrootLayersOverrides[layer]["displacements"].size() > 0)
                            jrootLayers[layer]["displacements"] = OverrideAssignations(jrootLayers[layer]["displacements"], jrootLayersOverrides[layer]["displacements"]);

                        if (jrootLayersOverrides[layer]["properties"].size() > 0)
                            OverrideProperties(jrootLayers[layer]["properties"], jrootLayersOverrides[layer]["properties"]);
                    }
                }
            }*/

        }


    }


    AtString abcfile = AiNodeGetStr(op, "abcShaders");
    data->useAbcShaders = false;
    if (abcfile.empty() == false)
    {
        Alembic::AbcCoreFactory::IFactory factory;
        Alembic::Abc::IArchive archive = factory.getArchive(abcfile.c_str());
        if (!archive.valid())
        {
            AiMsgWarning("Cannot read file %s", abcfile);
        }
        else
        {
            AiMsgDebug("reading file %s", abcfile);
            Alembic::Abc::IObject materialsObject(archive.getTop(), "materials");
            data->materialsObject = materialsObject;
            data->useAbcShaders = true;
        }
    }

    std::string ns = std::string(AiNodeGetName(op)) + "/";

    if (jrootShaders.size() > 0)
        ParseShaders(jrootShaders, ns, "", data, 1);

    return true;
}

operator_cleanup
{
    OperatorData * data = (OperatorData*)AiNodeGetLocalData(op);
    return true;
}

operator_cook
{
    const char* name = AiNodeGetName(node);
    AiMsgInfo("Node being cooked %s", name);
    OperatorData * data = (OperatorData*)AiNodeGetLocalData(op);

    const std::vector<std::string> tags;
    ApplyShaders(std::string(name), node, tags, data);


    return true;
}

node_loader
{
    if (i>0) return 0;
    node->methods = alembicMaterialOperatorMethods;
    node->output_type = AI_TYPE_NONE;
    node->name = "alembicMaterialOperator";
    node->node_type = AI_NODE_OPERATOR;
    strcpy(node->version, AI_VERSION);
return true;
}