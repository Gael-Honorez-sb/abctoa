#include "subSceneOverride.h"

#include <maya/MDagMessage.h>
#include <maya/MDGMessage.h>
#include <maya/MModelMessage.h>
#include <maya/MNodeMessage.h>
#include <maya/MSceneMessage.h>
#include <maya/MEventMessage.h>
#include <maya/MTime.h>

#include <maya/MHWGeometryUtilities.h>
#include <maya/MDrawContext.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MSelectionList.h>
#include <maya/MShaderManager.h>
#include <maya/MFragmentManager.h>
#include <maya/MUserData.h>

#include <tbb/mutex.h>

#include <boost/functional/hash.hpp>

#include <chrono>
#include <sstream>
#include <unordered_set>
#include <cstring>


#define kPluginId "alembicHolder"

using namespace MHWRender;

namespace AlembicHolder {

namespace {
    MFragmentManager* getFragmentManager()
    {
        MRenderer* renderer = MRenderer::theRenderer();
        if (!renderer)
            return nullptr;
        return renderer->getFragmentManager();
    }

    const MShaderManager* getShaderManager()
    {
        MRenderer* renderer = MRenderer::theRenderer();
        if (!renderer)
            return nullptr;
        return renderer->getShaderManager();
    }

    struct ShaderInstanceDeleter {
        void operator()(MShaderInstance* ptr) const
        {
            auto shader_manager = getShaderManager();
            if (!shader_manager)
                return;
            shader_manager->releaseShader(ptr);
        }
    };

    struct MColorHasher {
        size_t operator()(const MColor& color) const
        {
            size_t res = std::hash<float>()(color.r);
            boost::hash_combine(res, color.g);
            boost::hash_combine(res, color.b);
            boost::hash_combine(res, color.a);
            return res;
        }
    };
    typedef Cache<MColor, MShaderInstance, MColorHasher, std::equal_to<MColor>, ShaderInstanceDeleter> WireShaderCache;
    WireShaderCache s_wire_shader_cache;

    const char* s_texture_fragment_name = "fileTexturePluginFragment";
    const char* s_texture_fragment_xml = R"xml(
<fragment uiName="fileTexturePluginFragment" name="fileTexturePluginFragment" type="plumbing" class="ShadeFragment" version="1.0" feature_level="0">
    <description><![CDATA[ Simple file texture fragment]]></description>
    <properties>
        <float2 name="uvCoord" semantic="mayaUvCoordSemantic" flags="varyingInputParam" />
        <texture2 name="map" />
        <sampler name="textureSampler" />
    </properties>
    <values/>
    <outputs>
        <float4 name="output" />
    </outputs>
    <implementation>
    <implementation render="OGSRenderer" language="Cg" lang_version="2.100000">
        <function_name val="fileTexturePluginFragment" />
        <source>
            <![CDATA[
float4 fileTexturePluginFragment(float2 uv, texture2D map, sampler2D mapSampler)
{
    uv -= floor(uv);
    uv.y = 1.0f - uv.y;
    float4 color = tex2D(mapSampler, uv);
    return color.rgba;
}
            ]]>
        </source>
    </implementation>
    <implementation render="OGSRenderer" language="HLSL" lang_version="11.000000">
        <function_name val="fileTexturePluginFragment" />
        <source>
            <![CDATA[
float4 fileTexturePluginFragment(float2 uv, Texture2D map, sampler mapSampler)
{
    uv -= floor(uv);
    uv.y = 1.0f - uv.y;
    float4 color = map.Sample(mapSampler, uv);
    return color.rgba;
}
            ]]>
        </source>
    </implementation>
    <implementation render="OGSRenderer" language="GLSL" lang_version="3.000000">
        <function_name val="fileTexturePluginFragment" />
        <source>
            <![CDATA[
vec4 fileTexturePluginFragment(vec2 uv, sampler2D mapSampler)
{
    uv -= floor(uv);
    uv.y = 1.0f - uv.y;
    vec4 color = texture(mapSampler, uv);
    return color.rgba;
}
            ]]>
        </source>
    </implementation>
    <implementation render="OGSRenderer" language="HLSL" lang_version="10.000000">
        <function_name val="fileTexturePluginFragment" />
        <source>
            <![CDATA[
float4 fileTexturePluginFragment(float2 uv, Texture2D map, sampler mapSampler)
{
    uv -= floor(uv);
    uv.y = 1.0f - uv.y;
    float4 color = map.Sample(mapSampler, uv);
    return color.rgba;
}
            ]]>
        </source>
    </implementation>
    </implementation>
</fragment>
)xml";

    MShaderInstance* s_point_shader_template = nullptr;
    MShaderInstance* s_wire_shader_template = nullptr;
    MShaderInstance* s_flat_shader_template = nullptr;
    MShaderInstance* s_shaded_shader_template = nullptr;
    MShaderInstance* s_textured_shader_template = nullptr;

    ShaderPtr getWireShader(const MColor& wire_color)
    {
        auto ptr = s_wire_shader_cache.get(wire_color);
        if (ptr)
            return ptr;

        // Not in cache. Clone the template, set color and insert into cache.
        assert(s_wire_shader_template);
        auto shader = s_wire_shader_template->clone();
        assert(shader);
        const float color[4] = {wire_color.r, wire_color.g, wire_color.b, 1.0f};
        CHECK_MSTATUS(shader->setParameter("solidColor", color));
        return s_wire_shader_cache.put(wire_color, std::move(shader));
    }

    ShaderPtr newPointShader()
    {
        assert(s_point_shader_template);
        auto shader = s_point_shader_template->clone();
        return ShaderPtr(shader, ShaderInstanceDeleter());
    }

    ShaderPtr newFlatShader()
    {
        assert(s_flat_shader_template);
        auto shader = s_flat_shader_template->clone();
        return ShaderPtr(shader, ShaderInstanceDeleter());
    }

    ShaderPtr newShadedShader()
    {
        assert(s_shaded_shader_template);
        auto shader = s_shaded_shader_template->clone();
        return ShaderPtr(shader, ShaderInstanceDeleter());
    }

