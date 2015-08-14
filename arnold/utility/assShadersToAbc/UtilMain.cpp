#include <ai.h>

#include <iostream>
#include <sstream>
#include <iomanip>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <boost/algorithm/string/replace.hpp>

#include "abcshaderutils.h"
#include "abcExporterUtils.h"

using namespace boost;
namespace po = program_options;
namespace fs = filesystem;

namespace Abc =  Alembic::Abc;
namespace Mat = Alembic::AbcMaterial;

int main(int argc, char *argv[] )
{
	std::string assFile;
	std::string outputFile = "";
	std::string libraryFolder = ".";
	std::string logfile = "";

    po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "Display this message")
		("ass,i", po::value<std::string>(&assFile)->required(), "Ass Input File")
		("output,o", po::value<std::string>(&outputFile), "Output File. If not present, will use the ass file in .abc.")
		("libraries,l", po::value<std::string>(&libraryFolder), "Add search path for plugin libraries")
		("log", po::value<std::string>(&logfile), "output log to this file")        
		;

	po::positional_options_description p;
	p.add("ass", 1);
	p.add("output", 2);

	po::variables_map vm;
	try
	{
		po::store(po::parse_command_line(argc, argv, desc, po::command_line_style::unix_style ^ po::command_line_style::allow_short), vm);
		po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
	}
	catch (std::exception &e)
	{
		std::cout << desc << std::endl;
		return false;
	}
	if (vm.count("help"))
	{
		std::cout << desc << std::endl;
		return EXIT_SUCCESS;
	}
	try
	{
		po::notify(vm);
	}
	catch (std::exception &e)
	{
		std::cout << desc << std::endl;
		return false;
	}

	fs::path assPath(assFile);
	if (!fs::exists(assPath))    // does p actually exist?
	{
		std::cout << assFile << " " << "does not exists !" << std::endl;
		return EXIT_FAILURE;
	}

	if (outputFile.size() == 0)
	{
		outputFile =  assFile;
		replace_last(outputFile, ".ass", ".abc");
	}

	AiBegin();

	Abc::OArchive archive(Alembic::AbcCoreOgawa::WriteArchive(), outputFile.c_str() );
	Abc::OObject root(archive, Abc::kTop);
	Abc::OObject materials(root, "materials");	

	if (logfile.size() != 0)
		AiMsgSetLogFileName(logfile.c_str());

	AiMsgSetConsoleFlags(AI_LOG_INFO + AI_LOG_WARNINGS + AI_LOG_ERRORS + AI_LOG_STATS + AI_LOG_PLUGINS + AI_LOG_PROGRESS + AI_LOG_TIMESTAMP + AI_LOG_BACKTRACE + AI_LOG_MEMORY);
	AiMsgSetLogFileFlags(AI_LOG_INFO + AI_LOG_WARNINGS + AI_LOG_ERRORS + AI_LOG_STATS + AI_LOG_PLUGINS + AI_LOG_PROGRESS + AI_LOG_TIMESTAMP + AI_LOG_BACKTRACE + AI_LOG_MEMORY);

	AiLoadPlugins(libraryFolder.c_str());

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
			boost::replace_all(nodeName, ".message", "");
			boost::replace_all(nodeName, ".", "_");

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
					
					for(unsigned int i=0; i < paramArray->nelements; i++)
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