#include <ai.h>

#include <iostream>
#include <sstream>
#include <iomanip>

#include "ezOptionParser.hpp"

#include "abcshaderutils.h"
#include "abcExporterUtils.h"

namespace Abc =  Alembic::Abc;
namespace Mat = Alembic::AbcMaterial;

void loadLibraries(std::vector<std::string> libraryFolders)
{
    AiLoadPlugins("../shaders");
    AiLoadPlugins(".");

    for (size_t i = 0; i < libraryFolders.size(); i++)
        AiLoadPlugins(libraryFolders[i].c_str());
}


int main(int argc, char *argv[] )
{
    std::string assFile;
    std::string outputFile = "";
    std::vector<std::string> libraryFolders;
    std::string logfile = "";

    ez::OptionParser opt;
    opt.overview = "Ass to Abc";


    opt.add("input", true, 1, "Ass Input File", ez::EZ_FILE);
    opt.add("output", true, 1, "Output File.", ez::EZ_TEXT);
    opt.add("-l,--libraries", false, -1, "Add search path for plugin libraries");
    opt.add("--log", false, 1, "output log path", ez::EZ_TEXT);

    if (!opt.parse(argc, argv))
        return EXIT_SUCCESS;


    opt.get("input").get(assFile);
    opt.get("output").get(outputFile);
    opt.get("-l").getVector(libraryFolders);
    opt.get("--log").get(logfile);


    if (outputFile.size() == 0)
        outputFile = pystring::replace(assFile, ".ass", ".abc");

    AiBegin();

    Abc::OArchive archive(Alembic::AbcCoreOgawa::WriteArchive(), outputFile.c_str() );
    Abc::OObject root(archive, Abc::kTop);
    Abc::OObject materials(root, "materials");    

    if (logfile.size() != 0)
        AiMsgSetLogFileName(logfile.c_str());

    AiMsgSetConsoleFlags(AI_LOG_INFO + AI_LOG_WARNINGS + AI_LOG_ERRORS + AI_LOG_STATS + AI_LOG_PLUGINS + AI_LOG_PROGRESS + AI_LOG_TIMESTAMP + AI_LOG_BACKTRACE + AI_LOG_MEMORY);
    AiMsgSetLogFileFlags(AI_LOG_INFO + AI_LOG_WARNINGS + AI_LOG_ERRORS + AI_LOG_STATS + AI_LOG_PLUGINS + AI_LOG_PROGRESS + AI_LOG_TIMESTAMP + AI_LOG_BACKTRACE + AI_LOG_MEMORY);

    loadLibraries(libraryFolders);

    AiASSLoad(assFile.c_str(), AI_NODE_ALL - AI_NODE_DRIVER - AI_NODE_FILTER);

    AtNodeSet* exportedNodes = new AtNodeSet;

    // Get all shaders root.

    AtNodeIterator *iter = AiUniverseGetNodeIterator(AI_NODE_SHADER);
    std::set<AtNode*> rootShaders;

    // First, we iterate all nodes, everything is potentially a root node.
    while (!AiNodeIteratorFinished(iter))
        rootShaders.insert(AiNodeIteratorGetNext(iter));
    AiNodeIteratorDestroy(iter);

    // Then we iterate again. If a has another node as input parameter, it's not a root.
    iter = AiUniverseGetNodeIterator(AI_NODE_SHADER);
    while (!AiNodeIteratorFinished(iter))
    {
        AtNode *shader = AiNodeIteratorGetNext(iter);

        AtNodeSet* connectedNodes = new AtNodeSet;

        getAllArnoldNodes(shader, connectedNodes);

        std::set<AtNode*>::const_iterator sit (connectedNodes->begin()), send(connectedNodes->end());
        for(;sit!=send;++sit)
        {
            std::set<AtNode*>::iterator toErase = rootShaders.find (*sit);
            if(toErase !=  rootShaders.end())
                rootShaders.erase(toErase);    
        }
    }    
    AiNodeIteratorDestroy(iter);


    for (std::set<AtNode*>::iterator root = rootShaders.begin() ; root != rootShaders.end(); ++root)
    {
        std::string containerName(AiNodeGetName(*root));

        AiMsgInfo("[EXPORT] Creating container : %s", AiNodeGetName(*root));
        Mat::OMaterial matObj(materials, AiNodeGetName(*root));

        exportedNodes->insert(*root);
        getAllArnoldNodes(*root, exportedNodes);
        AiMsgTab(-2);
        std::set<AtNode*>::const_iterator sit (exportedNodes->begin()), send(exportedNodes->end());
        for(;sit!=send;++sit)
        {
            // adding the node to the network
            std::string nodeName = std::string(AiNodeGetName(*root)) + ":" + std::string(AiNodeGetName(*sit)) ;
            nodeName = pystring::replace(nodeName, ".message", "");
            nodeName = pystring::replace(nodeName, ".", "_");

            AiMsgInfo("[EXPORTING %s] Added node : %s", AiNodeGetName(*root), nodeName.c_str());
            matObj.getSchema().addNetworkNode(nodeName.c_str(), "arnold", AiNodeEntryGetName(AiNodeGetNodeEntry(*sit)));

            if(*root == *sit)
            {
                AiMsgInfo("[EXPORTING %s] Root node is : %s", AiNodeGetName(*root), nodeName.c_str());
                //TODO : get if it's a volume, eventually
                matObj.getSchema().setNetworkTerminal(
                "arnold",
                "surface",
                nodeName.c_str(),
                "out");
            }    

            //export parameters


            AtParamIterator* nodeParam = AiNodeEntryGetParamIterator(AiNodeGetNodeEntry(*sit));
            int outputType = AiNodeEntryGetOutputType(AiNodeGetNodeEntry(*sit));

            while (!AiParamIteratorFinished(nodeParam))
            {
                const AtParamEntry *paramEntry = AiParamIteratorGetNext(nodeParam);
                const char* paramName = AiParamGetName(paramEntry);

                if(AiParamGetType(paramEntry) == AI_TYPE_ARRAY)
                {

                    AtArray* paramArray = AiNodeGetArray(*sit, paramName);
                    processArrayValues(*sit, paramName, paramArray, outputType, matObj, nodeName);
                    
                    for(unsigned int i=0; i < AiArrayGetNumElements(paramArray); i++)
                        processArrayParam(*sit, paramName, paramArray, i, outputType, matObj, nodeName, containerName);

                }
                else
                    processLinkedParam(*sit, AiParamGetType(paramEntry), outputType, matObj, nodeName, paramName, containerName);

            }
            AiParamIteratorDestroy(nodeParam);

        }
        AiMsgTab(-2);

    }

}