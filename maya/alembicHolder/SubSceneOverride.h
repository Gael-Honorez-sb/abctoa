#pragma once

#include "AlembicScene.h"
#include "nozAlembicHolderNode.h"

#include <maya/MPxSubSceneOverride.h>
#include <maya/MSelectionContext.h>
#include <maya/MMessage.h>

namespace AlembicHolder {

struct VP2IndexBufferSample {
    MHWRender::MIndexBuffer triangle_buffer;
    MHWRender::MIndexBuffer wireframe_buffer;
    VP2IndexBufferSample()
        : triangle_buffer(MHWRender::MGeometry::kUnsignedInt32)
        , wireframe_buffer(MHWRender::MGeometry::kUnsignedInt32)
    {}
};
typedef Cache<index_t, VP2IndexBufferSample> VP2IndexBufferSampleCache;
typedef VP2IndexBufferSampleCache::ValuePtr VP2IndexBufferSamplePtr;

struct VP2GeometrySample {
    MHWRender::MVertexBuffer positions;
    MHWRender::MVertexBuffer normals;
    MHWRender::MVertexBuffer tangents;
    MHWRender::MVertexBuffer bitangents;
    typedef uint8_t Flags;
    enum FlagConstants : Flags { POSITIONS_ONLY = 0, HAS_NORMALS = 1, HAS_TANGENT_BASIS = 2 };
    Flags flags;
    VP2GeometrySample()
        : positions(MHWRender::MVertexBufferDescriptor("", MHWRender::MGeometry::kPosition, MHWRender::MGeometry::kFloat, 3))
        , normals(MHWRender::MVertexBufferDescriptor("", MHWRender::MGeometry::kNormal, MHWRender::MGeometry::kFloat, 3))
        , tangents(MHWRender::MVertexBufferDescriptor("", MHWRender::MGeometry::kTangent, MHWRender::MGeometry::kFloat, 3))
        , bitangents(MHWRender::MVertexBufferDescriptor("", MHWRender::MGeometry::kBitangent, MHWRender::MGeometry::kFloat, 3))
        , flags(POSITIONS_ONLY)
    {}
};

// VP2 caches are indexed with chrono_t so interpolated buffers are cached too.
typedef Cache<chrono_t, VP2GeometrySample> VP2GeometrySampleCache;
typedef VP2GeometrySampleCache::ValuePtr VP2GeometrySamplePtr;

// On the GPU, only interpolated samples are stored (not the interpolation
// endpoints). This struct stores handles to the buffer cache entries holding
// the interpolated buffers.
struct VP2BufferHandles {
    VP2IndexBufferSamplePtr indices;
    VP2GeometrySamplePtr geometry;
};
typedef std::vector<VP2BufferHandles> VP2BufferHandlesVector;

class VP2DrawableBufferCache {
public:
    VP2DrawableBufferCache(const Hierarchy::Node& node);
    VP2BufferHandles sampleBuffers(chrono_t time, const DrawableCacheHandles& cache_handles);
private:
    Alembic::AbcCoreAbstract::TimeSamplingPtr m_time_sampling;
    size_t m_num_samples;
    bool m_has_indices;
    bool m_constant_indices;
    bool m_constant_geometry;

    VP2IndexBufferSampleCache m_vp2_index_buffer_cache;
    VP2GeometrySampleCache m_vp2_geometry_sample_cache;
};

class VP2Scene {
public:
    VP2Scene(const HierarchyNodeCategories& hierarchy_node_categories);
    void updateVP2Buffers(chrono_t time, const DrawableSampleVector& in_samples, VP2BufferHandlesVector& out_buffers);
private:
    std::vector<VP2DrawableBufferCache> m_vp2_caches;
    tbb::mutex m_mutex;
};

typedef std::shared_ptr<VP2Scene> VP2ScenePtr;

class VP2SceneCache {
public:
    static VP2SceneCache& instance();
    VP2ScenePtr getScene(const AlembicSceneKey& key, const AlembicScenePtr& scene);
private:
    Cache<AlembicSceneKey, VP2Scene, AlembicSceneKeyHasher> m_scene_cache;
    tbb::mutex m_mutex;
};

typedef std::shared_ptr<MHWRender::MShaderInstance> ShaderPtr;


class SubSceneOverride : public MHWRender::MPxSubSceneOverride {
public:
    // These methods must be called in initializePlugin and uninitializePlugin
    // respectively.
    static void initializeShaderTemplates();
    static void releaseShaderTemplates();

