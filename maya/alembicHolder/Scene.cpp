//-*****************************************************************************
//
// Copyright (c) 2009-2010,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#include "Scene.h"
#include "gpuCacheMaterial.h"
#include "gpuCacheTimeInterval.h"
#include "gpuCacheDataProvider.h"
#include "IObjectDrw.h"
#include "../../common/PathUtil.h"
#include <Alembic/AbcCoreFactory/IFactory.h>
#include <Alembic/AbcMaterial/IMaterial.h>
#include "boost/foreach.hpp"

//-*****************************************************************************

namespace AlembicHolder {

//-*****************************************************************************
// SCENE CLASS
//-*****************************************************************************

//-*****************************************************************************
Scene::Scene( const std::string &abcFileName, const std::string &objectPath )
  : m_fileName( abcFileName )
  , m_objectPath( objectPath )
  , m_minTime( ( chrono_t )FLT_MAX )
  , m_maxTime( ( chrono_t )-FLT_MAX )
{
    boost::timer Timer;


    Alembic::AbcCoreFactory::IFactory factory;
	factory.setOgawaNumStreams(16);
    m_archive = factory.getArchive(abcFileName );

	if (!m_archive.valid())
    {
        std::cout << "[nozAlembicHolder] ERROR : Can't open file : " << abcFileName << std::endl;
		return ;
    }

    m_topObject = IObject( m_archive, kTop );


    m_selectionPath = "";

    // try to walk to the path
    PathList path;
    TokenizePath( objectPath, "|", path );

    m_drawable.reset( new IObjectDrw( m_topObject, false, path ) );

    readMaterials(m_topObject);


    ABCA_ASSERT( m_drawable->valid(),
                 "Invalid drawable for archive: " << abcFileName );


    m_minTime = m_drawable->getMinTime();
    m_maxTime = m_drawable->getMaxTime();

    if ( m_minTime <= m_maxTime )
    {
        m_drawable->setTime( m_minTime );
    }
    else
    {
        m_minTime = m_maxTime = 0.0;
        m_drawable->setTime( 0.0 );
    }

    ABCA_ASSERT( m_drawable->valid(),
                 "Invalid drawable after reading start time" );



    // Bounds have been formed!
    m_bounds = m_drawable->getBounds();


    std::cout << "[nozAlembicHolder] Opened archive: " << abcFileName << "|" << objectPath << std::endl;

}

//-*****************************************************************************
void Scene::setTime( chrono_t iSeconds )
{
    ABCA_ASSERT( m_archive && m_topObject &&
                 m_drawable && m_drawable->valid(),
                 "Invalid Scene: " << m_fileName );

    if ( m_minTime <= m_maxTime && m_curtime != iSeconds)
    {
        m_drawable->setTime( iSeconds );
        ABCA_ASSERT( m_drawable->valid(),
                     "Invalid drawable after setting time to: "
                     << iSeconds );
        m_curtime = iSeconds;
    }

	m_curFrame = MAnimControl::currentTime().value();

    m_bounds = m_drawable->getBounds();
}

void Scene::setSelectionPath(std::string path)
{
    m_selectionPath = path;
}

int Scene::getNumTriangles() const
{
    return m_drawable->getNumTriangles();

}


//-*****************************************************************************
void Scene::draw( SceneState &s_state, std::string selection, chrono_t iSeconds, holderPrms *m_params, bool flippedNormal)
{

   static MGLFunctionTable *gGLFT = NULL;
   if (gGLFT == NULL)
      gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();

   ABCA_ASSERT( m_archive && m_topObject &&
                 m_drawable && m_drawable->valid(),
                 "Invalid Scene: " << m_fileName );

   // Check if the holder is still at the right frame.
   if (iSeconds != m_curtime)
	   setTime(iSeconds);

    // Get the matrix
    M44d currentMatrix;
    gGLFT->glGetDoublev( MGL_MODELVIEW_MATRIX, ( MGLdouble * )&(currentMatrix[0][0]) );

    DrawContext dctx;
    dctx.setWorldToCamera( currentMatrix );
    dctx.setPointSize( s_state.pointSize );
    dctx.setSelection(selection);
    dctx.setShaderColors(m_params->shaderColors);
    dctx.setNormalFlipped(flippedNormal);
	dctx.setParams(m_params);
	m_drawable->draw( dctx );

}

} // namespace AlembicHolder

