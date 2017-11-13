/*Alembic Holder
Copyright (c) 2014, Gaël Honorez, All rights reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library.*/


#include "nozAlembicHolderNode.h"
#include "parseJsonShaders.h"
#include "../../common/PathUtil.h"

#include "gpuCacheGLPickingSelect.h"
#include "gpuCacheRasterSelect.h"

#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MGlobal.h>
#include <maya/MTime.h>

#include <maya/MDrawData.h>
#include <maya/MMaterial.h>
#include <maya/MSelectionMask.h>
#include <maya/MSelectionList.h>
#include <maya/MRenderView.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFileObject.h>
#include <maya/MSelectionList.h>
#include <maya/MRenderView.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MFileObject.h>
#include <maya/MObjectArray.h>

#include <stdio.h>
#include <map>
#include <algorithm>


#include <maya/MHardwareRenderer.h>
#include <maya/MGLFunctionTable.h>

// Wireframe line style defines
#define LINE_STIPPLE_SHORTDASHED    0x0303

#define LEAD_COLOR                18    // green

#include <stdio.h>
#include <map>
#include <unordered_map>

#define LEAD_COLOR                18    // green
#define ACTIVE_COLOR            15    // white
#define ACTIVE_AFFECTED_COLOR    8    // purple
#define DORMANT_COLOR            4    // blue
#define HILITE_COLOR            17    // pale blue
//#include "timer.h"

