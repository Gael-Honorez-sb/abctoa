#include "AlembicScene.h"
#include "../../common/PathUtil.h"
#include <Alembic/AbcCoreFactory/IFactory.h>
#include <ImathBoxAlgo.h>
#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

namespace AlembicHolder {

const uint32_t Hierarchy::NO_PARENT = std::numeric_limits<uint32_t>::max();

namespace {
    struct QueueEntry {
        Alembic::Abc::IObject source_object;
        Hierarchy::NodeID parent;
        QueueEntry(const Alembic::Abc::IObject& abc_object_, Hierarchy::NodeID parent_)
            : source_object(abc_object_), parent(parent_)
        {}
    };

    Hierarchy::NodeType getType(const Alembic::Abc::IObject& object)
    {
        const auto& header = object.getHeader();
        if (Alembic::AbcGeom::IXform::matches(header)) {
            return Hierarchy::NodeType::XFORM;
        } else if (Alembic::AbcGeom::IPolyMesh::matches(header)) {
            return Hierarchy::NodeType::POLYMESH;
        } else if (Alembic::AbcGeom::ICurves::matches(header)) {
            return Hierarchy::NodeType::CURVES;
        } else if (Alembic::AbcGeom::IPoints::matches(header)) {
            return Hierarchy::NodeType::POINTS;
        }
        return Hierarchy::NodeType::UNKNOWN;
    }
} // unnamed namespace

Hierarchy::Hierarchy(Alembic::Abc::IObject top_object, const std::string& geometry_path)
{
    if (!top_object.valid())
        return;

    PathList path;
    TokenizePath(geometry_path, "|/", path);
    auto path_iter = path.begin();

    std::queue<QueueEntry> queue;

    queue.push({top_object, NO_PARENT});
    m_nodes.resize(1);

    // Build a hierarchy in BFS order.
    uint32_t node_count = 0;
    while (!queue.empty()) {
        auto entry = queue.front();
        queue.pop();

        const uint32_t node_id = node_count++;
        Node& node = m_nodes[node_id];
        node.id = node_id;
        node.source_object = entry.source_object;
        node.children_start = uint32_t(m_nodes.size());
        node.type = getType(entry.source_object);

        uint16_t children_count = 0;
        const auto pushChildToQueue = [&queue, node_id, &children_count](Alembic::Abc::IObject child_object) {
            if (!child_object.valid())
                return;

            // Cull constant invisible subtrees.
            const auto visibility = Alembic::AbcGeom::GetVisibilityProperty(child_object);
            if (visibility.valid() && visibility.isConstant() && visibility.getValue(0) == false)
                return;

            queue.push({child_object, /* parent = */ node_id});
            children_count += 1;
        };
        if (path_iter != path.end()) {
            // Traverse down the path.
            const auto& child_name = *path_iter;
            pushChildToQueue(entry.source_object.getChild(child_name));
            ++path_iter;
        } else {
            // Traverse down to children.
            for (uint32_t i = 0; i < entry.source_object.getNumChildren(); ++i) {
                pushChildToQueue(entry.source_object.getChild(i));
            }
        }
        node.children_count = children_count;

        // Only resize now, since this may invalidate the node reference above.
        if (children_count != 0)
            m_nodes.resize(node.children_start + children_count);
    }
}

namespace {

    Alembic::Abc::IObject getAlembicTopObject(const std::vector<std::string>& file_names)
    {
        Alembic::AbcCoreFactory::IFactory factory;
        factory.setOgawaNumStreams(16);
        auto archive = factory.getArchive(file_names);
        if (!archive.valid())
            throw AlembicLoadFailedException();
        return Alembic::Abc::IObject(archive, Alembic::Abc::kTop);
    }

    const std::string kArbGeomParamsName(".arbGeomParams");

