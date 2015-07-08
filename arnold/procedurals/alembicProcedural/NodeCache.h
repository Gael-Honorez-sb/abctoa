#ifndef _nodeCache_h_
#define _nodeCache_h_

#include <ai.h>
#include <string>
#include <map>
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
};


#endif