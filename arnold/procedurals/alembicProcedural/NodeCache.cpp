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
	AiMsgInfo("Deleting node collector");
	ArnoldNodeCollector.clear();
}

//-*************************************************************************
// addNode
// This function adds a node in the node collector.
//-*************************************************************************
void NodeCollector::addNode(AtNode* node)
{
	boost::mutex::scoped_lock writeLock( lock );
	//AiMsgInfo("Adding node %s and type %s", AiNodeGetName(node), AiNodeEntryGetName(AiNodeGetNodeEntry (node)));
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




FileCache::FileCache()
{
	
}

FileCache::~FileCache()
{
	ArnoldFileCache.clear();
}

//-*************************************************************************
// getCachedNode
// This function return the the mesh node if already in the cache.
// Otherwise, return NULL.
//-*************************************************************************
std::vector<CachedNodeFile> FileCache::getCachedFile(std::string cacheId)
{
	std::vector<CachedNodeFile> createdNodes;
	boost::mutex::scoped_lock readLock( lock );
    std::map<std::string, std::vector<CachedNodeFile>>::iterator I = ArnoldFileCache.find(cacheId);
    if (I != ArnoldFileCache.end())
		createdNodes = I->second;

	readLock.unlock();
    return createdNodes;
}

const size_t FileCache::hash( std::string const& s )
{
    size_t result = 2166136261U ;
    std::string::const_iterator end = s.end() ;
    for ( std::string::const_iterator iter = s.begin() ;
            iter != end ;
            ++ iter )
    {
        result = 127 * result
                + static_cast< unsigned char >( *iter ) ;
    }
    return result ;
 }

std::string FileCache::getHash(std::string fileName,     
							   std::map<std::string, AtNode*> shaders,
							   std::map<std::string, AtNode*> displacements,
							   std::vector<std::string> attributes
							   )
{

	std::ostringstream shaderBuff;
	for(std::map<std::string, AtNode*>::iterator it = shaders.begin(); it != shaders.end(); ++it) 
	{
		shaderBuff << it->first;
	}

	std::ostringstream displaceBuff;
	for(std::map<std::string, AtNode*>::iterator it = displacements.begin(); it != displacements.end(); ++it) 
	{
		displaceBuff << it->first;
	}

	std::ostringstream attributesBuff;
	for(std::vector<std::string>::iterator it = attributes.begin(); it != attributes.end(); ++it) 
	{
		attributesBuff << *it;
	}

	std::ostringstream buffer;
	std::string cacheId;

	
    buffer << hash(fileName) << "@" << hash(shaderBuff.str()) << "@" << hash(displaceBuff.str()) << "@" << hash(attributesBuff.str());

    cacheId = buffer.str();
	return cacheId;

}

//-*************************************************************************
// addNode
// This function adds a node in the cache.
//-*************************************************************************
void FileCache::addCache(std::string cacheId, NodeCollector* createdNodes)
{
	boost::mutex::scoped_lock writeLock( lock );

	for(int i = 0; i <  createdNodes->getNumNodes(); i++)
	{
		
		AtNode* node = createdNodes->getNode(i);
		if(AiNodeIs(node, "polymesh"))
		{
			if(AiNodeGetByte(node, "visibility") != 0)
			{
				CachedNodeFile cachedNode;
				cachedNode.node = node;
				cachedNode.matrix = AiNodeGetArray(node, "matrix");

				//AiMsgInfo("ADDCACHE Getting obj %i %s and type %s", i, AiNodeGetName(createdNodes->getNode(i)), AiNodeEntryGetName(AiNodeGetNodeEntry (createdNodes->getNode(i))));
				//newCreatedNodes->addNode(createdNodes->getNode(i));
				ArnoldFileCache[cacheId].push_back(cachedNode);

				//AtMatrix m;
				//AiNodeGetMatrix(createdNodes->getNode(i), "matrix", m);
				//printMatrix2(m);
			}

		}
	}

	//ArnoldFileCache[cacheId] = newCreatedNodes;
	writeLock.unlock();

}