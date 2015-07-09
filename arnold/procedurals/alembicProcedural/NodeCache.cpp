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
	boost::mutex::scoped_lock readLock( lock );
    std::map<std::string, std::string>::iterator I = ArnoldNodeCache.find(cacheId);
    if (I != ArnoldNodeCache.end())
	{
		readLock.unlock();
		return AiNodeLookUpByName(I->second.c_str());
	}
	readLock.unlock();
    return NULL;
}

//-*************************************************************************
// addNode
// This function adds a node in the cache.
//-*************************************************************************
void NodeCache::addNode(std::string cacheId, AtNode* node)
{
	boost::mutex::scoped_lock writeLock( lock );
	ArnoldNodeCache[cacheId] = std::string(AiNodeGetName(node));
	writeLock.unlock();

}