    void setPointShaderSolidColor(MShaderInstance* shader, const C3f& solid_color)
    {
        assert(shader);
        const float color[4] = {solid_color.x, solid_color.y, solid_color.z, 1.0f};
        CHECK_MSTATUS(shader->setParameter("solidColor", color));
    }

    void setFlatShaderColor(MShaderInstance* shader, const C3f& shader_color)
    {
        assert(shader);
        const float color[4] = {shader_color.x, shader_color.y, shader_color.z, 1.0f};
        CHECK_MSTATUS(shader->setParameter("solidColor", color));
    }


    void setShadedShaderDiffuseColor(MShaderInstance* shader, const C3f& diffuse_color)
    {
        assert(shader);
        const float color[4] = {diffuse_color.x, diffuse_color.y, diffuse_color.z, 1.0f};
        CHECK_MSTATUS(shader->setParameter("diffuseColor", color));
    }

    ShaderPtr newTexturedShader()
    {
        assert(s_textured_shader_template);
        auto shader = s_textured_shader_template->clone();
        return ShaderPtr(shader, ShaderInstanceDeleter());
    }

    void setTexturedShaderTexture(MShaderInstance* shader, MTexture* texture)
    {
        assert(shader);
        MTextureAssignment texture_assignment;
        texture_assignment.texture = texture;
        CHECK_MSTATUS(shader->setParameter("map", texture_assignment));
    }

    const auto DEFAULT_SHADER_COLOR = C3f(0.7f, 0.7f, 0.7f);

} // unnamed namespace


void SubSceneOverride::initializeShaderTemplates()
{
    // Initialize wire and shaded templates.

    auto shader_manager = getShaderManager();
    if (!shader_manager)
        return;

    s_point_shader_template = shader_manager->getStockShader(MHWRender::MShaderManager::k3dFatPointShader);
    if (s_point_shader_template) {
        CHECK_MSTATUS(s_point_shader_template->setIsTransparent(false));
    }

    s_wire_shader_template = shader_manager->getFragmentShader("mayaDashLineShader", "", false);
    if (s_wire_shader_template) {
        CHECK_MSTATUS(s_wire_shader_template->setIsTransparent(false));
    }

    s_flat_shader_template = shader_manager->getStockShader(MHWRender::MShaderManager::k3dSolidShader);
    if (s_flat_shader_template) {
        CHECK_MSTATUS(s_flat_shader_template->setIsTransparent(false));
        setFlatShaderColor(s_flat_shader_template, DEFAULT_SHADER_COLOR);
    }

    s_shaded_shader_template = shader_manager->getStockShader(MHWRender::MShaderManager::k3dBlinnShader);
    if (s_shaded_shader_template) {
        CHECK_MSTATUS(s_shaded_shader_template->setIsTransparent(false));
        CHECK_MSTATUS(s_shaded_shader_template->setParameter("specularCoeff", 0.8f));
        CHECK_MSTATUS(s_shaded_shader_template->setParameter("specularPower", 4.0f));
        setShadedShaderDiffuseColor(s_shaded_shader_template, DEFAULT_SHADER_COLOR);
    }

    // Initialize texturing fragment and textured shader template.

    // Textured template is cloned from the shaded template, so bail if the
    // latter is nullptr.
    if (!s_shaded_shader_template)
        return;

    auto fragment_manager = getFragmentManager();
    if (!fragment_manager)
        return;

    // Register texturing fragment.
    if (!fragment_manager->hasFragment(s_texture_fragment_name)) {
        const auto fragment_name = fragment_manager->addShadeFragmentFromBuffer(s_texture_fragment_xml, false);
        if (fragment_name != s_texture_fragment_name) {
            MGlobal::displayError("[alembicHolder] Failed to register texturing fragment. Textured display is disabled.");
            return;
        }
    }

    // Clone the shaded shader.
    s_textured_shader_template = s_shaded_shader_template->clone();
    if (!s_textured_shader_template)
        return;

    // Connect the texturing fragment to the cloned shader.
    CHECK_MSTATUS(s_textured_shader_template->addInputFragment(s_texture_fragment_name, MString("output"), MString("diffuseColor")));

    // Set up anisotropic sampler.
    MStatus status;
    MSamplerStateDesc desc;
    desc.setDefaults();
    desc.addressU = MSamplerState::kTexWrap;
    desc.addressV = MSamplerState::kTexWrap;
    desc.filter = MHWRender::MSamplerState::kAnisotropic;
    desc.maxAnisotropy = 8;
    auto sampler = MStateManager::acquireSamplerState(desc, &status);
    CHECK_MSTATUS(status);
    if (!sampler)
        return;
    CHECK_MSTATUS(s_textured_shader_template->setParameter("textureSampler", *sampler));
    MStateManager::releaseSamplerState(sampler);
}

void SubSceneOverride::releaseShaderTemplates()
{
    auto shader_manager = getShaderManager();
    if (!shader_manager)
        return;

    auto fragment_manager = getFragmentManager();
    if (!fragment_manager)
        return;

    shader_manager->releaseShader(s_point_shader_template);
    shader_manager->releaseShader(s_wire_shader_template);
    shader_manager->releaseShader(s_flat_shader_template);
    shader_manager->releaseShader(s_shaded_shader_template);
    shader_manager->releaseShader(s_textured_shader_template);
    fragment_manager->removeFragment(s_texture_fragment_name);
}

namespace {

    bool isPathSelected(MDagPath path)
    {
        MSelectionList selectedList;
        MGlobal::getActiveSelectionList(selectedList);
        do {
            if (selectedList.hasItem(path)) {
                return true;
            }
        } while (path.pop());
        return false;
    }

    const auto FLT_INF = std::numeric_limits<float>::infinity();

} // unnamed namespace

MPxSubSceneOverride* SubSceneOverride::creator(const MObject& object)
{
    return new SubSceneOverride(object);
}

