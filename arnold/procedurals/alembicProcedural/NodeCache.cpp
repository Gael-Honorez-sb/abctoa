#include "NodeCache.h"

NodeCache::NodeCache()
{
}

NodeCache::~NodeCache()
{
	ArnoldNodeCache.clear();
}

AtNode* NodeCache::getCachedNode(std::string cacheId)
{
    std::map<std::string, std::string>::iterator I = ArnoldNodeCache.find(cacheId);
    if (I != ArnoldNodeCache.end())
		return AiNodeLookUpByName(I->second.c_str());

    return NULL;
}

void NodeCache::addNode(std::string cacheId, AtNode* node)
{
	ArnoldNodeCache[cacheId] = std::string(AiNodeGetName(node));

}