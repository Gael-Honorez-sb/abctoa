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
	AtNode* node = NULL;
	boost::mutex::scoped_lock readLock( lock );
    std::map<std::string, std::string>::iterator I = ArnoldNodeCache.find(cacheId);
    if (I != ArnoldNodeCache.end())
		node = AiNodeLookUpByName(I->second.c_str());

	readLock.unlock();
    return node;
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


// The node collector is a thread-safe class that accumlate AtNode created in the procedural.
NodeCollector::NodeCollector()
{
	
}

NodeCollector::~NodeCollector()
{
	ArnoldNodeCollector.clear();
}

//-*************************************************************************
// addNode
// This function adds a node in the node collector.
//-*************************************************************************
void NodeCollector::addNode(AtNode* node)
{
	boost::mutex::scoped_lock writeLock( lock );
	ArnoldNodeCollector.push_back(node);
	writeLock.unlock();

}

size_t NodeCollector::getNumNodes()
{
	size_t size;
	boost::mutex::scoped_lock readLock( lock );
	size = ArnoldNodeCollector.size();
	readLock.unlock();
	return size;
}

AtNode* NodeCollector::getNode(int num)
{
	AtNode* node = NULL;
	boost::mutex::scoped_lock readLock( lock );
	if (num <= ArnoldNodeCollector.size())
		node = ArnoldNodeCollector[num];
	readLock.unlock();
	return node;
}