static void WorldMatrixChangedCallback(
    MObject& transformNode,
    MDagMessage::MatrixModifiedFlags& modified,
    void* clientData)
{
    assert(clientData);
    static_cast<SubSceneOverride*>(clientData)->dirtyWorldMatrix();
}

void SubSceneOverride::dirtyWorldMatrix()
{
    m_update_world_matrix_required = true;
}

SubSceneOverride::SubSceneOverride(const MObject& object)
    : MPxSubSceneOverride(object)
    , m_object(object)
    , m_shape_node(nullptr)
    , m_update_world_matrix_required(true)
    , m_sample_time(-std::numeric_limits<chrono_t>::infinity())
    , m_bbox_extended_mode(false)
    , m_is_selected(false)
    , m_is_visible(false)
    , m_wire_color(MColor(FLT_INF, FLT_INF, FLT_INF, FLT_INF))
{
    // Extract the ShapeNode pointer.
    MFnDagNode dagNode(object);
    m_shape_node = (ShapeNode*)dagNode.userNode();
    assert(m_shape_node);

    // Register callbacks
    MDagPath dag_path = MDagPath::getAPathTo(m_object);
    m_world_matrix_changed_callback = MDagMessage::addWorldMatrixModifiedCallback(
        dag_path, WorldMatrixChangedCallback, this);
}

SubSceneOverride::~SubSceneOverride()
{
    MMessage::removeCallback(m_world_matrix_changed_callback);
}

DrawAPI SubSceneOverride::supportedDrawAPIs() const
{
    return kAllDevices;
}

bool SubSceneOverride::requiresUpdate(
    const MSubSceneContainer& container,
    const MFrameContext& frameContext) const
{
    assert(m_shape_node);
    if (!m_shape_node) return false;

    MDagPath dag_path = MDagPath::getAPathTo(m_object);
    if (!dag_path.isVisible() && !m_is_visible) {
        // If invisible, and the subSceneOverride has already been updated to
        // hide the render items (!m_is_visible), then there's nothing to do.
        return false;
    }

    const auto& m3dview = M3dView::active3dView();

    return
        m_meshes.empty() ||
        m_update_world_matrix_required ||
        m_scene_key != m_shape_node->getSceneKey() ||
        m_sample_time != m_shape_node->getTime() ||
        m_bbox_extended_mode != m_shape_node->isBBExtendedMode() ||
        m_selection_key != m_shape_node->getSelectionKey() ||
        m_shader_assignments_json != m_shape_node->getShaderAssignmentsJson() ||
        m_is_selected != isPathSelected(dag_path) ||
        m_is_visible != dag_path.isVisible() ||
        m_wire_color != MGeometryUtilities::wireframeColor(dag_path) ||
        m_texture_mode != m3dview.textureMode() ||
        m_display_style != m3dview.displayStyle();
}