namespace AlembicHolder {

const std::string kMaterialsObject("materials");
const std::string kMaterialsGpuCacheTarget("adskMayaGpuCache");
const std::string kMaterialsGpuCacheType("surface");

using namespace CacheReaderAlembicPrivate;

//==============================================================================
// CLASS AlembicCacheMaterialReader
//==============================================================================

class AlembicCacheMaterialReader : boost::noncopyable
{
public:
    AlembicCacheMaterialReader(Alembic::Abc::IObject abcObj);
    ~AlembicCacheMaterialReader();

    TimeInterval sampleMaterial(double seconds);
    MaterialGraph::MPtr get() const;

private:
    // Alembic readers
    const std::string fName;

    // Templated classes to translate Alembic properties.
    template<typename ABC_PROP>
    class ScalarMaterialProp
    {
    public:
        ScalarMaterialProp(Alembic::Abc::ICompoundProperty& parent,
            const std::string& name,
            MaterialNode::MPtr& node)
            : fName(name)
        {
            // Create Alembic input property
            ABC_PROP abcProp(parent, name);
            assert(abcProp.valid());

            // Create reader cache
            fCache.reset(new ScalarPropertyCache<ABC_PROP>());
            fCache->init(abcProp);

            // Find existing property.
            fProp = node->findProperty(name.c_str());
            assert(!fProp || fProp->type() == propertyType<ABC_PROP>());

            if (fProp && fProp->type() != propertyType<ABC_PROP>()) {
                // Something goes wrong..
                fCache.reset();
                fProp.reset();
                return;
            }

            // This is not a known property.
            if (!fProp) {
                fProp = node->createProperty(name.c_str(), propertyType<ABC_PROP>());
                assert(fProp->type() == propertyType<ABC_PROP>());
            }
        }

        TimeInterval sample(double seconds)
        {
            TimeInterval validityInterval(TimeInterval::kInfinite);

            if (fCache && fCache->valid()) {
                fCache->setTime(seconds);
                validityInterval &= fCache->getValidityInterval();

                if (seconds == validityInterval.startTime()) {
                    setMaterialProperty<ABC_PROP>(fCache, fProp, seconds);
                }
            }

            return validityInterval;
        }


    private:
        std::string                                       fName;
        boost::shared_ptr<ScalarPropertyCache<ABC_PROP> > fCache;
        MaterialProperty::MPtr                            fProp;
    };

    template<typename T> static MaterialProperty::Type propertyType()
    {
        assert(0); return MaterialProperty::kFloat;
    }

    template<typename T> static void setMaterialProperty(
        boost::shared_ptr<ScalarPropertyCache<T> >& cache,
        MaterialProperty::MPtr prop,
        double seconds)
    {}

    // The list of animated property caches
    std::vector<ScalarMaterialProp<Alembic::Abc::IBoolProperty> >    fBoolCaches;
    std::vector<ScalarMaterialProp<Alembic::Abc::IInt32Property> >   fInt32Caches;
    std::vector<ScalarMaterialProp<Alembic::Abc::IFloatProperty> >   fFloatCaches;
    std::vector<ScalarMaterialProp<Alembic::Abc::IV2fProperty> >     fFloat2Caches;
    std::vector<ScalarMaterialProp<Alembic::Abc::IV3fProperty> >     fFloat3Caches;
    std::vector<ScalarMaterialProp<Alembic::Abc::IC3fProperty> >     fRGBCaches;
    std::vector<ScalarMaterialProp<Alembic::Abc::IWstringProperty> > fStringCaches;

    // The valid range of the current data in property caches
    TimeInterval fValidityInterval;

