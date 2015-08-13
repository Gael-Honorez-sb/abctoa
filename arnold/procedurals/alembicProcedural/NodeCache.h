#ifndef _nodeCache_h_
#define _nodeCache_h_

#include <ai.h>
#include <string>
#include <map>
#include <vector>

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreFactory/All.h>

using namespace Alembic::AbcGeom;

/* 
This class is handling the caching of Arnold node
*/


class NodeCache
{
public:
	NodeCache(AtCritSec mycs);
	~NodeCache();

	AtNode* getCachedNode(std::string cacheId);
	void addNode(std::string cacheId, AtNode* node);


private:
	std::map<std::string, std::string> ArnoldNodeCache;
	//boost::mutex lock;
	AtCritSec lock;
};


class NodeCollector
{
public:
	NodeCollector(AtCritSec mycs);
	~NodeCollector();

	void addNode(AtNode* node);
	size_t getNumNodes();
	AtNode* getNode(int num);
	


private:
	std::vector<AtNode*> ArnoldNodeCollector;
	AtCritSec lock;
};


struct CachedNodeFile
{
	AtNode *node;
	AtArray *matrix;
};

class FileCache
{
public:
	FileCache(AtCritSec mycs);
	~FileCache();

	std::vector<CachedNodeFile> getCachedFile(std::string cacheId);
	void addCache(std::string cacheId, NodeCollector* createdNodes);

	const size_t hash( std::string const& s );

	std::string getHash(std::string fileName,     
						std::map<std::string, AtNode*> shaders,
						std::map<std::string, AtNode*> displacements,
						std::vector<std::string> attributes,
						double frame
						);


private:
	std::map< std::string, std::vector< CachedNodeFile > > ArnoldFileCache;
	//boost::mutex lock;
	AtCritSec lock;
};

#endif