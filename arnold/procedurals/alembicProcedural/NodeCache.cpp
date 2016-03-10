#include "NodeCache.h"


namespace {
	// to return a reference when the cache is not found
	std::vector<CachedNodeFile> emptyCreatedNodes;
}

NodeCache::NodeCache(AtCritSec mycs)
{
	
	lock = mycs;
}

NodeCache::~NodeCache()
{
	AiMsgDebug("\t[Alembic Procedural] Removing %i nodes from the cache", ArnoldNodeCache.size());
	ArnoldNodeCache.clear();

}

//-*************************************************************************
// getCachedNode
// This function return the the mesh node if already in the cache.
// Otherwise, return NULL.
//-*************************************************************************
AtNode* NodeCache::getCachedNode(const std::string& cacheId)
{
	AiMsgDebug("Searching for %s", cacheId.c_str());
	AtScopedLock sc(lock);
	std::map<std::string, std::string>::const_iterator I = ArnoldNodeCache.find(cacheId);
	if (I != ArnoldNodeCache.end())
		return AiNodeLookUpByName(I->second.c_str());
	else
		return 0;
}

//-*************************************************************************
// addNode
// This function adds a node in the cache.
//-*************************************************************************
void NodeCache::addNode(const std::string& cacheId, AtNode* node)
{
	AtScopedLock sc(lock);
	ArnoldNodeCache[cacheId] = std::string(AiNodeGetName(node));
}


// The node collector is a thread-safe class that accumlate AtNode created in the procedural.
NodeCollector::NodeCollector(AtCritSec mycs)
{
	lock = mycs;
}

NodeCollector::~NodeCollector()
{
	AiMsgDebug("Deleting node collector (%i collected nodes)", ArnoldNodeCollector.size());
	ArnoldNodeCollector.clear();
}

//-*************************************************************************
// addNode
// This function adds a node in the node collector.
//-*************************************************************************
void NodeCollector::addNode(AtNode* node)
{
	AtScopedLock sc(lock);
	AiMsgDebug("Adding node %s and type %s", AiNodeGetName(node), AiNodeEntryGetName(AiNodeGetNodeEntry (node)));
	ArnoldNodeCollector.push_back(node);
}

size_t NodeCollector::getNumNodes()
{
	AtScopedLock sc(lock);
	return ArnoldNodeCollector.size();
}

AtNode* NodeCollector::getNode(int num)
{
	AtScopedLock sc(lock);
	if (num < ArnoldNodeCollector.size())
		return ArnoldNodeCollector[num];
	else
	{
		AiMsgError("Returning null node!");
		return 0;
	}
}




FileCache::FileCache(AtCritSec mycs)
{
	lock = mycs;	
}

FileCache::~FileCache()
{
	AiMsgDebug("\t[Alembic Procedural] Removing %i files from the file cache", ArnoldFileCache.size());
	for (std::map<std::string, std::vector<CachedNodeFile>* >::iterator it = ArnoldFileCache.begin();
		 it != ArnoldFileCache.end(); ++it)
	{
		delete it->second;
	}
}

//-*************************************************************************
// getCachedNode
// This function return the the mesh node if already in the cache.
// Otherwise, return NULL.
//-*************************************************************************
const std::vector<CachedNodeFile>& FileCache::getCachedFile(const std::string& cacheId)
{
	AtScopedLock sc(&lock);
	std::map<std::string, std::vector<CachedNodeFile>* >::const_iterator I = ArnoldFileCache.find(cacheId);
	if (I != ArnoldFileCache.end())
		return *I->second;
	else
		return emptyCreatedNodes;
}

const size_t FileCache::hash(std::string const& s)
{
	size_t result = 2166136261U;
	std::string::const_iterator end = s.end();
	for ( std::string::const_iterator iter = s.begin();
			iter != end ;
			++ iter)
	{
		result = 127 * result
				+ static_cast< unsigned char >(*iter);
	}
	return result;
 }

std::string FileCache::getHash(const std::string& fileName,     
							   const std::map<std::string, AtNode*>& shaders,
							   const std::map<std::string, AtNode*>& displacements,
							   const Json::Value& attributesRoot,
							   double frame
							   )
{

	std::ostringstream shaderBuff;
	for (std::map<std::string, AtNode*>::const_iterator it = shaders.begin(); it != shaders.end(); ++it) 
	{
		shaderBuff << it->first;
	}

	std::ostringstream displaceBuff;
	for (std::map<std::string, AtNode*>::const_iterator it = displacements.begin(); it != displacements.end(); ++it) 
	{
		displaceBuff << it->first;
	}

	std::ostringstream buffer;
	std::string cacheId;
	
	buffer << hash(fileName) << "@" << frame << "@" << hash(shaderBuff.str()) << "@" << hash(displaceBuff.str()) << "@" << hash(attributesRoot.toStyledString());

	return buffer.str();

}

//-*************************************************************************
// addNode
// This function adds a node in the cache.
//-*************************************************************************
void FileCache::addCache(const std::string& cacheId, NodeCollector* createdNodes)
{
	AtScopedLock sc(&lock);
	if (ArnoldFileCache.find(cacheId) == ArnoldFileCache.end())
	{
		std::vector<CachedNodeFile>* nodeCache = new std::vector<CachedNodeFile>();

		const size_t numCreatedNodes = createdNodes->getNumNodes();
		if (numCreatedNodes > 0)
			nodeCache->reserve(numCreatedNodes);

		for(size_t i = 0; i < numCreatedNodes; ++i)
		{			
			AtNode* node = createdNodes->getNode(i);

			if(AiNodeEntryGetType(AiNodeGetNodeEntry(node)) == AI_NODE_SHAPE)
			{
				if(AiNodeGetByte(node, "visibility") != 0)
				{
					CachedNodeFile cachedNode;
					cachedNode.node = node;
					cachedNode.matrix = AiNodeGetArray(node, "matrix");
					nodeCache->push_back(cachedNode);
				}
			}
			else if (AiNodeEntryGetType(AiNodeGetNodeEntry(node)) == AI_NODE_LIGHT)
			{
				CachedNodeFile cachedNode;
				cachedNode.node = node;
				cachedNode.matrix = AiNodeGetArray(node, "matrix");
				nodeCache->push_back(cachedNode);
			}
		}

		ArnoldFileCache.insert(std::pair<std::string, std::vector<CachedNodeFile>* >(cacheId, nodeCache));
	}
}
