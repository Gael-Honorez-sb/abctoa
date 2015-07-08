#include "NodeCache.h"

NodeCache::NodeCache()
{
}

NodeCache::~NodeCache()
{
	ArnoldNodeCache.clear();
}

//-*************************************************************************
// getCachedNode
// This function return the the mesh node if already in the cache.
// Otherwise, return NULL.
//-*************************************************************************
AtNode* NodeCache::getCachedNode(std::string cacheId)
{
    std::map<std::string, std::string>::iterator I = ArnoldNodeCache.find(cacheId);
    if (I != ArnoldNodeCache.end())
		return AiNodeLookUpByName(I->second.c_str());

    return NULL;
}

//-*************************************************************************
// addNode
// This function adds a node in the cache.
//-*************************************************************************
void NodeCache::addNode(std::string cacheId, AtNode* node)
{
	ArnoldNodeCache[cacheId] = std::string(AiNodeGetName(node));

}