void SubSceneOverride::update(
    MSubSceneContainer& container,
    const MFrameContext& frameContext)
{
    assert(m_shape_node);
    if (!m_shape_node)
        return;

    const auto& m3dview = M3dView::active3dView();
    MDagPath dag_path = MDagPath::getAPathTo(m_object);
    const auto node_world_matrix = dag_path.inclusiveMatrix();

    // Check what has changed.
    const bool uninitialized = m_meshes.empty();
    const bool scene_updated = updateValue(m_scene_key, m_shape_node->getSceneKey());
    const bool time_updated = updateValue(m_sample_time, m_shape_node->getTime());
    const bool bboxmode_updated = updateValue(m_bbox_extended_mode, m_shape_node->isBBExtendedMode());
    const bool selection_key_updated = updateValue(m_selection_key, m_shape_node->getSelectionKey());
    const bool color_overrides_updated = updateValue(m_shader_assignments_json, m_shape_node->getShaderAssignmentsJson());
    const bool node_selection_updated = updateValue(m_is_selected, isPathSelected(dag_path));
    const bool node_visibility_updated = updateValue(m_is_visible, dag_path.isVisible());
    const bool wire_color_updated = updateValue(m_wire_color, MGeometryUtilities::wireframeColor(dag_path));
    const bool texture_mode_changed = updateValue(m_texture_mode, m3dview.textureMode());
    const bool display_style_changed = updateValue(m_display_style, m3dview.displayStyle());

    const bool sample_updated = uninitialized || scene_updated || time_updated;

    const auto& scene = m_shape_node->getScene();
    const auto& scene_sample = m_shape_node->getSample();

    // Get scene data from the shape node.
    if (scene_updated) {
        // First release the buffers, because they contain smart pointerts
        // pointing into VP2 scene caches which should not survive longer than
        // the scene itself. But the scene gets destroyed if our handle is
        // the last reference to it.
        m_buffers.clear();
        m_vp2_cache_handle = VP2SceneCache::instance().getScene(m_scene_key, scene);
    }

    // Wireframe shader. Needs to be updated before updating the render items
    // since the wireframe render items need a default shader.
    if (!m_wire_shader || wire_color_updated) {
        m_wire_shader = getWireShader(m_wire_color);
    }

    // Re-populate the render item container if needed.
    const bool update_render_items = uninitialized || scene_updated;
    if (update_render_items) {

        // Helper function for creating render items.
        const auto createRenderItem = [&container](
            uint32_t drawable_id,
            const std::string& name_suffix,
            MRenderItem::RenderItemType type,
            MGeometry::Primitive primitive,
            int draw_mode)
        {
            std::stringstream ss;
            ss << "alembicHolder_drawable_" << drawable_id << "_" << name_suffix;
            auto res = MRenderItem::Create(ss.str().c_str(), type, primitive);
            res->setDrawMode(MGeometry::DrawMode(draw_mode));
            res->setSelectionMask(MSelectionMask(kPluginId));
            res->enable(true);
            container.add(res);
            return res;
        };

        container.clear();
        m_bbox_wireframe.initRenderItem();
        m_bbox_wireframe.render_item->setShader(m_wire_shader.get());
        container.add(m_bbox_wireframe.render_item);

        // Add a wireframe, shaded and textured render item for each drawable.
        // Set default shader for shaded and textured items here, since a valid
        // shader is needed for setGeometryForRenderItem, and we can be certain,
        // that each shaded and textured render item gets a unique clone of the
        // corresponding template shader.
        // The wired shaders are shared based on color, so they are created in
        // the shader update loop.
        const auto points_count = scene ? scene->nodeCategories().countByType(Hierarchy::NodeType::POINTS) : 0;
        m_points.clear();
        m_points.reserve(points_count);
        const auto lines_count = scene ? scene->nodeCategories().countByType(Hierarchy::NodeType::CURVES) : 0;
        m_lines.clear();
        m_lines.reserve(lines_count);
        const auto mesh_count = scene ? scene->nodeCategories().countByType(Hierarchy::NodeType::POLYMESH) : 0;
        m_meshes.clear();
        m_meshes.reserve(mesh_count);
        for (const auto& sample : scene_sample.drawable_samples) {

            if (sample.type == GeometryType::POINTS) {
                m_points.push_back({sample.drawable_id});
                auto& points = m_points.back();

                points.item = createRenderItem(sample.drawable_id, "points",
                    MRenderItem::MaterialSceneItem, MGeometry::kPoints,
                    MGeometry::kWireframe | MGeometry::kShaded | MGeometry::kTextured);
                points.item->setExcludedFromPostEffects(true);
                points.item->depthPriority(MRenderItem::sActiveWireDepthPriority);
                points.shader = newPointShader();
                points.item->setShader(points.shader.get());

            } else if (sample.type == GeometryType::LINES) {
                m_lines.push_back({sample.drawable_id});
                auto& lines = m_lines.back();

                // Wireframe item.
                lines.wireframe_item = createRenderItem(sample.drawable_id, "wireframe",
                    MRenderItem::DecorationItem, MGeometry::kLines,
                    MGeometry::kWireframe | MGeometry::kShaded | MGeometry::kTextured);
                lines.wireframe_item->depthPriority(MRenderItem::sActiveWireDepthPriority);

                // Shaded item.
                lines.shaded_item = createRenderItem(sample.drawable_id, "shaded",
                    MRenderItem::MaterialSceneItem, MGeometry::kLines,
                    MGeometry::kShaded | MGeometry::kTextured);
                lines.shaded_item->setExcludedFromPostEffects(false);
                lines.shader = newFlatShader();
                lines.shaded_item->setShader(lines.shader.get());

            } else if (sample.type == GeometryType::TRIANGLES) {
                m_meshes.push_back({sample.drawable_id});
                auto& mesh = m_meshes.back();

                // Wireframe item.
                auto& wireframe_item = mesh.wireframe_item;
                wireframe_item = createRenderItem(sample.drawable_id, "wireframe",
                    MRenderItem::DecorationItem, MGeometry::kLines,
                    MGeometry::kWireframe | MGeometry::kShaded | MGeometry::kTextured);
                wireframe_item->depthPriority(MRenderItem::sActiveWireDepthPriority);

                // When no texture is bound, show the shaded item in textured
                // mode instead of the textured item.
                // This is changed when the textured shaders are updated and a
                // texture is bound to the render item.

                // Shaded item.
                auto& shaded_item = mesh.shaded_item;
                shaded_item = createRenderItem(sample.drawable_id, "shaded",
                    MRenderItem::MaterialSceneItem, MGeometry::kTriangles, MGeometry::kShaded | MGeometry::kTextured);
                shaded_item->setExcludedFromPostEffects(false);
                mesh.shaded_shader = newShadedShader();
                shaded_item->setShader(mesh.shaded_shader.get());

                // Textured item.
                auto& textured_item = mesh.textured_item;
                textured_item = createRenderItem(sample.drawable_id, "textured",
                    MRenderItem::MaterialSceneItem, MGeometry::kTriangles, 0);
                textured_item->setExcludedFromPostEffects(false);
                mesh.textured_shader = newTexturedShader();
                textured_item->setShader(mesh.textured_shader.get());
                mesh.texture_path.clear();
            }
        }
    }

    // Check what aspects of the bbox need to be updated.
    const bool update_bbox_visibility = uninitialized || scene_updated || bboxmode_updated ||
        selection_key_updated || node_visibility_updated || node_selection_updated;
    const bool update_bbox_shader = uninitialized || wire_color_updated;
    const bool update_bbox_streams = sample_updated;
    const bool force_bbox = m_bbox_extended_mode && !m_is_selected;

    // Update the bounding box wireframe.
    if (m_bbox_wireframe) {
        // Visibility.
        if (update_bbox_visibility) {
            // Show the wireframe if the shape node is visible and ethier the
            // scene is invalid or bbox mode is on.
            const bool selection_mode = !m_selection_key.empty();
            m_bbox_wireframe.render_item->enable((!scene || force_bbox || selection_mode) && m_is_visible);
        }
        // Shader.
        if (update_bbox_shader) {
            m_bbox_wireframe.render_item->setShader(m_wire_shader.get());
        }
        // Position buffer.
        if (update_bbox_streams) {
            auto bbox = scene ? scene_sample.hierarchy_stat.bbox : BBoxWireframe::DEFAULT_BBOX;
            m_bbox_wireframe.updateBBox(*this, bbox);
        }
        // World matrix.
        if (m_update_world_matrix_required) {
            m_bbox_wireframe.render_item->setMatrix(&node_world_matrix);
        }
    }

    // Check what aspects of the renderables need to be updated.
    const bool update_visibility = sample_updated ||
        selection_key_updated || bboxmode_updated ||
        node_selection_updated || node_visibility_updated ||
        display_style_changed;
    const bool update_wire_shaders = uninitialized || scene_updated || wire_color_updated;
    const bool update_shaded_shaders = uninitialized || scene_updated || color_overrides_updated;
    const bool update_textured_shaders = (uninitialized || scene_updated || color_overrides_updated || texture_mode_changed) && m_texture_mode;
    const bool update_streams = sample_updated;
    const bool update_matrices = m_update_world_matrix_required || sample_updated;

    const bool node_is_visible = m_is_visible;
    const auto& selection_visibility = scene_sample.selection_visibility;
    const auto sampleIsVisible = [node_is_visible, &selection_visibility, force_bbox](const DrawableSample& sample) {
        return node_is_visible && sample.visible &&
            selection_visibility[sample.drawable_id] &&
            !force_bbox;
    };

    // Update visibility.
    if (update_visibility) {
        const bool force_wireframe_on = (m_display_style == M3dView::DisplayStyle::kWireFrame);
        const bool wireframe_on = m_is_selected || force_wireframe_on;

        // Points.
        for (auto& points : m_points) {
            if (!points.item)
                continue;

            const auto& sample = scene_sample.drawable_samples[points.drawable_id];
            // For points we have a single render item for shaded view and
            // selection highlighting.
            points.item->enable(sampleIsVisible(sample));
        }

        // Lines.
        for (auto& lines : m_lines) {
            const auto& sample = scene_sample.drawable_samples[lines.drawable_id];
            const bool sample_is_visible = sampleIsVisible(sample);
            if (lines.wireframe_item) {
                const bool enabled = sample_is_visible && wireframe_on;
                lines.wireframe_item->enable(enabled);
            }
            if (lines.shaded_item)
                lines.shaded_item->enable(sample_is_visible);
        }

        // Meshes.
        for (auto& mesh : m_meshes) {
            const auto& sample = scene_sample.drawable_samples[mesh.drawable_id];
            const bool sample_is_visible = sampleIsVisible(sample);

            // Update wireframe visibility.
            if (mesh.wireframe_item) {
                const bool enabled = sample_is_visible && wireframe_on;
                mesh.wireframe_item->enable(enabled);
            }
            // Update shaded and textured visibility.
            for (auto render_item : { mesh.shaded_item, mesh.textured_item })
                if (render_item)
                    render_item->enable(sample_is_visible);
        }
    }

    // Update shaders.
    // This needs to come before updating the geometry streams, since
    // setGeometryForRenderItem requires a valid shader to be set to the
    // render item.

    // Wireframe items.
    if (update_wire_shaders) {
        // Lines.
        for (auto& lines : m_lines) {
            if (!lines.wireframe_item)
                continue;
            const auto& sample = scene_sample.drawable_samples[lines.drawable_id];
            lines.wireframe_item->setShader(m_wire_shader.get());
        }

        // Meshes.
        for (auto& mesh : m_meshes) {
            if (!mesh.wireframe_item)
                continue;
            const auto& sample = scene_sample.drawable_samples[mesh.drawable_id];
            mesh.wireframe_item->setShader(m_wire_shader.get());
        }
    }

    // Shaded items.
    const auto& color_overrides = m_shape_node->getDiffuseColorOverrides();
    const auto getDrawableColor = [&color_overrides](DrawableID drawable_id) {
        const auto color_it = color_overrides.find(drawable_id);
        if (color_it != color_overrides.end()) {
            return color_it->second.diffuse_color;
        } else {
            return DEFAULT_SHADER_COLOR;
        }
    };
    if (update_shaded_shaders) {

        // Lines.
        for (auto& lines : m_lines) {
            if (!lines.shaded_item)
                continue;
            const auto& sample = scene_sample.drawable_samples[lines.drawable_id];
            const auto color = getDrawableColor(sample.drawable_id);
            setFlatShaderColor(lines.shader.get(), color);
        }

        // Meshes.
        for (auto& mesh : m_meshes) {
            if (!mesh.shaded_item)
                continue;
            const auto& sample = scene_sample.drawable_samples[mesh.drawable_id];
            const auto diffuse_color = getDrawableColor(sample.drawable_id);
            setShadedShaderDiffuseColor(mesh.shaded_shader.get(), diffuse_color);
        }
    }

    // Textured items.
    const auto getDrawableTexturePath = [&color_overrides](DrawableID drawable_id) {
        const auto color_it = color_overrides.find(drawable_id);
        if (color_it != color_overrides.end()) {
            return color_it->second.diffuse_texture_path;
        } else {
            return std::string();
        }
    };
    if (update_textured_shaders) {
        for (auto& mesh : m_meshes) {
            if (!mesh.textured_item)
                continue;
            const auto& sample = scene_sample.drawable_samples[mesh.drawable_id];
            const auto& texture_path = getDrawableTexturePath(sample.drawable_id);
            const bool texture_is_empty = texture_path.empty();
            const bool texture_was_empty = mesh.texture_path.empty();
            const auto texture_path_changed = updateValue(mesh.texture_path, texture_path);
            if (!texture_path_changed)
                continue;
            if (!texture_is_empty) {
                mesh.texture = loadTexture(texture_path);
                setTexturedShaderTexture(mesh.textured_shader.get(), mesh.texture.get());
            }
            // If no texture is bound, show the shaded item in textured mode too.
            if (texture_is_empty != texture_was_empty) {
                auto shaded_draw_mode = MGeometry::kShaded;
                if (texture_is_empty)
                    shaded_draw_mode = MGeometry::DrawMode(shaded_draw_mode | MGeometry::kTextured);
                mesh.shaded_item->setDrawMode(shaded_draw_mode);

                auto textured_draw_mode = MGeometry::kTextured;
                if (texture_is_empty)
                    textured_draw_mode = MGeometry::DrawMode(0);
                mesh.textured_item->setDrawMode(textured_draw_mode);
            }
        }
    }

    // Point shaders.
    const auto update_point_shaders = node_selection_updated ||
        (m_is_selected ? update_wire_shaders : update_shaded_shaders);
    if (update_point_shaders) {
        for (auto& points : m_points) {
            if (!points.item)
                continue;
            const auto& sample = scene_sample.drawable_samples[points.drawable_id];
            const auto color = m_is_selected
                ? C3f(m_wire_color.r, m_wire_color.g, m_wire_color.b)
                : getDrawableColor(sample.drawable_id);
            setPointShaderSolidColor(points.shader.get(), color);
        }
    }

    // Update render item buffers.
    if (update_streams) {

        if (m_vp2_cache_handle)
            m_vp2_cache_handle->updateVP2Buffers(m_sample_time, scene_sample.drawable_samples, m_buffers);

        // Assign buffers to render items.
        const auto addBufferIfNotEmpty = [&](MVertexBufferArray& vba, const char* buffer_name, MVertexBuffer* buffer) {
            if (buffer && buffer->vertexCount() > 0)
                vba.addBuffer(buffer_name, buffer);
        };

        // Points.
        for (auto& points : m_points) {
            const auto& sample = scene_sample.drawable_samples[points.drawable_id];
            if (!sample.visible)
                continue;

            auto& vp2_buffers = m_buffers[points.drawable_id];
            if (!vp2_buffers.geometry)
                continue;

            MVertexBufferArray vba;
            addBufferIfNotEmpty(vba, "positions", &vp2_buffers.geometry->positions);
            const auto index_buffer = MIndexBuffer(MGeometry::kUnsignedInt32);
            const MBoundingBox maya_bbox = mayaFromImath(sample.bbox);
            CHECK_MSTATUS(setGeometryForRenderItem(*points.item, vba, index_buffer, &maya_bbox));
        }

        // Lines.
        for (auto& lines : m_lines) {
            const auto& sample = scene_sample.drawable_samples[lines.drawable_id];
            if (!sample.visible)
                continue;

            auto& vp2_buffers = m_buffers[lines.drawable_id];
            if (!vp2_buffers.geometry)
                continue;

            const MBoundingBox maya_bbox = mayaFromImath(sample.bbox);

            // Wireframe render item.
            {
                MVertexBufferArray vba;
                addBufferIfNotEmpty(vba, "positions", &vp2_buffers.geometry->positions);
                const auto index_buffer = vp2_buffers.indices
                    ? vp2_buffers.indices->wireframe_buffer
                    : MIndexBuffer(MGeometry::kUnsignedInt32);
                CHECK_MSTATUS(setGeometryForRenderItem(*lines.wireframe_item, vba, index_buffer, &maya_bbox));
            }

            // Shaded render item.
            {
                MVertexBufferArray vba;
                addBufferIfNotEmpty(vba, "positions", &vp2_buffers.geometry->positions);
                const auto index_buffer = vp2_buffers.indices
                    ? vp2_buffers.indices->wireframe_buffer
                    : MIndexBuffer(MGeometry::kUnsignedInt32);
                CHECK_MSTATUS(setGeometryForRenderItem(*lines.shaded_item, vba, index_buffer, &maya_bbox));
            }
        }

        // Meshes.
        for (auto& mesh : m_meshes) {
            const auto& sample = scene_sample.drawable_samples[mesh.drawable_id];
            if (!sample.visible)
                continue;

            auto& vp2_buffers = m_buffers[mesh.drawable_id];
            if (!vp2_buffers.geometry)
                continue;

            const MBoundingBox maya_bbox = mayaFromImath(sample.bbox);

            // Wireframe render item.
            {
                MVertexBufferArray vba;
                addBufferIfNotEmpty(vba, "positions", &vp2_buffers.geometry->positions);
                const auto index_buffer = vp2_buffers.indices
                    ? vp2_buffers.indices->wireframe_buffer
                    : MIndexBuffer(MGeometry::kUnsignedInt32);
                CHECK_MSTATUS(setGeometryForRenderItem(*mesh.wireframe_item, vba, index_buffer, &maya_bbox));
            }

            // Shaded render item.
            {
                MVertexBufferArray vba;
                addBufferIfNotEmpty(vba, "positions", &vp2_buffers.geometry->positions);
                if (vp2_buffers.geometry->flags & VP2GeometrySample::HAS_NORMALS)
                    addBufferIfNotEmpty(vba, "normals", &vp2_buffers.geometry->normals);
                const auto index_buffer = vp2_buffers.indices
                    ? vp2_buffers.indices->triangle_buffer
                    : MIndexBuffer(MGeometry::kUnsignedInt32);
                CHECK_MSTATUS(setGeometryForRenderItem(*mesh.shaded_item, vba, index_buffer, &maya_bbox));
            }

            // Textured render item;
            {
                MVertexBufferArray vba;
                addBufferIfNotEmpty(vba, "positions", &vp2_buffers.geometry->positions);
                if (vp2_buffers.geometry->flags & VP2GeometrySample::HAS_NORMALS)
                    addBufferIfNotEmpty(vba, "normals", &vp2_buffers.geometry->normals);
                if (vp2_buffers.texcoords)
                    addBufferIfNotEmpty(vba, "uvs", &vp2_buffers.texcoords->uvs);
                // TODO: tangents, bitangents
                const auto index_buffer = vp2_buffers.indices
                    ? vp2_buffers.indices->triangle_buffer
                    : MIndexBuffer(MGeometry::kUnsignedInt32);
                CHECK_MSTATUS(setGeometryForRenderItem(*mesh.textured_item, vba, index_buffer, &maya_bbox));
            }
        }
    }

    // Update world matrices.
    if (update_matrices) {
        m_update_world_matrix_required = false;

        for (auto& points : m_points) {
            const auto& sample = scene_sample.drawable_samples[points.drawable_id];
            if (!sample.visible)
                continue;
            const auto& world_matrix = mayaFromImath(sample.world_matrix) * node_world_matrix;
            points.item->setMatrix(&world_matrix);
        }

        for (auto& lines : m_lines) {
            const auto& sample = scene_sample.drawable_samples[lines.drawable_id];
            if (!sample.visible)
                continue;
            const auto& world_matrix = mayaFromImath(sample.world_matrix) * node_world_matrix;
            for (auto render_item : { lines.wireframe_item, lines.shaded_item })
                if (render_item)
                    render_item->setMatrix(&world_matrix);
        }

        for (auto& mesh : m_meshes) {
            const auto& sample = scene_sample.drawable_samples[mesh.drawable_id];
            if (!sample.visible)
                continue;
            const auto& world_matrix = mayaFromImath(sample.world_matrix) * node_world_matrix;
            for (auto render_item : { mesh.wireframe_item, mesh.shaded_item, mesh.textured_item })
                if (render_item)
                    render_item->setMatrix(&world_matrix);
        }
    }
}