    // The material graph currently being filled-in.
    MaterialGraph::MPtr fMaterialGraph;
};

// Template explicit specialization must be in namespace scope.
template<>
inline MaterialProperty::Type AlembicCacheMaterialReader::propertyType<Alembic::Abc::IBoolProperty>()
{
    return MaterialProperty::kBool;
}

template<>
inline MaterialProperty::Type AlembicCacheMaterialReader::propertyType<Alembic::Abc::IInt32Property>()
{
    return MaterialProperty::kInt32;
}

template<>
inline MaterialProperty::Type AlembicCacheMaterialReader::propertyType<Alembic::Abc::IFloatProperty>()
{
    return MaterialProperty::kFloat;
}

template<>
inline MaterialProperty::Type AlembicCacheMaterialReader::propertyType<Alembic::Abc::IV2fProperty>()
{
    return MaterialProperty::kFloat2;
}

template<>
inline MaterialProperty::Type AlembicCacheMaterialReader::propertyType<Alembic::Abc::IV3fProperty>()
{
    return MaterialProperty::kFloat3;
}

template<>
inline MaterialProperty::Type AlembicCacheMaterialReader::propertyType<Alembic::Abc::IC3fProperty>()
{
    return MaterialProperty::kRGB;
}

template<>
inline MaterialProperty::Type AlembicCacheMaterialReader::propertyType<Alembic::Abc::IWstringProperty>()
{
    return MaterialProperty::kString;
}

template<>
inline void AlembicCacheMaterialReader::setMaterialProperty<Alembic::Abc::IBoolProperty>(
    boost::shared_ptr<ScalarPropertyCache<Alembic::Abc::IBoolProperty> >& cache,
    MaterialProperty::MPtr prop,
    double seconds)
{
    prop->setBool(seconds, cache->getValue());
}

template<>
inline void AlembicCacheMaterialReader::setMaterialProperty<Alembic::Abc::IInt32Property>(
    boost::shared_ptr<ScalarPropertyCache<Alembic::Abc::IInt32Property> >& cache,
    MaterialProperty::MPtr prop,
    double seconds)
{
    prop->setInt32(seconds, cache->getValue());
}

template<>
inline void AlembicCacheMaterialReader::setMaterialProperty<Alembic::Abc::IFloatProperty>(
    boost::shared_ptr<ScalarPropertyCache<Alembic::Abc::IFloatProperty> >& cache,
    MaterialProperty::MPtr prop,
    double seconds)
{
    prop->setFloat(seconds, cache->getValue());
}

template<>
inline void AlembicCacheMaterialReader::setMaterialProperty<Alembic::Abc::IV2fProperty>(
    boost::shared_ptr<ScalarPropertyCache<Alembic::Abc::IV2fProperty> >& cache,
    MaterialProperty::MPtr prop,
    double seconds)
{
    Alembic::Abc::V2f value = cache->getValue();
    prop->setFloat2(seconds, value.x, value.y);
}

template<>
inline void AlembicCacheMaterialReader::setMaterialProperty<Alembic::Abc::IV3fProperty>(
    boost::shared_ptr<ScalarPropertyCache<Alembic::Abc::IV3fProperty> >& cache,
    MaterialProperty::MPtr prop,
    double seconds)
{
    Alembic::Abc::V3f value = cache->getValue();
    prop->setFloat3(seconds, value.x, value.y, value.z);
}

template<>
inline void AlembicCacheMaterialReader::setMaterialProperty<Alembic::Abc::IC3fProperty>(
    boost::shared_ptr<ScalarPropertyCache<Alembic::Abc::IC3fProperty> >& cache,
    MaterialProperty::MPtr prop,
    double seconds)
{
    Alembic::Abc::C3f value = cache->getValue();
    prop->setColor(seconds, MColor(value.x, value.y, value.z));
}

template<>
inline void AlembicCacheMaterialReader::setMaterialProperty<Alembic::Abc::IWstringProperty>(
    boost::shared_ptr<ScalarPropertyCache<Alembic::Abc::IWstringProperty> >& cache,
    MaterialProperty::MPtr prop,
    double seconds)
{
    std::wstring value = cache->getValue();
    prop->setString(seconds, value.c_str());
}

//==============================================================================
// CLASS AlembicCacheMaterialReader
//==============================================================================

AlembicCacheMaterialReader::AlembicCacheMaterialReader(Alembic::Abc::IObject abcObj)
    : fName(abcObj.getName()),
    fValidityInterval(TimeInterval::kInvalid)
{
    // Wrap with IMaterial
    Alembic::AbcMaterial::IMaterial material(abcObj, Alembic::Abc::kWrapExisting);

    // Material schema
    Alembic::AbcMaterial::IMaterialSchema schema = material.getSchema();

    // Create the material graph
    fMaterialGraph = boost::make_shared<MaterialGraph>(fName.c_str());

    // The number of nodes in the material
    size_t numNetworkNodes = schema.getNumNetworkNodes();

    // Map: name -> (IMaterialSchema::NetworkNode,MaterialNode)
    typedef std::pair<Alembic::AbcMaterial::IMaterialSchema::NetworkNode, MaterialNode::MPtr> NodePair;
    typedef boost::unordered_map<std::string, NodePair> NodeMap;
    NodeMap nodeMap;

    // Read nodes
    for (size_t i = 0; i < numNetworkNodes; i++) {
        Alembic::AbcMaterial::IMaterialSchema::NetworkNode abcNode = schema.getNetworkNode(i);

        std::string target;
        if (!abcNode.valid() || !abcNode.getTarget(target) || target != kMaterialsGpuCacheTarget) {
            continue;  // Invalid node
        }

        std::string type;
        if (!abcNode.getNodeType(type) || type.empty()) {
            continue;  // Invalid type
        }

        // Node name
        std::string name = abcNode.getName();
        assert(!name.empty());

        // Create material node
        MaterialNode::MPtr node = MaterialNode::create(name.c_str(), type.c_str());
        assert(node);

        fMaterialGraph->addNode(node);
        nodeMap.insert(std::make_pair(name, std::make_pair(abcNode, node)));
    }

    // Initialize property caches.
    BOOST_FOREACH(NodeMap::value_type& val, nodeMap) {
        Alembic::AbcMaterial::IMaterialSchema::NetworkNode& abcNode = val.second.first;
        MaterialNode::MPtr& node = val.second.second;

        // Loop over all child properties
        Alembic::Abc::ICompoundProperty compoundProp = abcNode.getParameters();
        size_t numProps = compoundProp.getNumProperties();
        for (size_t i = 0; i < numProps; i++) {
            const Alembic::Abc::PropertyHeader& header = compoundProp.getPropertyHeader(i);
            const std::string propName = header.getName();

            if (Alembic::Abc::IBoolProperty::matches(header)) {
                fBoolCaches.push_back(
                    ScalarMaterialProp<Alembic::Abc::IBoolProperty>(compoundProp, propName, node));
            }
            else if (Alembic::Abc::IInt32Property::matches(header)) {
                fInt32Caches.push_back(
                    ScalarMaterialProp<Alembic::Abc::IInt32Property>(compoundProp, propName, node));
            }
            else if (Alembic::Abc::IFloatProperty::matches(header)) {
                fFloatCaches.push_back(
                    ScalarMaterialProp<Alembic::Abc::IFloatProperty>(compoundProp, propName, node));
            }
            else if (Alembic::Abc::IV2fProperty::matches(header)) {
                fFloat2Caches.push_back(
                    ScalarMaterialProp<Alembic::Abc::IV2fProperty>(compoundProp, propName, node));
            }
            else if (Alembic::Abc::IV3fProperty::matches(header)) {
                fFloat3Caches.push_back(
                    ScalarMaterialProp<Alembic::Abc::IV3fProperty>(compoundProp, propName, node));
            }
            else if (Alembic::Abc::IC3fProperty::matches(header)) {
                fRGBCaches.push_back(
                    ScalarMaterialProp<Alembic::Abc::IC3fProperty>(compoundProp, propName, node));
            }
            else if (Alembic::Abc::IWstringProperty::matches(header)) {
                fStringCaches.push_back(
                    ScalarMaterialProp<Alembic::Abc::IWstringProperty>(compoundProp, propName, node));
            }
        }
    }

    // Read connections
    BOOST_FOREACH(NodeMap::value_type& val, nodeMap) {
        Alembic::AbcMaterial::IMaterialSchema::NetworkNode& abcNode = val.second.first;
        MaterialNode::MPtr& node = val.second.second;

        // Loop over the connections and connect properties
        size_t numConnections = abcNode.getNumConnections();
        for (size_t i = 0; i < numConnections; i++) {
            std::string inputName, connectedNodeName, connectedOutputName;
            abcNode.getConnection(i, inputName, connectedNodeName, connectedOutputName);

            // Find destination property
            MaterialProperty::MPtr prop = node->findProperty(inputName.c_str());

            // Find source node
            MaterialNode::MPtr srcNode;
            NodeMap::iterator it = nodeMap.find(connectedNodeName);
            if (it != nodeMap.end()) {
                srcNode = (*it).second.second;
            }

            // Find source property
            MaterialProperty::MPtr srcProp;
            if (srcNode) {
                srcProp = srcNode->findProperty(connectedOutputName.c_str());
            }

            // Make the connection
            if (prop && srcNode && srcProp) {
                prop->connect(srcNode, srcProp);
            }
        }
    }

    // Read Terminal node (ignore output)
    std::string rootNodeName, rootOutput;
    if (schema.getNetworkTerminal(kMaterialsGpuCacheTarget, kMaterialsGpuCacheType, rootNodeName, rootOutput)) {
        NodeMap::iterator it = nodeMap.find(rootNodeName);
        if (it != nodeMap.end()) {
            fMaterialGraph->setRootNode((*it).second.second);
        }
    }
}

AlembicCacheMaterialReader::~AlembicCacheMaterialReader()
{}

TimeInterval AlembicCacheMaterialReader::sampleMaterial(double seconds)
{
    TimeInterval validityInterval(TimeInterval::kInfinite);

    BOOST_FOREACH(ScalarMaterialProp<Alembic::Abc::IBoolProperty>& cache, fBoolCaches) {
        validityInterval &= cache.sample(seconds);
    }

    BOOST_FOREACH(ScalarMaterialProp<Alembic::Abc::IInt32Property>& cache, fInt32Caches) {
        validityInterval &= cache.sample(seconds);
    }

    BOOST_FOREACH(ScalarMaterialProp<Alembic::Abc::IFloatProperty>& cache, fFloatCaches) {
        validityInterval &= cache.sample(seconds);
    }

    BOOST_FOREACH(ScalarMaterialProp<Alembic::Abc::IV2fProperty>& cache, fFloat2Caches) {
        validityInterval &= cache.sample(seconds);
    }

    BOOST_FOREACH(ScalarMaterialProp<Alembic::Abc::IV3fProperty>& cache, fFloat3Caches) {
        validityInterval &= cache.sample(seconds);
    }

    BOOST_FOREACH(ScalarMaterialProp<Alembic::Abc::IC3fProperty>& cache, fRGBCaches) {
        validityInterval &= cache.sample(seconds);
    }

    BOOST_FOREACH(ScalarMaterialProp<Alembic::Abc::IWstringProperty>& cache, fStringCaches) {
        validityInterval &= cache.sample(seconds);
    }

    return validityInterval;
}

MaterialGraph::MPtr AlembicCacheMaterialReader::get() const
{
    // Check invalid graph.
    if (!fMaterialGraph || !fMaterialGraph->rootNode() || fMaterialGraph->getNodes().empty()) {
        return MaterialGraph::MPtr();
    }
    return fMaterialGraph;
}

} // namespace AlembicHolder