    static MHWRender::MPxSubSceneOverride* creator(const MObject& object);
    SubSceneOverride(const MObject& object);
    virtual ~SubSceneOverride();

    MHWRender::DrawAPI supportedDrawAPIs() const override;

    bool requiresUpdate(
        const MHWRender::MSubSceneContainer& container,
        const MHWRender::MFrameContext& frameContext) const override;

    void update(
        MHWRender::MSubSceneContainer& container,
        const MHWRender::MFrameContext& frameContext) override;

    void dirtyWorldMatrix();

    bool getSelectionPath(const MHWRender::MRenderItem &renderItem, MDagPath &dagPath) const override;

private:
    typedef nozAlembicHolder ShapeNode;
    const MObject m_object;
    ShapeNode* m_shape_node;

    bool m_update_world_matrix_required;
    MCallbackId m_world_matrix_changed_callback;

private:
    // AlembicHolder data affecting viewport display.
    AlembicSceneKey m_scene_key;
    std::string m_selection_key;
    chrono_t m_sample_time;
    bool m_bbox_extended_mode;
    bool m_is_selected;
    bool m_is_visible;
    std::string m_shader_assignments_json;
    M3dView::DisplayStyle m_display_style;

private:
    // The m_vp2_cache_handle holds a handle to the caches belonging to the
    // current scene (selected by m_scene_key). The caches contain interpolated
    // VP2 buffers, the handles of which are stored in m_buffers.
    // m_buffers have to be destroyed _before_ the scene caches, so m_buffers
    // have to be declared _after_ m_vp2_cache_handle.
    VP2ScenePtr m_vp2_cache_handle;
    std::vector<VP2BufferHandles> m_buffers;

private:
    // Render items.

    struct Points {
        DrawableID drawable_id;
        MHWRender::MRenderItem* item;
        ShaderPtr shader;
        Points(DrawableID id) : drawable_id(id), item(nullptr) {}
    };
    std::vector<Points> m_points;

    struct Lines {
        DrawableID drawable_id;
        MHWRender::MRenderItem* wireframe_item;
        MHWRender::MRenderItem* shaded_item;
        ShaderPtr shader;
        Lines(DrawableID id) : drawable_id(id), wireframe_item(nullptr), shaded_item(nullptr) {}
    };
    std::vector<Lines> m_lines;

    struct TriangleMesh {
        DrawableID drawable_id;
        MHWRender::MRenderItem* wireframe_item;
        MHWRender::MRenderItem* shaded_item;
        ShaderPtr shaded_shader;

        TriangleMesh(DrawableID id)
            : drawable_id(id), wireframe_item(nullptr), shaded_item(nullptr)
        {}
    };
    std::vector<TriangleMesh> m_meshes;

    struct BBoxWireframe {
        static const char* RENDER_ITEM_NAME;
        static const Imath::Box3f DEFAULT_BBOX;
        static const uint32_t BOX_INDICES[];
        MHWRender::MRenderItem* render_item;
        MHWRender::MVertexBuffer position_buffer;
        MHWRender::MIndexBuffer index_buffer;
        BBoxWireframe();
        void initRenderItem();
        void updateBBox(MHWRender::MPxSubSceneOverride& parent, const Box3f& bbox);
        operator bool() const { return render_item != nullptr; }
    };
    BBoxWireframe m_bbox_wireframe;
    MColor m_wire_color;
    ShaderPtr m_wire_shader;
};

} // namespace AlembicHolder