const char* SubSceneOverride::BBoxWireframe::RENDER_ITEM_NAME = "alembicHolder_bbox";
const Imath::Box3f SubSceneOverride::BBoxWireframe::DEFAULT_BBOX =
    Imath::Box3f(Imath::V3f(-0.5f, -0.5f, -0.5f), Imath::V3f(0.5f, 0.5f, 0.5f));
const uint32_t SubSceneOverride::BBoxWireframe::BOX_INDICES[] =
    {0, 1, 2, 3, 4, 5, 6, 7, 0, 2, 1, 3, 4, 6, 5, 7, 0, 4, 1, 5, 2, 6, 3, 7};

SubSceneOverride::BBoxWireframe::BBoxWireframe()
    : render_item(nullptr)
    , position_buffer(MVertexBufferDescriptor("", MGeometry::kPosition, MGeometry::kFloat, 3))
    , index_buffer(MGeometry::kUnsignedInt32)
{
    index_buffer.update(BOX_INDICES, 0, 24, true);
}

void SubSceneOverride::BBoxWireframe::initRenderItem()
{
    render_item = MRenderItem::Create(RENDER_ITEM_NAME, MRenderItem::DecorationItem, MGeometry::kLines);
    render_item->setDrawMode(MGeometry::DrawMode(MGeometry::kWireframe | MGeometry::kShaded | MGeometry::kTextured));
    render_item->depthPriority(MRenderItem::sActiveWireDepthPriority);
    render_item->setSelectionMask(MSelectionMask(kPluginId));
    render_item->enable(false);
}