namespace AlembicHolder {

void Scene::readMaterials(const IObject& topObject)
{
    using namespace AlembicHolder;
    //using namespace AlembicHolder::CacheReaderAlembicPrivate;

    //if (!valid()) {
    //    m_materials.reset();
    //    return;
    //}

    try {
        // Find "/materials"
        Alembic::Abc::IObject materialsObject = topObject.getChild(kMaterialsObject);

        // "/materials" doesn't exist!
        if (!materialsObject.valid()) {
            //return MaterialGraphMap::Ptr();
            m_materials.reset();
        }

        AlembicHolder::MaterialGraphMap::MPtr materials = boost::make_shared<AlembicHolder::MaterialGraphMap>();

        // Read materials one by one. Hierarchical materials are not supported.
        for (size_t i = 0; i < materialsObject.getNumChildren(); i++) {
            Alembic::Abc::IObject object = materialsObject.getChild(i);
            if (Alembic::AbcMaterial::IMaterial::matches(object.getHeader())) {
                AlembicCacheMaterialReader reader(object);

                // Read the material
                TimeInterval interval = reader.sampleMaterial(
                    -std::numeric_limits<double>::max());
                while (interval.endTime() != std::numeric_limits<double>::max()) {
                    interval = reader.sampleMaterial(interval.endTime());
                }

                MaterialGraph::MPtr graph = reader.get();
                if (graph) {
                    materials->addMaterialGraph(graph);
                }
            }
        }

        // No materials..
        if (materials->getGraphs().empty()) {
            m_materials.reset();
        } else {
            m_materials = materials;
        }
    }
    //catch (CacheReaderInterruptException& ex) {
    //    // pass upward
    //    throw ex;
    //}
    catch (std::exception& ex) {
    //    DisplayError(kReadFileErrorMsg, fFile.resolvedFullName(), ex.what());
    //    return MaterialGraphMap::Ptr();
    }
}

} // End namespace AlembicHolder
