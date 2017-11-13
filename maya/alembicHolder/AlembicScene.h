#pragma once

#include "Cache.h"
#include "Foundation.h"
#include <maya/MHWGeometry.h>
#include <tbb/mutex.h>
#include <boost/range/iterator_range.hpp>
#include <array>
#include <memory>
#include <string>

namespace AlembicHolder {

typedef Alembic::AbcCoreAbstract::chrono_t chrono_t;
typedef Alembic::AbcCoreAbstract::index_t index_t;

class Hierarchy {
public:
    typedef uint32_t NodeID;
    typedef uint32_t NodeCount;
    static const NodeID NO_PARENT;
    enum class NodeType : uint16_t { XFORM, POLYMESH, CURVES, POINTS, NUM_TYPES, UNKNOWN };
    static constexpr const uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

    struct Node {
        Alembic::Abc::IObject source_object;
        NodeID id;
        NodeID children_start;
        uint16_t children_count;
        NodeType type;
    };

    // Read a hierarchy starting from top_object.
    // If geometry_path is not empty, cull everything which is not one of or
    // descendant of the nodes on the geometry_path.
    Hierarchy(Alembic::Abc::IObject top_object, const std::string& geometry_path = "");

    // Get node from node index.
    const Node& node(NodeID index) const { return m_nodes[index]; }
    Node& node(NodeID index) { return m_nodes[index]; }

    // Get total node count.
    NodeCount nodeCount() const { return uint32_t(m_nodes.size()); }

    // Get the root node of the hierarchy.
    const Node& rootNode() const { return node(0); }
    Node& rootNode() { return node(0); }

    bool empty() const { return m_nodes.empty(); }

    // Get an iterator range to the children of the given node.
    typedef boost::iterator_range<std::vector<Node>::const_iterator> NodeRange;
    NodeRange childrenOf(NodeID node_id) const { return childrenOf(node(node_id)); }
    NodeRange childrenOf(const Node& node) const {
        const auto start = m_nodes.begin() + node.children_start;
        return { start, start + node.children_count };
    }

private:
    std::vector<Node> m_nodes;
};

class HierarchyNodeVisibility {
public:
    HierarchyNodeVisibility(Hierarchy& hierarchy);
    bool isNodeVisible(const Hierarchy::NodeID& node_id, const Alembic::Abc::ISampleSelector& ss) const;
private:
    std::vector<Alembic::AbcGeom::IVisibilityProperty> m_visibility_properties;
};

class HierarchyNodeCategories {
public:
    typedef Hierarchy::NodeCount XformID;
    typedef Hierarchy::NodeCount DrawableID;

    HierarchyNodeCategories(const Hierarchy& hierarchy);

    // Returns:
    //  - the number of xforms with id < `node_id` if node `node_id` is an xform (aka the XformID).
    //  - the number of drawables with id < `node_id` if node `node_id` is a drawable (aba the DrawableID).
    //  - INDEX_UNKNOWN otherwise
    Hierarchy::NodeCount categoryIndex(Hierarchy::NodeID node_id) const { return m_category_index[node_id]; }

    bool isXform(const Hierarchy::Node& node) const;
    Hierarchy::NodeCount countByType(Hierarchy::NodeType type) const { return m_count_by_type[size_t(type)]; }

    bool isDrawable(const Hierarchy::Node& node) const;
    Hierarchy::NodeCount drawableCount() const { return Hierarchy::NodeCount(m_drawables.size()); }

    struct DrawableNodeRef {
        DrawableID drawable_id;
        const Hierarchy::Node& node_ref;
        DrawableNodeRef(DrawableID drawable_id_, const Hierarchy::Node& node_ref_)
            : drawable_id(drawable_id_), node_ref(node_ref_) {}
    };
    class DrawableIterator : public std::iterator<std::forward_iterator_tag, DrawableNodeRef> {
    public:
        value_type operator*() const {
            return {m_drawable_id, m_parent.m_hierarchy.node(m_parent.m_drawables[m_drawable_id]) };
        }
        DrawableIterator& operator++() { m_drawable_id++; return *this; }
        bool operator!=(const DrawableIterator& rhs) const { return m_drawable_id != rhs.m_drawable_id; }
    private:
        friend class HierarchyNodeCategories;
        const HierarchyNodeCategories& m_parent;
        DrawableID m_drawable_id;
        DrawableIterator(const HierarchyNodeCategories& parent, DrawableID drawable_id)
            : m_parent(parent), m_drawable_id(drawable_id) {}
    };
    friend class DrawableIterator;