void SubSceneOverride::BBoxWireframe::updateBBox(MHWRender::MPxSubSceneOverride& parent, const Box3f& bbox)
{
    const auto extent = bbox.max - bbox.min;
    auto ptr = static_cast<V3f*>(position_buffer.acquire(8, false));
    const auto zeroOne = [](int f) { return (f != 0) ? 1.0f : 0.0f; };
    for (int i = 0; i < 8; ++i) {
        ptr[i] = bbox.min + extent * V3f(zeroOne(i & 1), zeroOne(i & 2), zeroOne(i & 4));
    }
    position_buffer.commit(ptr);

    MVertexBufferArray vba;
    vba.addBuffer("positions", &position_buffer);
    const auto maya_bbox = mayaFromImath(bbox);
    CHECK_MSTATUS(parent.setGeometryForRenderItem(*render_item, vba, index_buffer, &maya_bbox));
}

bool SubSceneOverride::getSelectionPath(const MHWRender::MRenderItem& renderItem, MDagPath& dagPath) const
{
    dagPath = MDagPath::getAPathTo(m_object);
    return true;
}


VP2Scene::VP2Scene(const HierarchyNodeCategories& categories)
{
    m_vp2_caches.reserve(categories.drawableCount());
    for (const auto& drawable : categories.drawables())
        m_vp2_caches.emplace_back(drawable.node_ref);
}