    // Read diffuse color and diffuse texture information from an alembic object.
    void readStaticMaterial(const Alembic::Abc::IObject& object, StaticMaterial& out_static_material)
    {
        Alembic::AbcCoreAbstract::CompoundPropertyReaderPtr property_reader;
        const Alembic::AbcCoreAbstract::PropertyHeader* arb_params_header = nullptr;

        const auto header = object.getHeader();
        if (Alembic::AbcGeom::IPolyMesh::matches(header)) {
            auto schema = Alembic::AbcGeom::IPolyMesh(object).getSchema();
            property_reader = schema.getPtr();
            arb_params_header = schema.getPropertyHeader(kArbGeomParamsName);
        } else if (Alembic::AbcGeom::ICurves::matches(header)) {
            auto schema = Alembic::AbcGeom::ICurves(object).getSchema();
            property_reader = schema.getPtr();
            arb_params_header = schema.getPropertyHeader(kArbGeomParamsName);

        } else if (Alembic::AbcGeom::IPoints::matches(header)) {
            auto schema = Alembic::AbcGeom::IPoints(object).getSchema();
            property_reader = schema.getPtr();
            arb_params_header = schema.getPropertyHeader(kArbGeomParamsName);
        }

        out_static_material.diffuse_color = kDefaultDiffuseColor;

        if (arb_params_header && arb_params_header->isCompound()) {
            const Alembic::Abc::ICompoundProperty arb_geom_params =
                Alembic::Abc::ICompoundProperty(property_reader, kArbGeomParamsName);

            // Diffuse color.
            Alembic::Abc::IC3fArrayProperty diffuseColorProperty;
            try {
                diffuseColorProperty = Alembic::Abc::IC3fArrayProperty(arb_geom_params.getPtr(), kDiffuseColorParamName);
                const auto color_sample = diffuseColorProperty.getValue();
                if (color_sample && color_sample->size() != 0) {
                    out_static_material.diffuse_color = color_sample->get()[0];
                }
            } catch (Alembic::Util::Exception) {
            }

            // Texture path.
            Alembic::Abc::IStringArrayProperty texturePathProperty;
            try {
                texturePathProperty = Alembic::Abc::IStringArrayProperty(arb_geom_params.getPtr(), kTexturePathParamName);
                const auto string_sample = texturePathProperty.getValue();
                if (string_sample && string_sample->size() != 0) {
                    out_static_material.diffuse_texture_path = string_sample->get()[0];
                }
            } catch (Alembic::Util::Exception) {
            }
        }
    }
} // unnamed namespace

AlembicScene::AlembicScene(const AlembicSceneKey& scene_key)
    : m_hierarchy(getAlembicTopObject(scene_key.file_ref.paths()), scene_key.root_path)
    , m_categories(m_hierarchy)
    , m_visibility(m_hierarchy)
{
    // Get the full names of drawable nodes.
    m_drawable_names.reserve(m_categories.drawableCount());
    for (const auto& drawable : m_categories.drawables()) {
        m_drawable_names.push_back(drawable.node_ref.source_object.getFullName());
    }

    // Read the diffuse color and diffuse texture values stored in the alembic,
    // then load the textures. These are assumed to be constant in time.
    m_static_materials.resize(m_categories.drawableCount());
    for (const auto& drawable : m_categories.drawables()) {
        readStaticMaterial(
            drawable.node_ref.source_object,
            m_static_materials[drawable.drawable_id]);
    }

    // Initialize drawable samplers.
    m_samplers.reserve(m_categories.drawableCount());
    for (const auto& drawable : m_categories.drawables()) {
        m_samplers.emplace_back(drawable.node_ref);
    }
}

struct TraverseState {
    M44f world_matrix;
    const Hierarchy::Node* node_ptr;
    bool is_parent_visible;
    TraverseState(const Hierarchy::Node& node, const M44f& world_matrix_ = M44f(), bool is_parent_visible_ = true)
        : node_ptr(&node), world_matrix(world_matrix_), is_parent_visible(is_parent_visible_)
    {}
};

void AlembicScene::sampleHierarchy(chrono_t time, DrawableSampleVector& out_samples, HierarchyStat& out_hierarchy_stat)
{
    tbb::mutex::scoped_lock guard(m_mutex);

    const auto drawable_count = m_categories.drawableCount();
    out_samples.resize(drawable_count);
    out_hierarchy_stat = HierarchyStat();

    if (m_hierarchy.empty())
        return;

    // Calculate visibility, world matrices and polymesh geometry data.
    // Top to bottom BFS traversal.
    {
        const auto floor_ss = Alembic::Abc::ISampleSelector(time, Alembic::Abc::ISampleSelector::kFloorIndex);
        std::queue<TraverseState> q;
        q.push(m_hierarchy.rootNode());
        while (!q.empty()) {
            const auto& state = q.front();
            const auto& node = *state.node_ptr;
            auto world_matrix = state.world_matrix;
            const auto is_visible = state.is_parent_visible && m_visibility.isNodeVisible(node.id, floor_ss);
            q.pop();

            const auto category_index = m_categories.categoryIndex(node.id);

            // Local matrix (xforms).
            if (node.type == Hierarchy::NodeType::XFORM) {
                Alembic::AbcGeom::IXform xform = node.source_object;
                Alembic::AbcGeom::XformSample floor_xform;
                Alembic::AbcGeom::XformSample ceil_xform;
                const auto xform_schema = xform.getSchema();

                // Interpolate matrices.
                index_t floor_index, ceil_index;
                chrono_t floor_time, ceil_time;
                std::tie(floor_index, floor_time) = xform_schema.getTimeSampling()->getFloorIndex(time, xform_schema.getNumSamples());
                std::tie(ceil_index, ceil_time) = xform_schema.getTimeSampling()->getCeilIndex(time, xform_schema.getNumSamples());
                xform.getSchema().get(floor_xform, floor_index);
                xform.getSchema().get(ceil_xform, ceil_index);
                const float alpha = (ceil_time == floor_time) ? 0 : float((time - floor_time) / (ceil_time - floor_time));
                const auto xform_matrix = M44f(floor_xform.getMatrix()) * (1 - alpha) + M44f(ceil_xform.getMatrix()) * alpha;

                world_matrix = xform_matrix * world_matrix;
            }

            // Drawable.
            if (m_categories.isDrawable(node)) {
                const auto drawable_index = category_index;
                auto& sample = out_samples[drawable_index];
                sample.visible = is_visible;
                sample.drawable_id = drawable_index;
                sample.world_matrix = world_matrix;
                sample.type = m_samplers[drawable_index].geometryType();

                // Geometry buffers.
                if (sample.visible) {
                    m_samplers[drawable_index].sampleBuffers(time, sample.cache_handles, sample.bbox);
                    out_hierarchy_stat.bbox.extendBy(Imath::transform(sample.bbox, sample.world_matrix));
                } else {
                    sample.bbox.makeEmpty();
                    sample.cache_handles.indices.reset();
                    for (auto& endpoint : sample.cache_handles.geometry.endpoints)
                        endpoint.reset();
                    for (auto& endpoint : sample.cache_handles.texcoords.endpoints)
                        endpoint.reset();
                }

                // Triangle count stat.
                if (sample.cache_handles.indices)
                    out_hierarchy_stat.triangle_count += (sample.cache_handles.indices->triangle_buffer.size() / 3);
            }

            // Process children.
            for (const auto& child : m_hierarchy.childrenOf(node))
                q.push({child, world_matrix, is_visible});
        }
    }
}

AlembicSceneCache& AlembicSceneCache::instance()
{
    static AlembicSceneCache instance;
    return instance;
}

AlembicScenePtr AlembicSceneCache::getScene(const AlembicSceneKey& key)
{
    tbb::mutex::scoped_lock guard(m_cache_mutex);

    auto ptr = m_scene_cache.get(key);
    if (ptr)
        return ptr;

    try {
        return m_scene_cache.put(key, new AlembicScene(key));
    } catch (AlembicLoadFailedException) {
        return nullptr;
    }
}

HierarchyNodeVisibility::HierarchyNodeVisibility(Hierarchy& hierarchy)
{
    m_visibility_properties.reserve(hierarchy.nodeCount());
    for (Hierarchy::NodeID node_id = 0; node_id < hierarchy.nodeCount(); ++node_id) {
        auto& node = hierarchy.node(node_id);
        m_visibility_properties.push_back(Alembic::AbcGeom::GetVisibilityProperty(node.source_object));
    }
}

bool HierarchyNodeVisibility::isNodeVisible(const Hierarchy::NodeID& node_id, const Alembic::Abc::ISampleSelector& ss) const
{
    const auto& visibility = m_visibility_properties[node_id];
    return !visibility.valid() || (visibility.getValue(ss) != Alembic::AbcGeom::kVisibilityHidden);
}


HierarchyNodeCategories::HierarchyNodeCategories(const Hierarchy& hierarchy)
    : m_hierarchy(hierarchy)
{
    // Compute hierarchy statistics.
    m_count_by_type.fill(0);
    m_category_index.resize(m_hierarchy.nodeCount());
    m_drawables.reserve(m_hierarchy.nodeCount());
    for (Hierarchy::NodeID node_id = 0; node_id < m_hierarchy.nodeCount(); ++node_id) {
        auto& node = m_hierarchy.node(node_id);
        if (isDrawable(node)) {
            m_category_index[node_id] = DrawableID(m_drawables.size());
            m_drawables.push_back(node_id);
        } else if (isXform(node)) {
            m_category_index[node_id] = m_count_by_type[size_t(node.type)];
        } else {
            m_category_index[node_id] = Hierarchy::INVALID_INDEX;
        }
        // Increment count_by_type last, so it can be used as index above.
        if (node.type != Hierarchy::NodeType::UNKNOWN)
            m_count_by_type[size_t(node.type)] += 1;
    }
}

bool HierarchyNodeCategories::isXform(const Hierarchy::Node& node) const
{
    return node.type == Hierarchy::NodeType::XFORM;
}

bool HierarchyNodeCategories::isDrawable(const Hierarchy::Node & node) const
{
    return node.type == Hierarchy::NodeType::POLYMESH ||
        node.type == Hierarchy::NodeType::CURVES ||
        node.type == Hierarchy::NodeType::POINTS;
}

namespace {

// The Alembic FaceIndices property is an array of int32s, but the normal
// and UV index properties are arrays of uint32s. Why? Dunno. Ask the
// designers of the Alembic library.
struct IndexTriplet {
    int32_t position_index;
    uint32_t normal_index;
    uint32_t uv_index;
    IndexTriplet(int32_t position_index_, uint32_t normal_index_, uint32_t uv_index_)
        : position_index(position_index_)
        , normal_index(normal_index_)
        , uv_index(uv_index_)
    {
    }
    bool operator==(const IndexTriplet& rhs) const
    {
        return position_index == rhs.position_index &&
            normal_index == rhs.normal_index &&
            uv_index == rhs.uv_index;
    }
};

struct IndexTripletHasher {
    size_t operator()(const IndexTriplet& index_triplet) const
    {
        size_t res = std::hash<int32_t>()(index_triplet.position_index);
        boost::hash_combine(res, index_triplet.normal_index);
        boost::hash_combine(res, index_triplet.uv_index);
        return res;
    }
};

// If normals or uvs are faceVarying, a mapping from face indices to
// vertex buffer indices has to be computed.
void computeIndexMapping(
    const Int32ArraySample& face_indices,
    const UInt32ArraySamplePtr& normal_indices,
    const UInt32ArraySamplePtr& uv_indices,
    IndexMapping& output)
{
    // This will map a triplet of position, normal and uv indices to a
    // vertex index identifying a unique position, normal and uv triplet.
    std::unordered_map<IndexTriplet, uint32_t, IndexTripletHasher> index_map;

    // Fill the index map by index triplets.
    const auto face_vertex_count = face_indices.size();
    output.vertex_index_from_face_index.resize(face_vertex_count);
    uint32_t vertex_count = 0;
    for (size_t i_face_vertex = 0; i_face_vertex < face_vertex_count; ++i_face_vertex) {
        const auto position_index = face_indices.get()[i_face_vertex];
        const auto normal_index = normal_indices ? normal_indices->get()[i_face_vertex] : position_index;
        const auto uv_index = uv_indices ? uv_indices->get()[i_face_vertex] : position_index;
        const IndexTriplet index_triplet = { position_index, normal_index, uv_index };
        const auto insert_result = index_map.insert({ index_triplet, vertex_count });
        if (insert_result.second) {
            // The insertion was succesfull, this is the first occurence of this index triplet.
            vertex_count += 1;
        }
        output.vertex_index_from_face_index[i_face_vertex] = insert_result.first->second;
    }

    output.position_indices.resize(vertex_count);
    if (normal_indices)
        output.normal_indices.resize(vertex_count);
    else
        output.normal_indices.clear();
    if (uv_indices)
        output.uv_indices.resize(vertex_count);
    else
        output.uv_indices.clear();

    for (const auto& triplet_index_pair : index_map) {
        const auto& triplet = triplet_index_pair.first;
        const auto vertex_index = triplet_index_pair.second;
        output.position_indices[vertex_index] = triplet.position_index;
        if (normal_indices)
            output.normal_indices[vertex_index] = triplet.normal_index;
        if (uv_indices)
            output.uv_indices[vertex_index] = triplet.uv_index;
    }
}

// Compute normals by averaging face (triangle) normals.
void generateNormals(const std::vector<uint32_t>& triangle_indices, const std::vector<V3f>& positions, std::vector<V3f>& out_normals)
{
    out_normals.assign(positions.size(), V3f(0, 0, 0));
    for (size_t i = 0; i < triangle_indices.size(); i += 3) {
        const auto& a = positions[triangle_indices[i + 0]];
        const auto& b = positions[triangle_indices[i + 1]];
        const auto& c = positions[triangle_indices[i + 2]];
        const auto face_normal = (c - a).cross(b - a);
        for (size_t j = 0; j < 3; j++)
            out_normals[triangle_indices[i + j]] += face_normal;
    }
    for (auto& n : out_normals) {
        n.normalize();
    }
}

template <typename T, typename IndexT>
void fillBuffer(
    const Span<const T>& values,
    const Span<const IndexT>& indices,
    std::vector<T>& output)
{
    if (values.empty()) {
        output.clear();
        return;
    }

    // Just copy if no remapping needed.
    if (indices.empty()) {
        output.assign(values.start, values.start + values.size);
        return;
    }

    const auto count = indices.size;
    output.reserve(count);
    output.clear();
    for (size_t i = 0; i < count; ++i) {
        output.push_back(values[indices[i]]);
    }
}

template <typename T>
void getParamSample(
    const Alembic::AbcGeom::ITypedGeomParam<T>& param, const index_t sample_index,
    typename Alembic::AbcGeom::ITypedGeomParam<T>::sample_type::samp_ptr_type& out_sample,
    UInt32ArraySamplePtr* out_indices = nullptr)
{
    if (!param.valid() || sample_index == INDEX_UNKNOWN) {
        out_sample.reset();
        if (out_indices)
            out_indices->reset();
        return;
    }

    const auto scope = param.getScope();
    if (scope == kVaryingScope || scope == kVertexScope) {
        out_sample = param.getExpandedValue(sample_index).getVals();
        if (out_indices)
            out_indices->reset();
    } else if (scope == kFacevaryingScope) {
        auto samp = param.getIndexedValue(sample_index);
        out_sample = samp.getVals();
        if (out_indices)
            *out_indices = samp.getIndices();
    } else {
        out_sample.reset();
        if (out_indices)
            out_indices->reset();
    }
}

struct MeshEdge {
    int32_t index1;
    int32_t index2;
    MeshEdge(int32_t index1_, int32_t index2_)
        : index1(std::min(index1_, index2_))
        , index2(std::max(index1_, index2_))
    {}
    bool operator==(const MeshEdge& rhs) const
    {
        return index1 == rhs.index1 && index2 == rhs.index2;
    }
};

struct MeshEdgeHasher {
    size_t operator()(const MeshEdge& mesh_edge) const
    {
        size_t res = std::hash<int32_t>()(mesh_edge.index1);
        boost::hash_combine(res, mesh_edge.index2);
        return res;
    }
};

void computeTriangleAndWireframeIndices(
    const Int32ArraySample& face_indices,
    const Int32ArraySample& face_counts,
    const IndexMapping& index_mapping,
    std::vector<uint32_t>& out_triangle_indices,
    std::vector<uint32_t>& out_wireframe_indices)
{
    out_triangle_indices.clear();

    const auto indices =
        index_mapping.position_indices.empty()
        ? Span<const int32_t>(face_indices)
        : Span<const int32_t>(index_mapping.vertex_index_from_face_index);

    std::unordered_set<MeshEdge, MeshEdgeHasher> mesh_edges;
    size_t base_index = 0;
    for (size_t i = 0; i < face_counts.size(); ++i) {
        const int32_t face_vertex_count = face_counts[i];

        // Triangulate this face and push the vertices to the triangle_buffer.
        for (int32_t j = 2; j < face_vertex_count; ++j) {
            out_triangle_indices.push_back(indices[base_index + 0]);
            out_triangle_indices.push_back(indices[base_index + j - 1]);
            out_triangle_indices.push_back(indices[base_index + j]);
        }

        // Insert face edges to the mesh_edges hashset.
        for (int32_t j = 0; j < face_vertex_count; ++j) {
            const auto index1 = indices[base_index + j];
            const auto index2 = indices[base_index + (j + 1) % face_vertex_count];
            mesh_edges.insert({ index1, index2 });
        }

        base_index += face_vertex_count;
    }

    // Push edge indices to the wireframe_buffer.
    out_wireframe_indices.clear();
    for (const auto& edge : mesh_edges) {
        out_wireframe_indices.push_back(edge.index1);
        out_wireframe_indices.push_back(edge.index2);
    }
};

template <typename PropertyOrParam>
std::pair<index_t, chrono_t> floorIndexAndTime(const PropertyOrParam& obj, chrono_t time)
{
    return obj.getTimeSampling()->getFloorIndex(time, obj.getNumSamples());
}

template <typename PropertyOrParam>
std::pair<index_t, chrono_t> ceilIndexAndTime(const PropertyOrParam& obj, chrono_t time)
{
    return obj.getTimeSampling()->getCeilIndex(time, obj.getNumSamples());
}

} // unnamed namespace

void DrawableBufferSampler::sampleBuffers(chrono_t time, DrawableCacheHandles& out_handles, Box3f& out_bbox)
{
    // Get floor indices.
    // sidx: sample index.

    const auto homogeneous_topology = m_topology_variance != Alembic::AbcGeom::kHeterogenousTopology;
    const auto indices_sidx = homogeneous_topology ? 0 : floorIndexAndTime(m_face_indices_property, time).first;

    const bool constant_normals = !m_normals_param.valid() || m_normals_param.isConstant();
    const bool constant_geo = m_positions_property.isConstant() && constant_normals;
    index_t geo_floor_sidx = 0;
    chrono_t geo_floor_time = 0;
    if (!constant_geo)
        std::tie(geo_floor_sidx, geo_floor_time) = floorIndexAndTime(m_positions_property, time);

    const bool constant_uvs = !m_uvs_param.valid() || m_uvs_param.isConstant();
    index_t uvs_floor_sidx = 0;
    chrono_t uvs_floor_time = 0;
    if (!constant_uvs)
        std::tie(uvs_floor_sidx, uvs_floor_time) = floorIndexAndTime(m_uvs_param, time);

    std::shared_ptr<AlembicHolder::IndexBufferSample> index_sample_ptr;
    if (m_face_counts_property.valid() && m_face_indices_property.valid()) {
        // Index properties are valid; get index buffer.
        index_sample_ptr = m_index_buffer_cache.get(indices_sidx);
        if (!index_sample_ptr) {
            // Not in cache; compute the indices and cache them.

            Int32ArraySamplePtr face_counts_sample = m_face_counts_property.getValue(indices_sidx);
            Int32ArraySamplePtr face_indices_sample = m_face_indices_property.getValue(indices_sidx);
            if (!face_indices_sample || !face_counts_sample) {
                return;
            }

            // Allocate IndexBufferSample. This will go into the index buffer cache.
            auto index_buffer_sample = std::unique_ptr<IndexBufferSample>(new IndexBufferSample());

            // Remap indices if needed.
            const auto index_remapping_needed = m_normals_are_indexed || m_uvs_are_indexed;
            auto& index_mapping = index_buffer_sample->index_mapping;
            if (index_remapping_needed) {
                N3fArraySamplePtr normals_sample;
                UInt32ArraySamplePtr normals_indices;
                V2fArraySamplePtr uvs_sample;
                UInt32ArraySamplePtr uvs_indices;
                getParamSample(m_normals_param, geo_floor_sidx, normals_sample, &normals_indices);
                getParamSample(m_uvs_param, uvs_floor_sidx, uvs_sample, &uvs_indices);
                computeIndexMapping(*face_indices_sample, normals_indices, uvs_indices, index_mapping);

            } else {
                // Clear index_mapping to indicate that the indices are not remapped.
                index_mapping.position_indices.clear();
                index_mapping.normal_indices.clear();
                index_mapping.uv_indices.clear();
            }

            // Fill index buffers.
            computeTriangleAndWireframeIndices(
                *face_indices_sample, *face_counts_sample, index_buffer_sample->index_mapping,
                index_buffer_sample->triangle_buffer,
                index_buffer_sample->wireframe_buffer);

            // Put the index sample into the cache.
            index_sample_ptr = m_index_buffer_cache.put(indices_sidx, index_buffer_sample.release());
        }

    } else if (m_type == GeometryType::LINES) {
        index_sample_ptr = m_index_buffer_cache.get(indices_sidx);
        if (!index_sample_ptr) {
            auto positions_sample = m_positions_property.getValue(geo_floor_sidx);
            if (!positions_sample)
                return;
            const auto position_count = positions_sample->size();
            auto index_buffer_sample = std::unique_ptr<IndexBufferSample>(new IndexBufferSample());
            index_buffer_sample->wireframe_buffer.resize((position_count - 1) * 2);
            for (unsigned int i = 0; i < position_count - 1; ++i) {
                index_buffer_sample->wireframe_buffer[2 * i + 0] = i;
                index_buffer_sample->wireframe_buffer[2 * i + 1] = i + 1;
            }
            index_sample_ptr = m_index_buffer_cache.put(indices_sidx, index_buffer_sample.release());
        }
    }

    // Helper function for obtaining a geometry sample at index `sample_index`.
    // Get from cache if found, otherwise create a new sample and cache it.
    const auto getGeoSample = [this, &index_sample_ptr](index_t sample_index) -> GeometrySamplePtr {
        auto geo_sample_ptr = m_geometry_sample_cache.get(sample_index);
        if (geo_sample_ptr)
            return geo_sample_ptr;

        // Not in cache; create geometry sample.

        auto geo_sample = std::unique_ptr<GeometrySample>(new GeometrySample());

        // Positions.

        // Read position sample.
        auto positions_sample = m_positions_property.getValue(sample_index);
        if (!positions_sample) {
            return nullptr;
        }

        const auto& position_indices = index_sample_ptr
            ? Span<const int32_t>(index_sample_ptr->index_mapping.position_indices)
            : Span<const int32_t>();
        fillBuffer(Span<const V3f>(positions_sample), position_indices, geo_sample->positions);

        // BBox from positions.
        geo_sample->bbox.makeEmpty();
        for (size_t i = 0; i < positions_sample->size(); ++i) {
            geo_sample->bbox.extendBy(positions_sample->get()[i]);
        }

        // Normals.

        // Read normal sample. It's possible that they are already read by the
        // index remapping logic.
        N3fArraySamplePtr normals_sample;
        if (m_normals_param.valid()) {
            getParamSample(m_normals_param, sample_index, normals_sample);
        }

        const auto& normal_indices = index_sample_ptr
            ? Span<const uint32_t>(index_sample_ptr->index_mapping.normal_indices)
            : Span<const uint32_t>();
        if (normals_sample) {
            fillBuffer(Span<const V3f>(normals_sample), normal_indices, geo_sample->normals);
        } else if (m_type == GeometryType::TRIANGLES && index_sample_ptr) {
            generateNormals(index_sample_ptr->triangle_buffer, geo_sample->positions, geo_sample->normals);
        }

        return m_geometry_sample_cache.put(sample_index, geo_sample.release());
    };

    // Helper function for obtaining a texcoord sample at index `sample_index`.
    // Get from cache if found, otherwise create a new sample and cache it.
    const auto getUVsSample = [this, &index_sample_ptr](index_t sample_index) -> TexCoordsSamplePtr {
        if (!m_uvs_param.valid())
            return nullptr;

        // Return cached if in cache.
        auto uvs_sample_ptr = m_texcoords_sample_cache.get(sample_index);
        if (uvs_sample_ptr)
            return uvs_sample_ptr;

        // Not in cache; create sample.

        // Read uv sample. It's possible that they are already read by the
        // index remapping logic.
        V2fArraySamplePtr uvs_sample;
        getParamSample(m_uvs_param, sample_index, uvs_sample);

        auto texcoords_sample = std::unique_ptr<TexCoordsSample>(new TexCoordsSample());

        const auto& uv_indices = index_sample_ptr
            ? Span<const uint32_t>(index_sample_ptr->index_mapping.uv_indices)
            : Span<const uint32_t>();
        fillBuffer(Span<const V2f>(uvs_sample), uv_indices, texcoords_sample->uvs);

        return m_texcoords_sample_cache.put(sample_index, texcoords_sample.release());
    };

    // Determine if interpolation is possible and needed.
    index_t geo_ceil_sidx;
    chrono_t geo_ceil_time;
    std::tie(geo_ceil_sidx, geo_ceil_time) = ceilIndexAndTime(m_positions_property, time);
    const bool interpolate_geo = homogeneous_topology && !constant_geo &&
        geo_floor_sidx != geo_ceil_sidx && geo_floor_time != time;

    index_t uvs_ceil_sidx = 0;
    chrono_t uvs_ceil_time = 0;
    if (m_uvs_param.valid()) {
        std::tie(uvs_ceil_sidx, uvs_ceil_time) = ceilIndexAndTime(m_uvs_param, time);
    }
    const bool interpolate_uvs = homogeneous_topology && !constant_uvs &&
        uvs_floor_sidx != uvs_ceil_sidx && uvs_floor_time != time;

    // Get pointers first and assign them to the output after that to maximize
    // cache usage (when the ref count drops to zero the entry gets dedelet from
    // the cache).
    InterpolationData<GeometrySamplePtr> out_geometry;
    InterpolationData<TexCoordsSamplePtr> out_texcoords;
    out_geometry.endpoints[0] = getGeoSample(geo_floor_sidx);
    if (interpolate_geo) {
        out_geometry.endpoints[1] = getGeoSample(geo_ceil_sidx);
        out_geometry.alpha = float((time - geo_floor_time) / (geo_ceil_time - geo_floor_time));
    }
    if (m_uvs_param.valid()) {
        out_texcoords.endpoints[0] = getUVsSample(uvs_floor_sidx);
        if (interpolate_uvs) {
            out_texcoords.endpoints[1] = getUVsSample(uvs_ceil_sidx);
            out_texcoords.alpha = float((time - uvs_floor_time) / (uvs_ceil_time - uvs_floor_time));
        }
    }

    out_handles.indices = index_sample_ptr;
    out_handles.geometry = out_geometry;
    out_handles.texcoords = out_texcoords;

    out_bbox.makeEmpty();
    for (auto& geo : out_handles.geometry.endpoints)
        if (geo)
            out_bbox.extendBy(geo->bbox);
}

DrawableBufferSampler::DrawableBufferSampler(const Hierarchy::Node& node)
    : m_topology_variance(Alembic::AbcGeom::MeshTopologyVariance::kConstantTopology)
    , m_normals_are_indexed(false)
    , m_uvs_are_indexed(false)
{
    if (node.type == Hierarchy::NodeType::POLYMESH) {
        m_type = GeometryType::TRIANGLES;
        auto schema = Alembic::AbcGeom::IPolyMesh(node.source_object).getSchema();
        m_face_counts_property = schema.getFaceCountsProperty();
        m_face_indices_property = schema.getFaceIndicesProperty();
        m_positions_property = schema.getPositionsProperty();
        m_normals_param = schema.getNormalsParam();
        m_uvs_param = schema.getUVsParam();
        m_topology_variance = schema.getTopologyVariance();
        m_normals_are_indexed = m_normals_param.valid() && m_normals_param.getScope() == kFacevaryingScope;
        m_uvs_are_indexed = m_uvs_param.valid() && m_uvs_param.getScope() == kFacevaryingScope;

    } else if (node.type == Hierarchy::NodeType::CURVES) {
        m_type = GeometryType::LINES;
        auto schema = Alembic::AbcGeom::ICurves(node.source_object).getSchema();
        m_positions_property = schema.getPositionsProperty();

    } else if (node.type == Hierarchy::NodeType::POINTS) {
        m_type = GeometryType::POINTS;
        auto schema = Alembic::AbcGeom::IPoints(node.source_object).getSchema();
        m_positions_property = schema.getPositionsProperty();

    } else {
        abort();
    }
}

} // namespace AlembicHolder