    typedef boost::iterator_range<DrawableIterator> Drawables;
    Drawables drawables() const {
        return {DrawableIterator(*this, 0), DrawableIterator(*this, drawableCount())};
    }

private:
    const Hierarchy& m_hierarchy;
    std::array<Hierarchy::NodeCount, size_t(Hierarchy::NodeType::NUM_TYPES)> m_count_by_type;
    std::vector<Hierarchy::NodeID> m_drawables;
    std::vector<Hierarchy::NodeCount> m_category_index;
};


// === Geometry sampling =======================================================

// Maps vertex buffer index to indices indexing into the respective Alembic
// sample arrays.
struct IndexMapping {
    std::vector<int32_t> vertex_index_from_face_index;
    std::vector<int32_t> position_indices;
    std::vector<uint32_t> normal_indices;
};

struct IndexBufferSample {
    std::vector<uint32_t> triangle_buffer;
    std::vector<uint32_t> wireframe_buffer;
    IndexMapping index_mapping;
};
typedef Cache<index_t, IndexBufferSample> IndexBufferSampleCache;
typedef IndexBufferSampleCache::ValuePtr IndexBufferSamplePtr;

struct GeometrySample {
    Box3f bbox;
    std::vector<V3f> positions;
    std::vector<V3f> normals;
    std::vector<V3f> tangents;
    std::vector<V3f> bitangents;
};

typedef Cache<index_t, GeometrySample> GeometrySampleCache;
typedef GeometrySampleCache::ValuePtr GeometrySamplePtr;

template <typename T>
struct InterpolationData {
    T endpoints[2];
    float alpha;
    InterpolationData() : alpha(0) {}
};

template <typename T>
inline void lerpSpans(float alpha, const Span<const T>& input1, const Span<const T>& input2, const Span<T>& output)
{
    assert(input2.size == input1.size);
    assert(output.size == input1.size);
    for (size_t i = 0; i < input1.size; ++i) {
        output[i] = (1 - alpha) * input1[i] + alpha * input2[i];
    }
}

enum class GeometryType {
    POINTS,
    LINES,
    TRIANGLES
};

struct DrawableCacheHandles {
    IndexBufferSamplePtr indices;
    // If interpolation is not required, the first element should hold the
    // sample to use.
    InterpolationData<GeometrySamplePtr> geometry;
};

// The purpose of this struct is to keep a reference to cached buffers needed to
// compute any data needed by visualization.
struct DrawableSample {
    Imath::M44f world_matrix;
    Imath::Box3f bbox;
    uint32_t drawable_id;
    bool visible;

    GeometryType type;
    DrawableCacheHandles cache_handles;

    DrawableSample() : drawable_id(Hierarchy::INVALID_INDEX), visible(false), type(GeometryType::POINTS) {}
};
typedef std::vector<DrawableSample> DrawableSampleVector;

class DrawableBufferSampler {
public:
    DrawableBufferSampler(const Hierarchy::Node& node);
    void sampleBuffers(chrono_t time, DrawableCacheHandles& out_handles, Box3f& out_bbox);
    GeometryType geometryType() const { return m_type; }

private:
    GeometryType m_type;
    Alembic::AbcGeom::IInt32ArrayProperty m_face_counts_property;
    Alembic::AbcGeom::IInt32ArrayProperty m_face_indices_property;
    Alembic::AbcGeom::IP3fArrayProperty m_positions_property;
    Alembic::AbcGeom::IN3fGeomParam m_normals_param;

    Alembic::AbcGeom::MeshTopologyVariance m_topology_variance;
    bool m_normals_are_indexed;

    IndexBufferSampleCache m_index_buffer_cache;
    GeometrySampleCache m_geometry_sample_cache;
};


// === Alembic scene ===========================================================

struct AlembicSceneKey {
    FileRef file_ref;
    std::string root_path;
    bool operator==(const AlembicSceneKey& rhs) const { return file_ref == rhs.file_ref && root_path == rhs.root_path; }
    bool operator!=(const AlembicSceneKey& rhs) const { return !(*this == rhs); }
    AlembicSceneKey() {}
    AlembicSceneKey(const FileRef& file_ref_, const std::string& root_path_)
        : file_ref(file_ref_), root_path(root_path_)
    {}
};

struct AlembicSceneKeyHasher {
    size_t operator()(const AlembicSceneKey& scene_key) const
    {
        size_t res = scene_key.file_ref.hash();
        boost::hash_combine(res, scene_key.root_path);
        return res;
    }
};

struct AlembicLoadFailedException {};

struct HierarchyStat {
    Imath::Box3f bbox;
    size_t triangle_count;
    HierarchyStat() : triangle_count(0)
    {
        bbox.makeEmpty();
    }
};

class AlembicScene {
public:
    AlembicScene(const AlembicSceneKey& scene_key);

    // Sample geometry of each drawable in the hierarchy at time 'time'.
    void sampleHierarchy(chrono_t time, DrawableSampleVector& out_samples, HierarchyStat& out_hierarchy_stat);

    const Hierarchy& hierarchy() const { return m_hierarchy; }
    const HierarchyNodeCategories& nodeCategories() const { return m_categories; }

    const std::string& getDrawableName(HierarchyNodeCategories::DrawableID drawable_id) const
    {
        return m_drawable_names[drawable_id];
    }

private:
    // The order of these declaration matter. If the construction of something
    // depends on another object, the dependent should be declared later.
    Hierarchy m_hierarchy;
    HierarchyNodeCategories m_categories;
    HierarchyNodeVisibility m_visibility;
    std::vector<std::string> m_drawable_names;

    std::vector<DrawableBufferSampler> m_samplers;
    tbb::mutex m_mutex;
};

typedef std::shared_ptr<AlembicScene> AlembicScenePtr;

class AlembicSceneCache {
public:
    static AlembicSceneCache& instance();
    AlembicScenePtr getScene(const AlembicSceneKey& key);
private:
    Cache<AlembicSceneKey, AlembicScene, AlembicSceneKeyHasher> m_scene_cache;
    tbb::mutex m_cache_mutex;
};

} // namespace AlembicHolder