void VP2Scene::updateVP2Buffers(chrono_t time, const DrawableSampleVector& in_samples, VP2BufferHandlesVector& out_buffers)
{
    tbb::mutex::scoped_lock guard(m_mutex);

    out_buffers.resize(in_samples.size());
    for (size_t i = 0; i < in_samples.size(); ++i) {
        auto& cache = m_vp2_caches[in_samples[i].drawable_id];
        out_buffers[i] = cache.sampleBuffers(time, in_samples[i].cache_handles);
    }
}


namespace {

    template <typename T, typename GPUBuffer>
    void updateVP2Buffer(const std::vector<T>& input, GPUBuffer& output)
    {
        if (input.empty())
            return;
        output.update(input.data(), 0, unsigned(input.size()), true);
    }

    void updateVP2IndexBufferSample(const IndexBufferSample& cpu_buffer, VP2IndexBufferSample& out_gpu_buffer)
    {
        updateVP2Buffer(cpu_buffer.triangle_buffer, out_gpu_buffer.triangle_buffer);
        updateVP2Buffer(cpu_buffer.wireframe_buffer, out_gpu_buffer.wireframe_buffer);
    }

    void updateVP2GeometrySample(const GeometrySample& input, VP2GeometrySample& output)
    {
        updateVP2Buffer(input.positions, output.positions);
        updateVP2Buffer(input.normals, output.normals);
        updateVP2Buffer(input.tangents, output.tangents);
        updateVP2Buffer(input.bitangents, output.bitangents);

        output.flags = VP2GeometrySample::POSITIONS_ONLY;
        if (!input.normals.empty()) {
            assert(input.normals.size() == input.positions.size());
            output.flags |= VP2GeometrySample::HAS_NORMALS;
        }
        if (!input.tangents.empty()) {
            assert(input.tangents.size() == input.positions.size());
            assert(input.bitangents.size() == input.positions.size());
            output.flags |= VP2GeometrySample::HAS_TANGENT_BASIS;
        }
    }

    void updateVP2TexCoordsSample(const TexCoordsSample& input, VP2TexCoordsSample& output)
    {
        updateVP2Buffer(input.uvs, output.uvs);
    }

} // unnamed namespace

