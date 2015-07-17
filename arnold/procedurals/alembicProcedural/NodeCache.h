#ifndef _nodeCache_h_
#define _nodeCache_h_

#include <ai.h>
#include <string>
#include <map>
#include <vector>
#include <boost/thread.hpp>

/* 
This class is handling the caching of Arnold node
*/


class NodeCache
{
public:
	NodeCache();
	~NodeCache();

	AtNode* getCachedNode(std::string cacheId);
	void addNode(std::string cacheId, AtNode* node);


private:
	std::map<std::string, std::string> ArnoldNodeCache;
	boost::mutex lock;
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


class FileCache
{
public:
	FileCache();
	~FileCache();

	NodeCollector* getCachedFile(std::string cacheId);
	void addCache(std::string cacheId, NodeCollector* createdNodes);

	const size_t hash( std::string const& s );

	std::string getHash(std::string fileName,     
						std::map<std::string, AtNode*> shaders,
						std::map<std::string, AtNode*> displacements,
						std::vector<std::string> attributes
						);

private:
	std::map<std::string, NodeCollector*> ArnoldFileCache;
	boost::mutex lock;
};

#endif