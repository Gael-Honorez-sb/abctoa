#ifndef _nodeCache_h_
#define _nodeCache_h_

#include <ai.h>
#include <string>
#include <map>
#include <vector>
#include <boost/thread.hpp>

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
	NodeCollector();
	~NodeCollector();

	void addNode(AtNode* node);
	size_t getNumNodes();
	AtNode* getNode(int num);
	


private:
	std::vector<AtNode*> ArnoldNodeCollector;
	boost::mutex lock;
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
						std::vector<std::string> attributes
						);

	void addToOpenedFiles(std::string filename);
	void removeFromOpenedFiles(std::string filename);
	bool isFileOpened(std::string filename);

	void addReader(std::string filename);
	IArchive getReader(std::string filename);

private:
	std::map<std::string, std::vector<CachedNodeFile>> ArnoldFileCache;
	std::map< std::string, IArchive > AlembicFileReader;
	std::vector<std::string> openedFiles;
	//boost::mutex lock;
	AtCritSec lock;
};

#endif