VP2DrawableBufferCache::VP2DrawableBufferCache(const Hierarchy::Node& node)
    : m_has_indices(false)
    , m_num_samples(0)
    , m_constant_indices(true)
    , m_constant_geometry(true)
    , m_constant_texcoords(true)
{
    if (node.type == Hierarchy::NodeType::POLYMESH) {
        auto schema = Alembic::AbcGeom::IPolyMesh(node.source_object).getSchema();
        auto face_counts_property = schema.getFaceCountsProperty();
        m_time_sampling = face_counts_property.getTimeSampling();
        m_num_samples = face_counts_property.getNumSamples();
        m_has_indices = face_counts_property.valid();
        m_constant_indices = schema.getTopologyVariance() != kHeterogeneousTopology;
        const auto normals_param = schema.getNormalsParam();
        const bool constant_normals = !normals_param.valid() || normals_param.isConstant();
        m_constant_geometry = schema.getPositionsProperty().isConstant() && constant_normals;
        const auto uvs_param = schema.getUVsParam();
        m_constant_texcoords = !uvs_param.valid() || uvs_param.isConstant();

    } else if (node.type == Hierarchy::NodeType::CURVES) {
        auto schema = Alembic::AbcGeom::ICurves(node.source_object).getSchema();
        auto positions_property = schema.getPositionsProperty();
        m_time_sampling = positions_property.getTimeSampling();
        m_num_samples = positions_property.getNumSamples();
        m_has_indices = true;
        m_constant_indices = schema.getTopologyVariance() != kHeterogeneousTopology;
        m_constant_geometry = positions_property.isConstant();

    } else if (node.type == Hierarchy::NodeType::POINTS) {
        auto schema = Alembic::AbcGeom::IPoints(node.source_object).getSchema();
        auto positions_property = schema.getPositionsProperty();
        m_time_sampling = positions_property.getTimeSampling();
        m_num_samples = positions_property.getNumSamples();
        m_constant_geometry = positions_property.isConstant();

    } else {
        abort();
    }
}

VP2BufferHandles VP2DrawableBufferCache::sampleBuffers(chrono_t time, const DrawableCacheHandles& cache_handles)
{
    VP2BufferHandles res;

    // Indices.
    if (m_has_indices) {
        const auto indices_sidx = m_constant_indices ? 0 : m_time_sampling->getFloorIndex(time, m_num_samples).first;
        res.indices = m_vp2_index_buffer_cache.get(indices_sidx);
        if (!res.indices && cache_handles.indices) {
            auto indices = std::unique_ptr<VP2IndexBufferSample>(new VP2IndexBufferSample());
            updateVP2IndexBufferSample(*cache_handles.indices, *indices);
            res.indices = m_vp2_index_buffer_cache.put(indices_sidx, indices.release());
        }
    }

    // Geometry (positions, normals).
    const auto geometry_key = m_constant_geometry ? 0 : time;
    res.geometry = m_vp2_geometry_sample_cache.get(geometry_key);
    if (!res.geometry && cache_handles.geometry.endpoints[0]) {
        const auto cpu_geo = cache_handles.geometry;
        auto geometry = std::unique_ptr<VP2GeometrySample>(new VP2GeometrySample());

        // Interpolate if needed.
        if (cache_handles.geometry.endpoints[1] && cache_handles.geometry.alpha != 0) {
            const unsigned int vertex_count = unsigned(cpu_geo.endpoints[0]->positions.size());
            // Positions.
            {
                auto position_buffer = static_cast<V3f*>(geometry->positions.acquire(vertex_count));
                lerpSpans(cpu_geo.alpha,
                    Span<const V3f>(cpu_geo.endpoints[0]->positions),
                    Span<const V3f>(cpu_geo.endpoints[1]->positions),
                    Span<V3f>(position_buffer, vertex_count));
                geometry->positions.commit(position_buffer);
            }
            // Normals.
            if (!cpu_geo.endpoints[0]->normals.empty()) {
                auto normal_buffer = static_cast<V3f*>(geometry->normals.acquire(vertex_count));
                lerpSpans(cpu_geo.alpha,
                    Span<const V3f>(cpu_geo.endpoints[0]->normals),
                    Span<const V3f>(cpu_geo.endpoints[1]->normals),
                    Span<V3f>(normal_buffer, vertex_count));
                geometry->normals.commit(normal_buffer);
                geometry->flags |= VP2GeometrySample::HAS_NORMALS;
            }

        } else {
            // Simply copy endpoints[0].
            updateVP2GeometrySample(*cpu_geo.endpoints[0], *geometry);
        }

        res.geometry = m_vp2_geometry_sample_cache.put(geometry_key, geometry.release());
    }

    // UVs.
    const auto texcoords_key = m_constant_texcoords ? 0 : time;
    res.texcoords = m_vp2_texcoords_sample_cache.get(texcoords_key);
    if (!res.texcoords && cache_handles.texcoords.endpoints[0]) {
        const auto cpu_uvs = cache_handles.texcoords;
        auto texcoords = std::unique_ptr<VP2TexCoordsSample>(new VP2TexCoordsSample());

        // Interpolate if needed.
        if (cache_handles.texcoords.endpoints[1] && cache_handles.texcoords.alpha != 0) {
            const unsigned int vertex_count = unsigned(cpu_uvs.endpoints[0]->uvs.size());
            auto uv_buffer = static_cast<V2f*>(texcoords->uvs.acquire(vertex_count));
            lerpSpans(cpu_uvs.alpha,
                Span<const V2f>(cpu_uvs.endpoints[0]->uvs),
                Span<const V2f>(cpu_uvs.endpoints[1]->uvs),
                Span<V2f>(uv_buffer, vertex_count));
            texcoords->uvs.commit(uv_buffer);

        } else {
            // Simply copy endpoints[0].
            updateVP2TexCoordsSample(*cpu_uvs.endpoints[0], *texcoords);
        }

        res.texcoords = m_vp2_texcoords_sample_cache.put(texcoords_key, texcoords.release());
    }

    return res;
}

VP2SceneCache& VP2SceneCache::instance()
{
    static VP2SceneCache instance;
    return instance;
}

VP2ScenePtr VP2SceneCache::getScene(const AlembicSceneKey& key, const AlembicScenePtr& scene)
{
    if (!scene)
        return nullptr;
    tbb::mutex::scoped_lock guard(m_mutex);
    auto ptr = m_scene_cache.get(key);
    if (ptr)
        return ptr;
    return m_scene_cache.put(key, new VP2Scene(scene->nodeCategories()));
}

} // namespace AlembicHolder