namespace AlembicHolder {

//
static MGLFunctionTable *gGLFT = NULL;

//#include "boost/foreach.hpp"


// The id is a 32bit value used to identify this type of node in the binary file format.
MTypeId nozAlembicHolder::id(0x00114956);

MObject nozAlembicHolder::aAbcFile;
MObject nozAlembicHolder::aObjectPath;
MObject nozAlembicHolder::aSelectionPath;
MObject nozAlembicHolder::aBoundingExtended;
MObject nozAlembicHolder::aTime;
MObject nozAlembicHolder::aTimeOffset;
MObject nozAlembicHolder::aShaderPath;
MObject nozAlembicHolder::aForceReload;

MObject nozAlembicHolder::aJsonFile;
MObject nozAlembicHolder::aJsonFileSecondary;
MObject nozAlembicHolder::aShadersNamespace;
MObject nozAlembicHolder::aShadersAttribute;
MObject nozAlembicHolder::aAbcShaders;
MObject nozAlembicHolder::aUvsArchive;
MObject nozAlembicHolder::aShadersAssignation;
MObject nozAlembicHolder::aAttributes;
MObject nozAlembicHolder::aDisplacementsAssignation;
MObject nozAlembicHolder::aLayersOverride;
MObject nozAlembicHolder::aSkipJsonFile;
MObject nozAlembicHolder::aSkipShaders;
MObject nozAlembicHolder::aSkipAttributes;
MObject nozAlembicHolder::aSkipLayers;
MObject nozAlembicHolder::aSkipDisplacements;


MObject nozAlembicHolder::aUpdateAssign;
MObject nozAlembicHolder::aUpdateCache;

// for bbox
MObject nozAlembicHolder::aBoundMin;
MObject nozAlembicHolder::aBoundMax;

nozAlembicHolder::nozAlembicHolder()
{
    if (gGLFT == NULL)
        gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();
}

nozAlembicHolder::~nozAlembicHolder()
{
}

bool nozAlembicHolder::isBounded() const
{
    return true;
}

MBoundingBox nozAlembicHolder::boundingBox() const
{
    updateCache();
    if (m_scene)
        return mayaFromImath(m_sample.hierarchy_stat.bbox);
    else
        return mayaFromImath(Box3d({-0.5, -0.5, -0.5}, {0.5, 0.5, 0.5}));
}

void* nozAlembicHolder::creator()
{
    return new nozAlembicHolder();
}

void nozAlembicHolder::postConstructor()
{
    // This call allows the shape to have shading groups assigned
    setRenderable(true);
}

void nozAlembicHolder::copyInternalData(MPxNode* srcNode) {
    // here we ensure that the scene manager stays up to date when duplicating nodes

    const nozAlembicHolder& node = *(nozAlembicHolder*)srcNode;

    MFnDagNode fn(node.thisMObject());
    MString abcfile;
    MPlug plug = fn.findPlug(aAbcFile);
    plug.getValue(abcfile);
    abcfile = abcfile.expandFilePath();
    MString objectPath;
    plug = fn.findPlug(aObjectPath);
    plug.getValue(objectPath);

    MString selectionPath;
    plug = fn.findPlug(aSelectionPath);
    plug.getValue(selectionPath);

    bool extendedMode = false;
    plug = fn.findPlug(aBoundingExtended);
    plug.getValue(extendedMode);

    updateCache();
    updateAssign();
}

MStatus nozAlembicHolder::initialize() {
    // init maya stuff

    MFnTypedAttribute tAttr;
    MFnNumericAttribute nAttr;
    MFnMessageAttribute mAttr;
    MFnUnitAttribute uAttr;
    MStatus stat;

    aAbcFile = tAttr.create("cacheFileName", "cfn", MFnStringData::kString, MObject::kNullObj);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aObjectPath = tAttr.create("cacheGeomPath", "cmp", MFnStringData::kString);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aSelectionPath = tAttr.create("cacheSelectionPath", "csp", MFnStringData::kString);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aSelectionPath = tAttr.create("cacheSelectionPath", "csp", MFnStringData::kString);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aShaderPath = mAttr.create("shaders", "shds", &stat);
    mAttr.setArray(true);
    mAttr.setInternal(true);

    aTime = uAttr.create("time", "t", MFnUnitAttribute::kTime);
    uAttr.setStorable(false);
    uAttr.setKeyable(true);
    uAttr.setReadable(true);
    uAttr.setWritable(true);

    aTimeOffset = uAttr.create("timeOffset", "to", MFnUnitAttribute::kTime);
    uAttr.setStorable(true);
    uAttr.setKeyable(true);

    aBoundingExtended = nAttr.create("boundingBoxExtendedMode", "bem", MFnNumericData::kBoolean, false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(false);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);

    aJsonFile = tAttr.create("jsonFile", "jf", MFnStringData::kString, MObject::kNullObj);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aJsonFileSecondary = tAttr.create("secondaryJsonFile", "sjf", MFnStringData::kString, MObject::kNullObj);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aShadersNamespace = tAttr.create("shadersNamespace", "sn", MFnStringData::kString, MObject::kNullObj);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aShadersAttribute = tAttr.create("shadersAttribute", "sattr", MFnStringData::kString, MObject::kNullObj);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aAbcShaders = tAttr.create("abcShaders", "abcs", MFnStringData::kString, MObject::kNullObj);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aUvsArchive = tAttr.create("uvsArchive", "uvsa", MFnStringData::kString, MObject::kNullObj);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aShadersAssignation = tAttr.create("shadersAssignation", "sa", MFnStringData::kString, MObject::kNullObj);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aAttributes = tAttr.create("attributes", "attr", MFnStringData::kString, MObject::kNullObj);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aDisplacementsAssignation = tAttr.create("displacementsAssignation", "da", MFnStringData::kString, MObject::kNullObj);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aLayersOverride = tAttr.create("layersOverride", "lo", MFnStringData::kString, MObject::kNullObj);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aSkipJsonFile = nAttr.create("skipJsonFile", "sjf", MFnNumericData::kBoolean, false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(false);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);

    aSkipShaders = nAttr.create("skipShaders", "ss", MFnNumericData::kBoolean, false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(false);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);

    aSkipAttributes = nAttr.create("skipAttributes", "sat", MFnNumericData::kBoolean, false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(false);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);

    aSkipLayers = nAttr.create("skipLayers", "sl", MFnNumericData::kBoolean, false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(false);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);

    aSkipDisplacements = nAttr.create("skipDisplacements", "sd", MFnNumericData::kBoolean, false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(false);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);

    aUpdateAssign = nAttr.create("updateAssign", "uass", MFnNumericData::kInt, 0);
    nAttr.setHidden(true);
    nAttr.setStorable(false);
    nAttr.setReadable(false);
    nAttr.setWritable(false);

    aUpdateCache = nAttr.create("updateCache", "upc", MFnNumericData::kInt, 0);
    nAttr.setHidden(true);
    nAttr.setStorable(false);
    nAttr.setReadable(false);
    nAttr.setWritable(false);

    aForceReload = nAttr.create("forceReload", "frel", MFnNumericData::kBoolean, false, &stat);
    nAttr.setDefault(false);
    nAttr.setKeyable(false);

    aBoundMin = nAttr.create( "MinBoundingBox", "min", MFnNumericData::k3Float, -1.0, &stat);
    nAttr.setWritable( false );
    nAttr.setStorable( false );

    aBoundMax = nAttr.create( "MaxBoundingBox", "ma", MFnNumericData::k3Float, 1.0, &stat );
    nAttr.setWritable( false );
    nAttr.setStorable( false );

    // Add the attributes we have created to the node
    //
    addAttribute(aAbcFile);
    addAttribute(aObjectPath);
    addAttribute(aSelectionPath);
    addAttribute(aBoundingExtended);
    addAttribute(aForceReload);
    addAttribute(aShaderPath);
    addAttribute(aTime);
    addAttribute(aTimeOffset);

	addAttribute(aJsonFile);
	addAttribute(aJsonFileSecondary);
	addAttribute(aShadersNamespace);
	addAttribute(aShadersAttribute);
	addAttribute(aAbcShaders);
	addAttribute(aUvsArchive);
	addAttribute(aShadersAssignation);
	addAttribute(aAttributes);
	addAttribute(aDisplacementsAssignation);
	addAttribute(aLayersOverride);
	addAttribute(aSkipJsonFile);
	addAttribute(aSkipShaders);
	addAttribute(aSkipAttributes);
	addAttribute(aSkipLayers);
	addAttribute(aSkipDisplacements);

	addAttribute(aUpdateAssign);
    addAttribute(aUpdateCache);
    addAttribute(aBoundMin);
    addAttribute(aBoundMax);

    // Update cache
    attributeAffects(aAbcFile, aUpdateCache);
    attributeAffects(aTime, aUpdateCache);
    attributeAffects(aTimeOffset, aUpdateCache);
    attributeAffects(aSelectionPath, aUpdateCache);
    attributeAffects(aObjectPath, aUpdateCache);
    attributeAffects(aBoundingExtended, aUpdateCache);
    attributeAffects(aForceReload, aUpdateCache);
    attributeAffects(aShadersAssignation, aUpdateCache);

    // Update assinations
	attributeAffects(aJsonFile, aUpdateAssign);
	attributeAffects(aJsonFileSecondary, aUpdateAssign);
	attributeAffects(aAttributes, aUpdateAssign);
	attributeAffects(aLayersOverride, aUpdateAssign);
	attributeAffects(aSkipJsonFile, aUpdateAssign);
	attributeAffects(aSkipAttributes, aUpdateAssign);
	attributeAffects(aSkipLayers, aUpdateAssign);

	return MS::kSuccess;
}

MSelectionMask nozAlembicHolder::getShapeSelectionMask() const
{
    return MSelectionMask("alembicHolder");
}

const nozAlembicHolder::SceneSample& nozAlembicHolder::getSample() const
{
    updateCache();
    return m_sample;
}

const AlembicScenePtr& nozAlembicHolder::getScene() const
{
    updateCache();
    return m_scene;
}

const AlembicSceneKey& nozAlembicHolder::getSceneKey() const
{
    updateCache();
    return m_scene_key;
}

std::string nozAlembicHolder::getSelectionKey() const {
    return std::string(MPlug(thisMObject(), aSelectionPath).asString().asChar());
}

chrono_t nozAlembicHolder::getTime() const
{
    auto time = MPlug(thisMObject(), aTime).asMTime() + MPlug(thisMObject(), aTimeOffset).asMTime();
    return time.as(MTime::kSeconds);
}

bool nozAlembicHolder::isBBExtendedMode() const
{
    return MPlug(thisMObject(), aBoundingExtended).asBool();
}

const DiffuseColorOverrideMap& nozAlembicHolder::getDiffuseColorOverrides() const
{
    updateCache();
    return m_diffuse_color_overrides;
}

std::string nozAlembicHolder::getShaderAssignmentsJson() const
{
    return MPlug(thisMObject(), aShadersAssignation).asString().asChar();
}

void nozAlembicHolder::updateDiffuseColorOverrides()
{
    if (!m_scene)
        return;

    m_diffuse_color_overrides.clear();

    Json::Value jroot;
    Json::Reader reader;
    if (!reader.parse(m_shader_assignments, jroot, false))
        return;

    std::map<std::string, MColor> shaderColors;
    ParseShaders(jroot, shaderColors);

    for (const auto& drawable : m_scene->nodeCategories().drawables())
    {
        const auto fullName = drawable.node_ref.source_object.getFullName();

        for (auto it = shaderColors.begin(); it != shaderColors.end(); ++it)
        {
            //check both path & tag
            const bool matches_exactly = it->first.find("/") != std::string::npos &&
                fullName.find(it->first) != std::string::npos;
            const bool matches_wildcard = matchPattern(fullName, it->first);
            if (matches_exactly || matches_wildcard)
            {
                const auto& color = it->second;
                auto& entry = m_diffuse_color_overrides[drawable.drawable_id];
                entry.diffuse_color = C3f(color.r, color.g, color.b);
            }
        }
    }
}

MStatus nozAlembicHolder::compute(const MPlug& plug, MDataBlock& block)
{
	if (plug == aUpdateAssign)
	{
		MStatus status;
        MFnDagNode fn(thisMObject());

		// Try to parse the JSON attributes here.
		bool skipJson = false;
		bool skipAttributes = false;
		bool skipLayers = false;
		bool customLayer = false;
		std::string layerName = "defaultRenderLayer";
		MObject currentRenderLayerObj = MFnRenderLayer::currentLayer(&status);
		if (status)
		{
			MFnRenderLayer currentRenderLayer(currentRenderLayerObj, &status);
			if (status)
				layerName = currentRenderLayer.name().asChar();
		}
        if(layerName != std::string("defaultRenderLayer"))
            customLayer = true;

		Json::Value jrootattributes;
		Json::Value jrootLayers;

		MPlug skipJsonPlug = fn.findPlug("skipJsonFile", &status);
		if(status == MS::kSuccess)
			skipJson =  block.inputValue(skipJsonPlug).asBool();
		MPlug skipAttributesPlug = fn.findPlug("skipAttributes", &status);
		if(status == MS::kSuccess)
			skipAttributes =  block.inputValue(skipAttributesPlug).asBool();
		MPlug skipLayersPlug = fn.findPlug("skipLayers", &status);
		if(status == MS::kSuccess)
			skipLayers =  block.inputValue(skipLayersPlug).asBool();

		bool parsingSuccessful = false;
		MPlug jsonFile = fn.findPlug("jsonFile", &status);

		if(status == MS::kSuccess && skipJson==false )
		{
			Json::Value jroot;
			Json::Reader reader;
			MString jsonFileStr = block.inputValue(jsonFile).asString();
			std::ifstream test(jsonFileStr.asChar(), std::ifstream::binary);
			parsingSuccessful = reader.parse( test, jroot, false );

			MPlug jsonFileSecondary = fn.findPlug("secondaryJsonFile", &status);
			if(status == MS::kSuccess)
			{
				MString jsonFileSecondaryStr = block.inputValue(jsonFile).asString();
				std::ifstream test2(jsonFileSecondaryStr.asChar(), std::ifstream::binary);
				Json::Value jroot2;
				if (reader.parse( test2, jroot2, false ))
					update(jroot, jroot2);
			}

			if ( parsingSuccessful )
			{
				if(skipAttributes == false)
				{

					jrootattributes = jroot["attributes"];
					MPlug attributesAssignationPlug = fn.findPlug("attributes", &status);
					if(status == MS::kSuccess)
					{
						Json::Reader readerOverride;
						Json::Value jrootattributesattributes;

						if(readerOverride.parse( block.inputValue(attributesAssignationPlug).asString().asChar(), jrootattributesattributes))
							OverrideProperties(jrootattributes, jrootattributesattributes);

					}
				}
				if(skipLayers == false && customLayer)
				{
					jrootLayers = jroot["layers"];

					MPlug layersOverridePlug = fn.findPlug("layersOverride", &status);
					if(status == MS::kSuccess)
					{
						Json::Reader readerOverride;
						Json::Value jrootLayersattributes;

						if(readerOverride.parse( block.inputValue(layersOverridePlug).asString().asChar(), jrootLayersattributes ))
						{
							jrootLayers[layerName]["removeProperties"] = jrootLayersattributes[layerName].get("removeProperties", skipAttributes).asBool();

							if(jrootLayersattributes[layerName]["properties"].size() > 0)
								OverrideProperties(jrootLayers[layerName]["properties"], jrootLayersattributes[layerName]["properties"]);
						}
					}
				}
			}
		}

		if(!parsingSuccessful)
		{
			MPlug layersOverridePlug = fn.findPlug("layersOverride", &status);

			if (customLayer && status == MS::kSuccess)
			{
				Json::Reader reader;
				bool parsingSuccessful = reader.parse( block.inputValue(layersOverridePlug).asString().asChar(), jrootLayers );

			}
			// Check if we have to skip something....
			if( jrootLayers[layerName].size() > 0 && customLayer && parsingSuccessful)
			{
				skipAttributes =jrootLayers[layerName].get("removeProperties", skipAttributes).asBool();
			}

			MPlug attributesAssignationPlug = fn.findPlug("attributes", &status);
			if (status == MS::kSuccess  && skipAttributes == false)
			{
				Json::Reader reader;
				bool parsingSuccessful = reader.parse( block.inputValue(attributesAssignationPlug).asString().asChar(), jrootattributes );
			}

		}

		if( jrootLayers[layerName].size() > 0 && customLayer)
		{
			if(jrootLayers[layerName]["properties"].size() > 0)
			{
				if(jrootLayers[layerName].get("removeProperties", skipAttributes).asBool())
					jrootattributes = jrootLayers[layerName]["properties"];
				else
					OverrideProperties(jrootattributes, jrootLayers[layerName]["properties"]);
			}
		}

		m_params.attributes.clear();

		if( jrootattributes.size() > 0 )
		{
			bool addtoPath = false;
			m_params.linkAttributes = true;
			m_params.attributesRoot = jrootattributes;
			for( Json::ValueIterator itr = jrootattributes.begin() ; itr != jrootattributes.end() ; itr++ )
			{
				addtoPath = false;

				std::string path = itr.key().asString();

				Json::Value attributes = jrootattributes[path];
				for( Json::ValueIterator itr = attributes.begin() ; itr != attributes.end() ; itr++ )
				{
					std::string attribute = itr.key().asString();
                    if (attribute == "forceVisible" || attribute == "visibility")
					{
						addtoPath = true;
						break;
					}
				}
				if(addtoPath)
					m_params.attributes.push_back(path);

			}

			std::sort(m_params.attributes.begin(), m_params.attributes.end());

		}
		else
		{
			m_params.linkAttributes = false;
		}

		block.outputValue(aForceReload).setBool(false);

	}
    else if (plug == aUpdateCache)
    {
        MStatus status;
        MFnDagNode fn(thisMObject());

        MString file = block.inputValue(aAbcFile).asString();
        MFileObject fileObject;
        fileObject.setRawFullName(file.expandFilePath());
        fileObject.setResolveMethod(MFileObject::kInputFile);
        file = fileObject.resolvedFullName();

        MString objectPath = block.inputValue(aObjectPath).asString();
        MString selectionPath = block.inputValue(aSelectionPath).asString();

        // Update scene and scene key.
        const auto scene_key_changed = updateValue(m_scene_key, AlembicSceneKey(FileRef(file.asChar()), objectPath.asChar()));
        if (!m_scene || scene_key_changed) {
            // First clear the drawable samples so the handles to the cache
            // buffers are deleted. The handles must not survive the caches.
            m_sample.drawable_samples.clear();
            m_scene = AlembicSceneCache::instance().getScene(m_scene_key);
        }

        if (m_scene) {
            // Update sample.
            const auto time_changed = updateValue(m_sample.time, getTime());
            if (m_sample.drawable_samples.empty() || time_changed) {
                m_scene->sampleHierarchy(m_sample.time, m_sample.drawable_samples, m_sample.hierarchy_stat);
            }

            // Update selection visibility.
            if (scene_key_changed) {
                m_sample.selection_visibility.assign(m_scene->nodeCategories().drawableCount(), true);
            }

            const auto selection_key_changed = updateValue(m_selection_key, getSelectionKey());
            if (scene_key_changed || selection_key_changed) {
                for (auto& sample : m_sample.drawable_samples) {
                    const auto visible = m_selection_key.empty() ||
                        pathInJsonString(m_scene->getDrawableName(sample.drawable_id), m_selection_key);
                    m_sample.selection_visibility[sample.drawable_id] = visible;
                }
            }
        }

        // Try to parse JSON Shader assignation here.
        const bool shader_assignments_changed = updateValue(m_shader_assignments, getShaderAssignmentsJson());
        if (scene_key_changed || shader_assignments_changed) {
            updateDiffuseColorOverrides();
        }

        block.outputValue(aForceReload).setBool(false);
    }
    else
    {
        return ( MS::kUnknownParameter );
    }

    block.setClean(plug);
    return MS::kSuccess;

}

// UI IMPLEMENTATION

CAlembicHolderUI::~CAlembicHolderUI() {
}

void* CAlembicHolderUI::creator() {
    return new CAlembicHolderUI();
}

CAlembicHolderUI::CAlembicHolderUI() {
}

void CAlembicHolderUI::updateVP1Drawables(const nozAlembicHolder::SceneSample& scene_sample, const DiffuseColorOverrideMap& color_overrides)
{
    m_vp1drawables.drawables.clear();
    for (const auto& drawable : scene_sample.drawable_samples) {
        const bool is_visible = drawable.visible && scene_sample.selection_visibility[drawable.drawable_id];
        if (!is_visible || !drawable.cache_handles.geometry.endpoints[0])
            continue;

        MGLenum primitive_type;
        MGLsizei element_count;
        if (drawable.type == GeometryType::TRIANGLES) {
            primitive_type = MGL_TRIANGLES;
            element_count = drawable.cache_handles.indices
                ? MGLsizei(drawable.cache_handles.indices->triangle_buffer.size())
                : MGLsizei(drawable.cache_handles.geometry.endpoints[0]->positions.size());

        } else if (drawable.type == GeometryType::LINES) {
            primitive_type = MGL_LINES;
            element_count = drawable.cache_handles.indices
                ? MGLsizei(drawable.cache_handles.indices->wireframe_buffer.size())
                : MGLsizei(drawable.cache_handles.geometry.endpoints[0]->positions.size());

        } else if (drawable.type == GeometryType::POINTS) {
            primitive_type = MGL_POINTS;
            element_count = MGLsizei(drawable.cache_handles.geometry.endpoints[0]->positions.size());

        } else {
            continue;
        }

        m_vp1drawables.drawables.push_back({primitive_type, element_count});
        auto& item = m_vp1drawables.drawables.back();
        item.world_matrix = drawable.world_matrix;
        item.drawable_id = drawable.drawable_id;

        if (drawable.cache_handles.indices) {
            if (drawable.type == GeometryType::TRIANGLES) {
                item.buffer.genIndexBuffer(Span<const uint32_t>(drawable.cache_handles.indices->triangle_buffer));
            } else if (drawable.type == GeometryType::LINES) {
                item.buffer.genIndexBuffer(Span<const uint32_t>(drawable.cache_handles.indices->wireframe_buffer));
            }
        }

        const auto& geo = drawable.cache_handles.geometry;
        std::vector<V3f> staging(geo.endpoints[0]->positions.size());
        const auto staging_span = Span<V3f>(staging);

        // Generate straight and flipped normal buffers.
        // Should work if normals and staging alias the same memory range
        // (and exactly the same, no partial overlap).
        const auto genNormals = [&item, &staging_span](const Span<const V3f>& normals) {
            item.buffer.genNormalBuffer(normals, false);
            for (size_t i = 0; i < normals.size; ++i) {
                staging_span[i] = -normals[i];
            }
            item.buffer.genNormalBuffer(staging_span, true);
        };

        // Interpolate positions and normals if needed.
        if (geo.endpoints[1] && geo.alpha != 0) {
            auto& pos0 = geo.endpoints[0]->positions;
            auto& pos1 = geo.endpoints[1]->positions;
            assert(pos0.size() == pos1.size());

            lerpSpans(geo.alpha,
                Span<const V3f>(pos0),
                Span<const V3f>(pos1),
                staging_span);
            item.buffer.genVertexBuffer(staging_span);

            auto& normals0 = geo.endpoints[0]->normals;
            auto& normals1 = geo.endpoints[1]->normals;
            assert(normals0.size() == normals1.size());
            assert(normals0.size() == pos0.size());

            if (!normals0.empty()) {
                lerpSpans(geo.alpha,
                    Span<const V3f>(normals0),
                    Span<const V3f>(normals1),
                    staging_span);
                genNormals(staging_span);
            }
        } else {
            item.buffer.genVertexBuffer(Span<const V3f>(geo.endpoints[0]->positions));
            if (!geo.endpoints[0]->normals.empty())
                item.buffer.genNormalBuffer(Span<const V3f>(geo.endpoints[0]->normals));
        }
    }
}

void CAlembicHolderUI::getDrawRequests(const MDrawInfo & info,
        bool /*objectAndActiveOnly*/, MDrawRequestQueue & queue) {

    if(MGlobal::mayaState() != MGlobal::kInteractive)
        return;

    MDrawData data;
    MDrawRequest request = info.getPrototype(*this);
    nozAlembicHolder* shapeNode = (nozAlembicHolder*) surfaceShape();
    const auto& scene = shapeNode->getScene();
    const auto& sample = shapeNode->getSample();

    // Update GL buffers.
    updateVP1Drawables(sample, shapeNode->getDiffuseColorOverrides());

    getDrawData(&m_vp1drawables, data);
    request.setDrawData(data);

    // Are we displaying meshes?
    if (!info.objectDisplayStatus(M3dView::kDisplayMeshes))
        return;

    // Use display status to determine what color to draw the object
    //
    switch (info.displayStyle()) {
    case M3dView::kWireFrame:
        getDrawRequestsWireFrame(request, info);
        queue.add(request);
        break;

    case M3dView::kGouraudShaded: {
        request.setToken(kDrawSmoothShaded);
        getDrawRequestsShaded(request, info, queue, data);
        queue.add(request);
        break;
    }

    case M3dView::kFlatShaded:
        request.setToken(kDrawFlatShaded);
        getDrawRequestsShaded(request, info, queue, data);
        queue.add(request);
        break;
    case M3dView::kBoundingBox :
            request.setToken( kDrawBoundingBox );
            getDrawRequestsBoundingBox(request, info);
            queue.add( request );
    default:
        break;
    }

    //cout << "Ending draw request" << endl;
}

namespace {
    struct GLGuard {
        GLGuard(M3dView& m3dview_): m3dview(m3dview_)
        {
            m3dview.beginGL();
            // Setup the OpenGL state as necessary
            //
            // The most straightforward way to ensure that the OpenGL
            // material parameters are properly restored after drawing is
            // to use push/pop attrib as we have no easy of knowing the
            // current values of all the parameters.
            glPushAttrib(MGL_LIGHTING_BIT);
        }
        ~GLGuard()
        {
            glPopAttrib();
            m3dview.endGL();
        }
        M3dView& m3dview;
    };
} // unnamed namespace

void CAlembicHolderUI::draw(const MDrawRequest& request, M3dView& view) const
{
    int token = request.token();

    M3dView::DisplayStatus displayStatus = request.displayStatus();

    bool cacheSelected =  ((displayStatus == M3dView::kActive) || (displayStatus == M3dView::kLead) || (displayStatus == M3dView::kHilite));
    MDrawData data = request.drawData();

    nozAlembicHolder* shapeNode = (nozAlembicHolder*) surfaceShape();
    if (!shapeNode)
        return;
    auto drawable_container = (VP1DrawableContainer*)(data.geometry());
    const auto& color_overrides = shapeNode->getDiffuseColorOverrides();

    const bool forceBoundingBox = shapeNode->isBBExtendedMode() && cacheSelected == false;
    const auto sceneKey = shapeNode->getSceneKey();
    const auto selectionKey = shapeNode->getSelectionKey();
    const bool selectionMode = !selectionKey.empty();

    GLGuard glguard(view);

    // handling refreshes
    if( token == kDrawBoundingBox || forceBoundingBox || selectionMode )
    {
        drawBoundingBox(request, view);
    }

	if (forceBoundingBox)
		return;

    switch (token)
    {
		case kDrawBoundingBox :
			break;
		case kDrawWireframe:
		case kDrawWireframeOnShaded:
			if (drawable_container && !drawable_container->drawables.empty()) {
				gGLFT->glPolygonMode(GL_FRONT_AND_BACK, MGL_LINE);
				gGLFT->glPushMatrix();
                auto color = request.color();
                gGLFT->glColor4f(color.r, color.g, color.b, 1.0f);
                drawable_container->draw(VP1DrawSettings(false));
				gGLFT->glPopMatrix();
				gGLFT->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
			else
				drawBoundingBox( request, view );
			break;
		case kDrawSmoothShaded:
		case kDrawFlatShaded:
        {
            if (drawable_container && !drawable_container->drawables.empty()) {
                drawWithTwoSidedLightingSupport(*drawable_container, VP1DrawSettings(true, color_overrides, VP1PrimitiveFilter::TRIANGLES));
                gGLFT->glDisable(GL_LIGHTING);
                drawable_container->draw(VP1DrawSettings(true, color_overrides, ~VP1PrimitiveFilter::TRIANGLES));
                gGLFT->glEnable(GL_LIGHTING);
            } else {
                drawBoundingBox( request, view );
            }
            break;
        }
    }
}

void VP1DrawableContainer::draw(const VP1DrawSettings& draw_settings) const
{
    static MGLFunctionTable *gGLFT = NULL;
    if (gGLFT == NULL)
        gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();

    for (size_t i = 0; i < drawables.size(); ++i) {
        const auto& drawable = drawables[i];

        if (!draw_settings.primitive_filter.has(drawable.buffer.primType()))
            continue;

        gGLFT->glPushMatrix();
        // Get the current matrix.
        MGLdouble currentMatrix[16];
        gGLFT->glGetDoublev(MGL_MODELVIEW_MATRIX, currentMatrix);

        // Basically, we want to load our matrix into the thingy.
        // We don't use the OpenGL transform stack because we have
        // deep deep hierarchy that exhausts the max stack depth quickly.
        gGLFT->glMatrixMode(MGL_MODELVIEW);
        static_assert(sizeof(MGLfloat) == sizeof(drawable.world_matrix[0][0]),
            "matrix element size mismatch");
        gGLFT->glMultMatrixf((const MGLfloat *)&drawable.world_matrix[0][0]);

        C3f color(0.7f, 0.7f, 0.7f);
        const auto& color_overrides = draw_settings.color_overrides;
        auto color_override_it = color_overrides.find(drawable.drawable_id);
        if (color_override_it != color_overrides.end()) {
            color = color_override_it->second.diffuse_color;
        }

        if (draw_settings.override_color) {
            gGLFT->glColor4f(color.x, color.y, color.z, 1.0f);
        }

        drawable.buffer.render(draw_settings.flip_normals);

        // And back out, restore the matrix.
        gGLFT->glMatrixMode(MGL_MODELVIEW);
        gGLFT->glLoadMatrixd(currentMatrix);
        gGLFT->glPopMatrix();
    }
}

void CAlembicHolderUI::drawWithTwoSidedLightingSupport(
    const VP1DrawableContainer& drawable_container,
    VP1DrawSettings draw_settings) const
{
    gGLFT->glPushMatrix();

    glEnable(MGL_POLYGON_OFFSET_FILL);  // Viewport has set the offset, just enable it

    glColorMaterial(MGL_FRONT_AND_BACK, MGL_AMBIENT_AND_DIFFUSE);
    glEnable(MGL_COLOR_MATERIAL) ;
    glColor4f(.7, .7, .7, 1.0);
    // On Geforce cards, we emulate two-sided lighting by drawing
    // triangles twice because two-sided lighting is 10 times
    // slower than single-sided lighting.

    bool needEmulateTwoSidedLighting = false;
    // Query face-culling and two-sided lighting state
    bool  cullFace = (gGLFT->glIsEnabled(MGL_CULL_FACE) == MGL_TRUE);
    MGLint twoSidedLighting = MGL_FALSE;
    gGLFT->glGetIntegerv(MGL_LIGHT_MODEL_TWO_SIDE, &twoSidedLighting);

    needEmulateTwoSidedLighting = (!cullFace && twoSidedLighting);

    if(needEmulateTwoSidedLighting)
    {
        glEnable(MGL_CULL_FACE);
        glLightModeli(MGL_LIGHT_MODEL_TWO_SIDE, 0);
        glCullFace(MGL_FRONT);
        draw_settings.flip_normals = false;
        drawable_container.draw(draw_settings);
        glCullFace(MGL_BACK);
        draw_settings.flip_normals = true;
        drawable_container.draw(draw_settings);
        gGLFT->glLightModeli(MGL_LIGHT_MODEL_TWO_SIDE, 1);
        glDisable(MGL_CULL_FACE);
    }
    else
        drawable_container.draw(draw_settings);

    glDisable(MGL_POLYGON_OFFSET_FILL);
    glFrontFace(MGL_CCW);

    gGLFT->glPopMatrix();
}

void CAlembicHolderUI::drawBoundingBox( const MDrawRequest & request,
                                                          M3dView & view ) const
//
// Description:
//
//     Bounding box drawing routine
//
// Arguments:
//
//     request - request to be drawn
//     view    - view to draw into
//
{
        // Get the surface shape
        MPxSurfaceShape *shape = surfaceShape();
        if ( !shape )
                return;

        // Get the bounding box
        MBoundingBox box = shape->boundingBox();
    float w = (float) box.width();
    float h = (float) box.height();
    float d = (float) box.depth();

    {
        // Query current state so it can be restored
        //
        bool lightingWasOn = glIsEnabled( MGL_LIGHTING ) == MGL_TRUE;

        // Setup the OpenGL state as necessary
        //
        if ( lightingWasOn ) {
            glDisable( MGL_LIGHTING );
        }

        glEnable( MGL_LINE_STIPPLE );
        glLineStipple(1, LINE_STIPPLE_SHORTDASHED);

        // Below we just two sides and then connect
        // the edges together

        MPoint minVertex = box.min();

        request.color();

        // Draw first side
        glBegin( MGL_LINE_LOOP );
        MPoint vertex = minVertex;
        glVertex3f( (float)vertex[0],   (float)vertex[1],   (float)vertex[2] );
        glVertex3f( (float)vertex[0]+w, (float)vertex[1],   (float)vertex[2] );
        glVertex3f( (float)vertex[0]+w, (float)vertex[1]+h, (float)vertex[2] );
        glVertex3f( (float)vertex[0],   (float)vertex[1]+h, (float)vertex[2] );
        glVertex3f( (float)vertex[0],   (float)vertex[1],   (float)vertex[2] );
        glEnd();

        // Draw second side
        MPoint sideFactor(0,0,d);
        MPoint vertex2 = minVertex + sideFactor;
        glBegin( MGL_LINE_LOOP );
        glVertex3f( (float)vertex2[0],   (float)vertex2[1],   (float)vertex2[2] );
        glVertex3f( (float)vertex2[0]+w, (float)vertex2[1],   (float)vertex2[2] );
        glVertex3f( (float)vertex2[0]+w, (float)vertex2[1]+h, (float)vertex2[2] );
        glVertex3f( (float)vertex2[0],   (float)vertex2[1]+h, (float)vertex2[2] );
        glVertex3f( (float)vertex2[0],   (float)vertex2[1],   (float)vertex2[2] );
        glEnd();

        // Connect the edges together
        glBegin( MGL_LINES );
        glVertex3f( (float)vertex2[0],   (float)vertex2[1],   (float)vertex2[2] );
        glVertex3f( (float)vertex[0],   (float)vertex[1],   (float)vertex[2] );

        glVertex3f( (float)vertex2[0]+w,   (float)vertex2[1],   (float)vertex2[2] );
        glVertex3f( (float)vertex[0]+w,   (float)vertex[1],   (float)vertex[2] );

        glVertex3f( (float)vertex2[0]+w,   (float)vertex2[1]+h,   (float)vertex2[2] );
        glVertex3f( (float)vertex[0]+w,   (float)vertex[1]+h,   (float)vertex[2] );

        glVertex3f( (float)vertex2[0],   (float)vertex2[1]+h,   (float)vertex2[2] );
        glVertex3f( (float)vertex[0],   (float)vertex[1]+h,   (float)vertex[2] );
        glEnd();

        // Restore the state
        //
        if ( lightingWasOn ) {
            glEnable( MGL_LIGHTING );
        }

        glDisable( MGL_LINE_STIPPLE );
    }
}

void CAlembicHolderUI::getDrawRequestsWireFrame(MDrawRequest& request,
        const MDrawInfo& info) {

    request.setToken(kDrawWireframe);
    M3dView::DisplayStatus displayStatus = info.displayStatus();
    M3dView::ColorTable activeColorTable = M3dView::kActiveColors;
    M3dView::ColorTable dormantColorTable = M3dView::kDormantColors;
    switch (displayStatus) {
    case M3dView::kLead:
        request.setColor(LEAD_COLOR, activeColorTable);
        break;
    case M3dView::kActive:
        request.setColor(ACTIVE_COLOR, activeColorTable);
        break;
    case M3dView::kActiveAffected:
        request.setColor(ACTIVE_AFFECTED_COLOR, activeColorTable);
        break;
    case M3dView::kDormant:
        request.setColor(DORMANT_COLOR, dormantColorTable);
        break;
    case M3dView::kHilite:
        request.setColor(HILITE_COLOR, activeColorTable);
        break;
    default:
        break;
    }

}

void CAlembicHolderUI::getDrawRequestsBoundingBox(MDrawRequest& request,
        const MDrawInfo& info) {

    request.setToken(kDrawBoundingBox);
    M3dView::DisplayStatus displayStatus = info.displayStatus();
    M3dView::ColorTable activeColorTable = M3dView::kActiveColors;
    M3dView::ColorTable dormantColorTable = M3dView::kDormantColors;
    switch (displayStatus) {
    case M3dView::kLead:
        request.setColor(LEAD_COLOR, activeColorTable);
        break;
    case M3dView::kActive:
        request.setColor(ACTIVE_COLOR, activeColorTable);
        break;
    case M3dView::kActiveAffected:
        request.setColor(ACTIVE_AFFECTED_COLOR, activeColorTable);
        break;
    case M3dView::kDormant:
        request.setColor(DORMANT_COLOR, dormantColorTable);
        break;
    case M3dView::kHilite:
        request.setColor(HILITE_COLOR, activeColorTable);
        break;
    default:
        break;
    }

}


void CAlembicHolderUI::getDrawRequestsShaded(MDrawRequest& request,
        const MDrawInfo& info, MDrawRequestQueue& queue, MDrawData& data)
{
        // Need to get the material info
        //
        MDagPath path = info.multiPath();       // path to your dag object
        M3dView view = info.view();;            // view to draw to
        MMaterial material = MPxSurfaceShapeUI::material( path );
        M3dView::DisplayStatus displayStatus = info.displayStatus();

        // Evaluate the material and if necessary, the texture.
        //
        if ( ! material.evaluateMaterial( view, path ) ) {
                cerr << "Couldnt evaluate\n";
        }

        bool drawTexture = true;
        if ( drawTexture && material.materialIsTextured() ) {
                material.evaluateTexture( data );
        }

        request.setMaterial( material );

        bool materialTransparent = false;
        material.getHasTransparency( materialTransparent );
        if ( materialTransparent ) {
                request.setIsTransparent( true );
        }

        // create a draw request for wireframe on shaded if
        // necessary.
        //
        if ( (displayStatus == M3dView::kActive) ||
                 (displayStatus == M3dView::kLead) ||
                 (displayStatus == M3dView::kHilite) )
        {
                MDrawRequest wireRequest = info.getPrototype( *this );
                wireRequest.setDrawData( data );
                getDrawRequestsWireFrame( wireRequest, info );
                wireRequest.setToken( kDrawWireframeOnShaded );
                wireRequest.setDisplayStyle( M3dView::kWireFrame );
                wireRequest.setColor(MHWRender::MGeometryUtilities::wireframeColor(path));
                queue.add( wireRequest );
        }
}

//
// Returns the point in world space corresponding to a given
// depth. The depth is specified as 0.0 for the near clipping plane and
// 1.0 for the far clipping plane.
//
MPoint CAlembicHolderUI::getPointAtDepth(
    MSelectInfo &selectInfo,
    double     depth) const
{
    MDagPath cameraPath;
    M3dView view = selectInfo.view();

    view.getCamera(cameraPath);
    MStatus status;
    MFnCamera camera(cameraPath, &status);

    // Ortho cam maps [0,1] to [near,far] linearly
    // persp cam has non linear z:
    //
    //        fp np
    // -------------------
    // 1. fp - d fp + d np
    //
    // Maps [0,1] -> [np,fp]. Then using linear mapping to get back to
    // [0,1] gives.
    //
    //       d np
    // ----------------  for linear mapped distance
    // fp - d fp + d np

    if (!camera.isOrtho())
    {
        double np = camera.nearClippingPlane();
        double fp = camera.farClippingPlane();

        depth *= np / (fp - depth * (fp - np));
    }

    MPoint     cursor;
    MVector rayVector;
    selectInfo.getLocalRay(cursor, rayVector);
    cursor = cursor * selectInfo.multiPath().inclusiveMatrix();
    short x,y;
    view.worldToView(cursor, x, y);

    MPoint res, neardb, fardb;
    view.viewToWorld(x,y, neardb, fardb);
    res = neardb + depth*(fardb-neardb);

    return res;
}


bool CAlembicHolderUI::select(MSelectInfo &selectInfo,
        MSelectionList &selectionList, MPointArray &worldSpaceSelectPts) const
{
    nozAlembicHolder* shapeNode = (nozAlembicHolder*) surfaceShape();
    auto sample = shapeNode->getSample();
    if (sample.empty())
        return false;

    MSelectionMask mask("alembicHolder");
    if (!selectInfo.selectable(mask)){
        return false;
    }

    const bool boundingboxSelection =
        (M3dView::kBoundingBox == selectInfo.displayStyle() ||
         !selectInfo.singleSelection());


    if (boundingboxSelection)
    {
        // We hit the bounding box, so we want the object?
        MSelectionList item;
        item.add(selectInfo.selectPath());
        MPoint xformedPt;
        selectInfo.addSelection(item, xformedPt, selectionList,
                worldSpaceSelectPts, mask, false);
        return true;
    }

    GLfloat minZ;
    Select* selector;

    size_t numTriangles = sample.hierarchy_stat.triangle_count;

    if (numTriangles < 1024)
        selector = new GLPickingSelect(selectInfo);
    else
        selector = new RasterSelect(selectInfo);

    selector->processTriangles(m_vp1drawables, numTriangles);

    selector->end();
    minZ = selector->minZ();
    delete selector;


    bool selected = (minZ <= 1.0f);
    if ( selected )
    {
        // Add the selected item to the selection list

        MSelectionList selectionItem;
        {
            MDagPath path = selectInfo.multiPath();
            MStatus lStatus = path.pop();
            while (lStatus == MStatus::kSuccess)
            {
                if (path.hasFn(MFn::kTransform))
                {
                    break;
                }
                else
                {
                    lStatus = path.pop();
                }
            }
            selectionItem.add(path);
        }

        MPoint worldSpaceselectionPoint =
            getPointAtDepth(selectInfo, minZ);

        selectInfo.addSelection(
            selectionItem,
            worldSpaceselectionPoint,
            selectionList, worldSpaceSelectPts,
            mask, false );
    }

    return selected;

}

} // namespace AlembicHolder
