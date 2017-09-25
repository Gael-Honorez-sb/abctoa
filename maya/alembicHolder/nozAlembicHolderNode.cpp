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


#include <maya/MCommandResult.h>

#include "nozAlembicHolderNode.h"
#include "parseJsonShaders.h"

#include "gpuCacheConfig.h"
#include "gpuCacheDrawTraversal.h"
#include "gpuCacheFrustum.h"
#include "gpuCacheGLFT.h"
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
#include <maya/MFnEnumAttribute.h>
#include <maya/MFileObject.h>
#include <maya/MSelectionList.h>
#include <maya/MRenderView.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MFileObject.h>
#include <maya/MObjectArray.h>
#include <maya/MStringResource.h>
#include <maya/MStringResourceId.h>
#include <maya/MEventMessage.h>

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

#define LEAD_COLOR                18    // green
#define ACTIVE_COLOR            15    // white
#define ACTIVE_AFFECTED_COLOR    8    // purple
#define DORMANT_COLOR            4    // blue
#define HILITE_COLOR            17    // pale blue
//#include "timer.h"

//#include "boost/foreach.hpp"

typedef std::map<std::string, GLuint> NodeCache;

NodeCache g_bboxCache;


// The id is a 32bit value used to identify this type of node in the binary file format.
MTypeId nozAlembicHolder::id(0x00114956);

MObject nozAlembicHolder::aAbcFiles;
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

MObject nozAlembicHolder::aTextureResolution;

AlembicHolder::SceneState CAlembicDatas::abcSceneState;
AlembicHolder::SceneManager CAlembicDatas::abcSceneManager;

CAlembicDatas::CAlembicDatas() {

    bbox = MBoundingBox(MPoint(-1.0f, -1.0f, -1.0f), MPoint(1.0f, 1.0f, 1.0f));
    token = 0;
    m_bbextendedmode = false;
    m_currscenekey = "";
	m_currselectionkey = "";
    m_abcdirty = false;
	m_params = new holderPrms();
	m_params->linkAttributes = false;
}

void nodePreRemovalCallback(MObject& obj, void* data) 
{
    std::cout << "Pre Removal callback" << std::endl;
    std::string sceneKey = ((nozAlembicHolder*)data)->getSceneKey();
}

nozAlembicHolder::nozAlembicHolder()
    : fCacheVersion(0)
    , fAssignmentVersion(0)
{
}

nozAlembicHolder::~nozAlembicHolder()
{
    std::cout << "removint alembic Holder" << std::endl;
    std::string sceneKey = getSceneKey();
    CAlembicDatas::abcSceneManager.removeScene(sceneKey);
    if(CAlembicDatas::abcSceneManager.hasKey(sceneKey) == 0)
    {
        NodeCache::iterator J = g_bboxCache.find(sceneKey);
        if (J != g_bboxCache.end())
        {
            glDeleteLists((*J).second, 1);
            g_bboxCache.erase(J);
        }
    }
    std::cout << "removint alembic Holder done" << std::endl;
}

void nozAlembicHolder::setHolderTime() {

    CAlembicDatas* geom = alembicData();

    if(geom != NULL)
    {
        MFnDagNode fn(thisMObject());
        MTime time, timeOffset;

        MPlug plug = fn.findPlug(aTime);
        plug.getValue(time);
        plug = fn.findPlug(aTimeOffset);
        plug.getValue(timeOffset);

        double dtime;

        dtime = time.as(MTime::kSeconds) + timeOffset.as(MTime::kSeconds);
		
		geom->time = dtime;

        std::string sceneKey = getSceneKey();
        if (geom->abcSceneManager.hasKey(sceneKey))
            geom->abcSceneManager.getScene(sceneKey)->setTime(dtime );
    }
}


bool nozAlembicHolder::isBounded() const
{
    return true;
}



MBoundingBox nozAlembicHolder::boundingBox() const
{
	const CAlembicDatas* geom = alembicData();
    MBoundingBox bbox = MBoundingBox(MPoint(-1.0f, -1.0f, -1.0f), MPoint(1.0f, 1.0f, 1.0f));

    if(geom != NULL)
        bbox = geom->bbox;

    return bbox;
}

void* nozAlembicHolder::creator()
{
    return new nozAlembicHolder();
}

void nozAlembicHolder::postConstructor()
{
    // This call allows the shape to have shading groups assigned
    setRenderable(true);
    isConstant = false;
    // callbacks
    MObject node = thisMObject();
    MNodeMessage::addNodePreRemovalCallback(node, nodePreRemovalCallback, this);


}

void nozAlembicHolder::copyInternalData(MPxNode* srcNode) {
    // here we ensure that the scene manager stays up to date when duplicating nodes

    const nozAlembicHolder& node = *(nozAlembicHolder*)srcNode;

    CAlembicDatas* geom = alembicData();

    MFnDagNode fn(node.thisMObject());
    MStringArray abcfiles;
    MPlug plug = fn.findPlug(aAbcFiles);

    std::vector<std::string> fileNames;

    for (unsigned int i = 0; i <plug.numElements(); i++)
    {
        MPlug fileName = plug[i];
        MString filename;
        fileName.getValue(filename);
        fileNames.push_back(filename.asChar());
    }

    MString objectPath;
    plug = fn.findPlug(aObjectPath);
    plug.getValue(objectPath);

    MString selectionPath;
    plug = fn.findPlug(aSelectionPath);
    plug.getValue(selectionPath);

    bool extendedMode = false;
    plug = fn.findPlug(aBoundingExtended);
    plug.getValue(extendedMode);

    if(geom != NULL)
    {
        geom->abcSceneManager.addScene(fileNames, objectPath.asChar());
        geom->m_currscenekey = getSceneKey();
		geom->m_currselectionkey = getSelectionKey();
        geom->m_bbextendedmode = extendedMode;
    }

}

MStatus nozAlembicHolder::initialize() {
    // init maya stuff

    MFnTypedAttribute tAttr;
    MFnNumericAttribute nAttr;
    MFnMessageAttribute mAttr;
    MFnUnitAttribute uAttr;
    MStatus stat;

    aAbcFiles = tAttr.create("cacheFileNames", "cfn", MFnData::kString);
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);
    tAttr.setArray(true);

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

    MFnEnumAttribute enumAttrFn;
    aTextureResolution = enumAttrFn.create("textureResolution", "txr", 0, &stat);
    CHECK_MSTATUS(enumAttrFn.addField("Original", 0));
    CHECK_MSTATUS(enumAttrFn.addField("128", 128));
    CHECK_MSTATUS(enumAttrFn.addField("256", 256));
    CHECK_MSTATUS(enumAttrFn.addField("512", 512));
    CHECK_MSTATUS(enumAttrFn.addField("1024", 1024));
    CHECK_MSTATUS(enumAttrFn.addField("2048", 2048));
    enumAttrFn.setInternal(true);
    addAttribute(aTextureResolution);

    // Add the attributes we have created to the node
    //
    addAttribute(aAbcFiles);
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

    attributeAffects(aAbcFiles, aUpdateCache);
    attributeAffects(aTime, aUpdateCache);
    attributeAffects(aTimeOffset, aUpdateCache);

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

MStatus nozAlembicHolder::setDependentsDirty(const MPlug& plug, MPlugArray& plugArray)
{

    if (plug != aForceReload && plug != aBoundingExtended && plug != aObjectPath
        && plug != aSelectionPath && plug != aTime && plug != aTimeOffset && plug != aUpdateCache)
        return MPxNode::setDependentsDirty(plug, plugArray);


    if ((plug == aTime || plug == aTimeOffset) && isConstant)
        return MPxNode::setDependentsDirty(plug, plugArray);

    if (plug == aUpdateCache)
    {
        MObjectArray objArray;
        objArray.append(aBoundMin);
        objArray.append(aBoundMax);
        for (unsigned i(0), numObjects = objArray.length(); i < numObjects; i++)
        {
            MPlug plug(thisMObject(), objArray[i]);
            plugArray.append(plug);
        }
    }
    else
    {
        MPlug plug(thisMObject(), aUpdateCache);
        plugArray.append(plug);
    }
    return MPxNode::setDependentsDirty(plug, plugArray);
}

std::string nozAlembicHolder::getSelectionKey() const {
    MFnDagNode fn(thisMObject());
    MString selectionPath;
    MPlug plug = fn.findPlug(aSelectionPath);
    plug.getValue(selectionPath);
    return std::string(selectionPath.asChar());
}

std::string nozAlembicHolder::getSceneKey() const {
    MFnDagNode fn(thisMObject());

    MPlug plug = fn.findPlug(aAbcFiles);
    std::string key;
    for (unsigned int i = 0; i <plug.numElements(); i++)
    {
        MPlug fileName = plug[i];
        MString filename;
        fileName.getValue(filename);
        MFileObject fileObject;
        fileObject.setRawFullName(filename.expandFilePath());
        fileObject.setResolveMethod(MFileObject::kInputFile);
        std::string abcFile = fileObject.resolvedFullName().asChar();
        key += abcFile + "|";
    }

    MString objectPath;
    plug = fn.findPlug(aObjectPath);
    plug.getValue(objectPath);

    key += std::string(objectPath.asChar());

    return key;
}
MStatus nozAlembicHolder::compute(const MPlug& plug, MDataBlock& block)
{
	if (plug == aUpdateAssign)
	{
        // Increment the value of aUpdateAssign which acts as a version number.
        fAssignmentVersion += 1;
        block.outputValue(aUpdateAssign).setInt(fAssignmentVersion);

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
		
		fGeometry.m_params->attributes.clear();

		if( jrootattributes.size() > 0 )
		{
			bool addtoPath = false;
			fGeometry.m_params->linkAttributes = true;
			fGeometry.m_params->attributesRoot = jrootattributes;
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
					fGeometry.m_params->attributes.push_back(path);

			}
			
			std::sort(fGeometry.m_params->attributes.begin(), fGeometry.m_params->attributes.end());
			
		}
		else
		{
			fGeometry.m_params->linkAttributes = false;
		}

		fGeometry.m_abcdirty = true;
		block.outputValue(aForceReload).setBool(false);

	}

	else if (plug == aUpdateCache)
    {
        // Increment the value of aUpdateCache which acts as a version number.
        fCacheVersion += 1;
        block.outputValue(aUpdateCache).setInt(fCacheVersion);

        MStatus status;
        MFnDagNode fn(thisMObject());

        // Try to parse JSON Shader assignation here.
        MPlug shaderAssignation = fn.findPlug("shadersAssignation", &status);

        if(status == MS::kSuccess)
        {
            bool parsingSuccessful = false;
            MString jsonAssign = block.inputValue(shaderAssignation).asString();
            if(jsonAssign != "")
            {
                Json::Value jroot;
                Json::Reader reader;
                parsingSuccessful = reader.parse( jsonAssign.asChar(), jroot, false );
                if(parsingSuccessful)
                    ParseShaders(jroot, fGeometry.m_params->shaderColors);
            }
        }

        unsigned count = block.inputArrayValue(aAbcFiles).elementCount();;

        MArrayDataHandle fileArrayHandle = block.inputArrayValue(aAbcFiles);

        std::string key;
        std::vector <std::string> files;
        for (unsigned i = 0; i < count; i++)
        {
            fileArrayHandle.jumpToArrayElement(i);
            MDataHandle InputElement = fileArrayHandle.inputValue(&status);

            MFileObject fileObject;
            fileObject.setRawFullName(InputElement.asString().expandFilePath());
            fileObject.setResolveMethod(MFileObject::kInputFile);
            std::string nameResolved = std::string(fileObject.resolvedFullName().asChar());
            files.push_back(nameResolved);
            key += nameResolved + "|";
        }

        MString objectPath = block.inputValue(aObjectPath).asString();
        MString selectionPath = block.inputValue(aSelectionPath).asString();

		fGeometry.m_currselectionkey = selectionPath.asChar();

        MTime time = block.inputValue(aTime).asTime() + block.inputValue(aTimeOffset).asTime();

        fGeometry.m_bbextendedmode = block.inputValue(aBoundingExtended).asBool();

        bool hasToReload = false;

        if (fGeometry.time != time.value())
        {
            fGeometry.m_abcdirty = true;
			fGeometry.time = time.value();
            hasToReload = true;
        }

        key += objectPath.asChar();

        if (fGeometry.m_currscenekey != key || hasToReload)
        {
            if (fGeometry.m_currscenekey != key)
            {
                CAlembicDatas::abcSceneManager.removeScene(fGeometry.m_currscenekey);
                if(CAlembicDatas::abcSceneManager.hasKey(fGeometry.m_currscenekey) == 0)
                {
                    NodeCache::iterator J = g_bboxCache.find(fGeometry.m_currscenekey);
                    if (J != g_bboxCache.end())
                    {
                        glDeleteLists((*J).second, 1);
                        g_bboxCache.erase(J);
                    }

                }


                fGeometry.m_currscenekey = "";
                CAlembicDatas::abcSceneManager.addScene(files, objectPath.asChar());
                fGeometry.m_currscenekey = key;
            }

            if (CAlembicDatas::abcSceneManager.hasKey(key))
            {
                fGeometry.bbox = MBoundingBox();
                AlembicHolder::Box3d bb;
                setHolderTime();
                bb = CAlembicDatas::abcSceneManager.getScene(key)->getBounds();
                fGeometry.bbox.expand(MPoint(bb.min.x, bb.min.y, bb.min.z));
                fGeometry.bbox.expand(MPoint(bb.max.x, bb.max.y, bb.max.z));


				block.outputValue(aBoundMin).set3Float(float(bb.min.x), float(bb.min.y), float(bb.min.z));
				block.outputValue(aBoundMax).set3Float(float(bb.max.x), float(bb.max.y), float(bb.max.z));


                // notify viewport 2.0 that we are dirty
                MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());

            }
            fGeometry.m_abcdirty = true;
            if(fGeometry.abcSceneManager.hasKey(fGeometry.m_currscenekey))
                isConstant = fGeometry.abcSceneManager.getScene(fGeometry.m_currscenekey)->isConstant();

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

bool nozAlembicHolder::getInternalValueInContext(const MPlug & plug, MDataHandle & dataHandle, MDGContext & ctx)
{
    if (plug == aTextureResolution) {
        dataHandle.setShort(textRes);
        return true;
    }

    return MPxNode::getInternalValueInContext(plug, dataHandle, ctx);
}

bool nozAlembicHolder::setInternalValueInContext(const MPlug & plug, const MDataHandle & dataHandle, MDGContext & ctx)
{
    if (plug == aTextureResolution) {
        short newRes = dataHandle.asShort();
        if (newRes != textRes){
            textRes = newRes;

            for (std::set<MString, MStringComp>::iterator it = ownTextures.begin(); it != ownTextures.end(); ++it)
            {
                textureMap.erase((*it));
            }
            ownTextures.clear();
        }
        return true;
    }

    return MPxNode::setInternalValueInContext(plug, dataHandle, ctx);
}

CAlembicDatas* nozAlembicHolder::alembicData()
{
    if (MRenderView::doesRenderEditorExist())
    {
        return &fGeometry;
    }
    return NULL;
}

AlembicHolder::DrawablePtr nozAlembicHolder::getGeometry() const
{
    auto cache = alembicData();
    if (!cache)
        return nullptr;

    const auto& key = cache->m_currscenekey;
    if (!cache->abcSceneManager.hasKey(key))
        return nullptr;

    const auto& scene = cache->abcSceneManager.getScene(key);
    if (!scene)
        return nullptr;

    return scene->getDrawable();
}

AlembicHolder::MaterialGraphMap::Ptr nozAlembicHolder::getMaterial() const
{
    const CAlembicDatas* cache = alembicData();
    if (!cache)
        return nullptr;

    const auto& key = cache->m_currscenekey;
    if (!cache->abcSceneManager.hasKey(key))
        return nullptr;

    const auto& scene = cache->abcSceneManager.getScene(key);
    if (!scene)
        return nullptr;

    return scene->getMaterials();
}


// UI IMPLEMENTATION

using namespace AlembicHolder;

//==========================================================================
// CLASS NbPrimitivesVisitor
//==========================================================================

class NbPrimitivesVisitor : public ::AlembicHolder::DrawableVisitor
{
public:

    NbPrimitivesVisitor(double seconds)
        : fSeconds(seconds),
          fNumWires(0),
          fNumTriangles(0)
    {}

    size_t numWires()       { return fNumWires; }
    size_t numTriangles()   { return fNumTriangles; }

    virtual void visit(const IXformDrw&   xform)
    {
        // Recurse into children sub nodes. Expand all instances.
        BOOST_FOREACH(const auto& child, xform.getChildren() ) {
            child->accept(*this);
        }
    }

    virtual void visit(const IPolyMeshDrw&   shape)
    {
        const std::shared_ptr<const ShapeSample>& sample =
            shape.getSample(fSeconds);
        if (!sample) return;

        fNumWires       += sample->numWires();
        fNumTriangles   += sample->numTriangles();
    }

private:

    const double    fSeconds;
    size_t          fNumWires;
    size_t          fNumTriangles;
};


//==========================================================================
// CLASS SnapTraversal
//==========================================================================

class SnapTraversalState : public DrawTraversalState
{
public:

    SnapTraversalState(const Frustum&  frustrum,
        const double    seconds,
        const MMatrix&  localToPort,
        const MMatrix&  inclusiveMatrix,
        MSelectInfo&    snapInfo)
        : DrawTraversalState(frustrum, seconds, kPruneNone),
        fLocalToPort(localToPort),
        fInclusiveMatrix(inclusiveMatrix),
        fSnapInfo(snapInfo),
        fSelected(false)
    {}

    const MMatrix& localToPort() const { return fLocalToPort; }
    const MMatrix& inclusiveMatrix() const { return fInclusiveMatrix; }
    MSelectInfo& snapInfo() { return fSnapInfo; }

    bool selected() const { return fSelected; }
    void setSelected() { fSelected = true; }

private:
    const MMatrix   fLocalToPort;
    const MMatrix   fInclusiveMatrix;
    MSelectInfo&    fSnapInfo;
    bool            fSelected;
};


class SnapTraversal
    : public DrawTraversal<SnapTraversal, SnapTraversalState>
{
public:

    typedef DrawTraversal<SnapTraversal, SnapTraversalState> BaseClass;

    SnapTraversal(
        SnapTraversalState&     state,
        const MMatrix&          xform,
        bool                    isReflection,
        Frustum::ClippingResult parentClippingResult)
        : BaseClass(state, xform, false, parentClippingResult)
    {}

    void draw(const std::shared_ptr<const ShapeSample>& sample)
    {
        if (!sample->visibility()) return;
        if (sample->isBoundingBoxPlaceHolder()) return;

        assert(sample->positions());
        VertexBuffer::ReadInterfacePtr readable = sample->positions()->readableInterface();
        const float* const positions = readable->get();

        unsigned int srx, sry, srw, srh;
        state().snapInfo().selectRect(srx, sry, srw, srh);
        double srxl = srx;
        double sryl = sry;
        double srxh = srx + srw;
        double sryh = sry + srh;

        const MMatrix localToPort = xform() * state().localToPort();
        const MMatrix inclusiveMatrix = xform() * state().inclusiveMatrix();

        // Loop through all vertices of the mesh.
        // See if they lie withing the view frustum,
        // then send them to snapping check.
        size_t numVertices = sample->numVerts();
        for (size_t vertexIndex = 0; vertexIndex<numVertices; vertexIndex++)
        {
            const float* const currentPoint = &positions[vertexIndex * 3];

            // Find the closest snapping point using the CPU. This is
            // faster than trying to use OpenGL picking.
            MPoint loPt(currentPoint[0], currentPoint[1], currentPoint[2]);
            MPoint pt = loPt * localToPort;
            pt.rationalize();

            if (pt.x >= srxl && pt.x <= srxh &&
                pt.y >= sryl && pt.y <= sryh &&
                pt.z >= 0.0  && pt.z <= 1.0) {
                MPoint wsPt = loPt * inclusiveMatrix;
                wsPt.rationalize();
                state().snapInfo().setSnapPoint(wsPt);
                state().setSelected();
            }
        }
    }
};


//==============================================================================
// CLASS ShapeUI
//==============================================================================

#define kPluginId "alembicHolder"
#define kEvaluateMaterialErrorMsg MStringResourceId(kPluginId, "kEvaluateMaterialErrorMsg",\
                "Couldn't evaluate material\n")


void* CAlembicHolderUI::creator()
{
    return new CAlembicHolderUI;
}

CAlembicHolderUI::CAlembicHolderUI()
{
    std::cout << "CAlembicHolderUI constructor" << std::endl;
}

CAlembicHolderUI::~CAlembicHolderUI()
{
    std::cout << "CAlembicHolderUI destructor" << std::endl;
}

void CAlembicHolderUI::getDrawRequests(const MDrawInfo & info,
    bool objectAndActiveOnly,
    MDrawRequestQueue & queue)
{
    // Get the data necessary to draw the shape
    MDrawData data;
    getDrawData(0, data);

    // Decode the draw info and determine what needs to be drawn
    M3dView::DisplayStyle  appearance = info.displayStyle();
    M3dView::DisplayStatus displayStatus = info.displayStatus();

    // Are we displaying gpuCache?
    if (!info.pluginObjectDisplayStatus(Config::kDisplayFilter)) {
        return;
    }

    MDagPath path = info.multiPath();

    switch (appearance)
    {
    case M3dView::kBoundingBox:
    {
        MDrawRequest request = info.getPrototype(*this);
        request.setDrawData(data);
        request.setToken(kBoundingBox);

        MColor wireframeColor = MHWRender::MGeometryUtilities::wireframeColor(path);
        request.setColor(wireframeColor);

        queue.add(request);
    }break;

    case M3dView::kWireFrame:
    {
        MDrawRequest request = info.getPrototype(*this);
        request.setDrawData(data);
        request.setToken(kDrawWireframe);

        MColor wireframeColor = MHWRender::MGeometryUtilities::wireframeColor(path);
        request.setColor(wireframeColor);

        queue.add(request);
    } break;


    // All of these modes are interpreted as meaning smooth shaded
    // just as it is done in the viewport 2.0.
    case M3dView::kFlatShaded:
    case M3dView::kGouraudShaded:
    default:
    {
        nozAlembicHolder* node = (nozAlembicHolder*)surfaceShape();
        if (!node) break;
        const auto& geom = node->getGeometry();
        if (!geom) break;

        // Get the view to draw to
        M3dView view = info.view();

        const bool needWireframe = ((displayStatus == M3dView::kActive) ||
            (displayStatus == M3dView::kLead) ||
            (displayStatus == M3dView::kHilite) ||
            (view.wireframeOnShaded()));

        // When we need to draw both the shaded geometry and the
        // wireframe mesh, we need to offset the shaded geometry
        // in depth to avoid Z-fighting against the wireframe
        // mesh.
        //
        // On the hand, we don't want to use depth offset when
        // drawing only the shaded geometry because it leads to
        // some drawing artifacts. The reason is a litle bit
        // subtle. At silouhette edges, both front-facing and
        // back-facing faces are meeting. These faces can have a
        // different slope in Z and this can lead to a different
        // Z-offset being applied. When unlucky, the back-facing
        // face can be drawn in front of the front-facing face. If
        // two-sided lighting is enabled, the back-facing fragment
        // can have a different resultant color. This can lead to
        // a rim of either dark or bright pixels around silouhette
        // edges.
        //
        // When the wireframe mesh is drawn on top (even a dotted
        // one), it masks this effect sufficiently that it is no
        // longer distracting for the user, so it is OK to use
        // depth offset when the wireframe mesh is drawn on top.
        const DrawToken shadedDrawToken = needWireframe ?
            kDrawSmoothShadedDepthOffset : kDrawSmoothShaded;

        // Get the default material.
        //
        // Note that we will only use the material if the viewport
        // option "Use default material" has been selected. But,
        // we still need to set a material (even an unevaluated
        // one), so that the draw request is indentified as
        // drawing geometry instead of drawing the wireframe mesh.
        MMaterial material = MMaterial::defaultMaterial();

        if (view.usingDefaultMaterial()) {
            // Evaluate the material.
            if (!material.evaluateMaterial(view, path)) {
                MStatus stat;
                MString msg = MStringResource::getString(kEvaluateMaterialErrorMsg, stat);
                perror(msg.asChar());
            }

            // Create the smooth shaded draw request
            MDrawRequest request = info.getPrototype(*this);
            request.setDrawData(data);

            // This draw request will draw all sub nodes using an
            // opaque default material.
            request.setToken(shadedDrawToken);
            request.setIsTransparent(false);

            request.setMaterial(material);
            queue.add(request);
        }
        else if (view.xray()) {
            // Create the smooth shaded draw request
            MDrawRequest request = info.getPrototype(*this);
            request.setDrawData(data);

            // This draw request will draw all sub nodes using in X-Ray mode
            request.setToken(shadedDrawToken);
            request.setIsTransparent(true);

            request.setMaterial(material);
            queue.add(request);
        }
        else {
            // Opaque draw request
            //if (geom->transparentType() != SubNode::kTransparent)
            {
                // Create the smooth shaded draw request
                MDrawRequest request = info.getPrototype(*this);
                request.setDrawData(data);

                // This draw request will draw opaque sub nodes
                request.setToken(shadedDrawToken);

                request.setMaterial(material);
                queue.add(request);
            }

            // Transparent draw request
            /*
            if (geom->transparentType() != SubNode::kOpaque) {
            // Create the smooth shaded draw request
            MDrawRequest request = info.getPrototype( *this );
            request.setDrawData( data );

            // This draw request will draw transparent sub nodes
            request.setToken( shadedDrawToken );
            request.setIsTransparent( true );

            request.setMaterial( material );
            queue.add( request );
            }
            */
        }

        // create a draw request for wireframe on shaded if
        // necessary.
        if (needWireframe &&
            DisplayPref::wireframeOnShadedMode() != DisplayPref::kWireframeOnShadedNone)
        {
            MDrawRequest wireRequest = info.getPrototype(*this);
            wireRequest.setDrawData(data);
            wireRequest.setToken(kDrawWireframeOnShaded);
            wireRequest.setDisplayStyle(M3dView::kWireFrame);

            MColor wireframeColor = MHWRender::MGeometryUtilities::wireframeColor(path);
            wireRequest.setColor(wireframeColor);

            queue.add(wireRequest);
        }
    } break;
    }
}

void CAlembicHolderUI::draw(const MDrawRequest & request, M3dView & view) const
{
    // Initialize GL Function Table.
    InitializeGLFT();

    // Get the token from the draw request.
    // The token specifies what needs to be drawn.
    DrawToken token = DrawToken(request.token());

    switch (token)
    {
    case kBoundingBox:
        drawBoundingBox(request, view);
        break;

    case kDrawWireframe:
    case kDrawWireframeOnShaded:
        drawWireframe(request, view);
        break;

    case kDrawSmoothShaded:
        drawShaded(request, view, false);
        break;

    case kDrawSmoothShadedDepthOffset:
        drawShaded(request, view, true);
        break;
    }
}

void CAlembicHolderUI::drawBoundingBox(const MDrawRequest & request, M3dView & view) const
{
    // Get the surface shape
    auto node = (nozAlembicHolder*)surfaceShape();
    if (!node) return;

    // Get the bounding box
    MBoundingBox box = node->boundingBox();

    view.beginGL();
    {
        // Query current state so it can be restored
        //
        bool lightingWasOn = gGLFT->glIsEnabled(MGL_LIGHTING) == MGL_TRUE;

        // Setup the OpenGL state as necessary
        //
        if (lightingWasOn) {
            gGLFT->glDisable(MGL_LIGHTING);
        }

        gGLFT->glEnable(MGL_LINE_STIPPLE);
        gGLFT->glLineStipple(1, Config::kLineStippleShortDashed);

        VBOProxy vboProxy;
        vboProxy.drawBoundingBox(box);

        // Restore the state
        //
        if (lightingWasOn) {
            gGLFT->glEnable(MGL_LIGHTING);
        }

        gGLFT->glDisable(MGL_LINE_STIPPLE);
    }
    view.endGL();
}


namespace {

//==========================================================================
// CLASS DrawWireframeTraversal
//==========================================================================

class DrawWireframeState : public DrawTraversalState
{
public:
    DrawWireframeState(
        const Frustum&  frustrum,
        const double    seconds)
        : DrawTraversalState(frustrum, seconds, kPruneNone)
    {}
};

class DrawWireframeTraversal
    : public DrawTraversal<DrawWireframeTraversal, DrawWireframeState>
{
public:

    typedef DrawTraversal<DrawWireframeTraversal, DrawWireframeState> BaseClass;

    DrawWireframeTraversal(
        DrawWireframeState&     state,
        const MMatrix&          xform,
        bool                    isReflection,
        Frustum::ClippingResult parentClippingResult)
        : BaseClass(state, xform, isReflection, parentClippingResult)
    {}

    void draw(const std::shared_ptr<const ShapeSample>& sample)
    {
        if (!sample->visibility()) return;
        gGLFT->glLoadMatrixd(xform().matrix[0]);

        if (sample->isBoundingBoxPlaceHolder()) {
            state().vboProxy().drawBoundingBox(sample);
            //GlobalReaderCache::theCache().hintShapeReadOrder(subNode());
            return;
        }

        assert(sample->positions());
        assert(sample->normals());

        state().vboProxy().drawWireframe(sample);
    }
};


//==========================================================================
// CLASS DrawShadedTraversal
//==========================================================================

class DrawShadedTypes
{
public:
    enum ColorType {
        kSubNodeColor,
        kDefaultColor,
        kBlackColor,
        kXrayColor
    };

    enum NormalsType {
        kFrontNormals,
        kBackNormals
    };
};

typedef ::nozAlembicHolder::TextureMap TextureMap;

class DrawShadedState : public DrawTraversalState, public DrawShadedTypes
{
public:
    DrawShadedState(
        const Frustum&                  frustrum,
        const double                    seconds,
        const TransparentPruneType      transparentPrune,
        const ColorType                 colorType,
        const MColor&                   defaultDiffuseColor,
        const NormalsType               normalsType,
        const bool                      isTextured,
        TextureMap*                     textureMap,
        short                           textRes,
        std::set<MString, MStringComp>* ownTextures)
        : DrawTraversalState(frustrum, seconds, transparentPrune),
        fColorType(colorType),
        fDefaultDiffuseColor(defaultDiffuseColor),
        fNormalsType(normalsType),
        fIsTextured(isTextured),
        fTextureMap(textureMap),
        fTextRes(textRes),
        fOwnTextures(ownTextures)
    {}

    ColorType      colorType() const { return fColorType; }
    const MColor&  defaultDiffuseColor() const { return fDefaultDiffuseColor; }
    NormalsType    normalsType() const { return fNormalsType; }

    bool isTextured() const { return fIsTextured; }
    std::map<MString, MGLuint, MStringComp>* textureMap() { return fTextureMap; }
    short textRes() { return fTextRes; }
    short fTextRes;

    std::set<MString, MStringComp>* ownTextures() { return fOwnTextures; }

private:
    const ColorType      fColorType;
    const MColor         fDefaultDiffuseColor;
    const NormalsType    fNormalsType;

    const bool             fIsTextured;
    std::map<MString, MGLuint, MStringComp>* fTextureMap;
    std::set<MString, MStringComp>* fOwnTextures;
};

class DrawShadedTraversal
    : public DrawTraversal<DrawShadedTraversal, DrawShadedState>,
    public DrawShadedTypes
{
public:

    typedef DrawTraversal<DrawShadedTraversal, DrawShadedState> BaseClass;

    DrawShadedTraversal(
        DrawShadedState&        state,
        const MMatrix&          xform,
        bool                    isReflection,
        Frustum::ClippingResult parentClippingResult)
        : BaseClass(state, xform, isReflection, parentClippingResult)
    {}

    void draw(const std::shared_ptr<const ShapeSample>& sample)
    {
        if (!sample->visibility()) return;

        MGLuint textureID;

        VBOProxy::UVsMode useUvs = VBOProxy::kNoUVs;

        if (sample->uvs() && state().isTextured())
        {
            useUvs = VBOProxy::kUVs;
        }

        MColor diffuseColor = sample->diffuseColor();

        if (sample->getTexturePath() == "" || !state().isTextured())
        {
            gGLFT->glDisable(MGL_TEXTURE_2D);

        }
        else
        {
            std::map<MString, MGLuint, MStringComp>::iterator it;
            it = state().textureMap()->find(sample->getTexturePath());
            if (it != state().textureMap()->end()) {
                textureID = (*it).second;
            }
            else {
                state().ownTextures()->insert(sample->getTexturePath());
                gGLFT->glGenTextures(1, &textureID);
                gGLFT->glBindTexture(MGL_TEXTURE_2D, textureID);
                MImage image;
                MStatus stat = image.readFromFile(sample->getTexturePath());
                unsigned int res = state().textRes();
                unsigned int width, height;
                image.getSize(width, height);
                if (res != 0)
                {
                    if (width > res || height > res)
                    {
                        image.resize(res, res, true);
                        image.getSize(width, height);
                    }
                }
                if (!stat)
                {
                    MGlobal::displayWarning("In MTexture::load(), file not found:");
                }
                gGLFT->glTexImage2D(MGL_TEXTURE_2D, 0, MGL_RGBA8, width, height, 0, MGL_RGBA, MGL_UNSIGNED_BYTE, image.pixels());
                gGLFT->glTexParameteri(MGL_TEXTURE_2D, MGL_TEXTURE_MIN_FILTER, MGL_LINEAR);
                gGLFT->glTexParameteri(MGL_TEXTURE_2D, MGL_TEXTURE_MAG_FILTER, MGL_LINEAR);
                gGLFT->glBindTexture(MGL_TEXTURE_2D, 0); // deactivate the texture

                state().textureMap()->insert(std::pair<MString, MGLuint>(sample->getTexturePath(), textureID));


            }
        }

        gGLFT->glLoadMatrixd(xform().matrix[0]);

        if (sample->isBoundingBoxPlaceHolder()) {
            state().vboProxy().drawBoundingBox(sample, true);
            //GlobalReaderCache::theCache().hintShapeReadOrder(subNode());
            return;
        }

        if (!sample->positions() || !sample->normals())
        {
            return;
        }

        switch (state().colorType()) {
        case kSubNodeColor:
            //diffuseColor = sample->diffuseColor();
            diffuseColor = MColor(sample->diffuseColor()[0],
                sample->diffuseColor()[1],
                sample->diffuseColor()[2],
                1.0f);
            break;
        case kDefaultColor:
            diffuseColor = state().defaultDiffuseColor();
            break;
        case kBlackColor:
            diffuseColor =
                MColor(0.0f, 0.0f, 0.0f, sample->diffuseColor()[3]);
            break;
        case kXrayColor:
            diffuseColor = MColor(sample->diffuseColor()[0],
                sample->diffuseColor()[1],
                sample->diffuseColor()[2],
                0.3f);
            break;
        default:
            assert(0);
            break;
        }

        if (diffuseColor[3] <= 0.0 ||
            (diffuseColor[3] >= 1.0 &&
                state().transparentPrune() == DrawShadedState::kPruneOpaque) ||
                (diffuseColor[3] < 1.0 &&
                    state().transparentPrune() == DrawShadedState::kPruneTransparent)) {
            return;
        }

        if (state().isTextured() && sample->getTexturePath() != "")
        {
            gGLFT->glEnable(MGL_TEXTURE_2D);
            gGLFT->glBindTexture(MGL_TEXTURE_2D, textureID);
            diffuseColor = MColor(1.0f, 1.0f, 1.0f, 1.0f);
        }

        gGLFT->glColor4f(diffuseColor[0] * diffuseColor[3],
            diffuseColor[1] * diffuseColor[3],
            diffuseColor[2] * diffuseColor[3],
            diffuseColor[3]);

        // The meaning of front faces changes depending whether
        // the transformation has a reflection or not.
        gGLFT->glFrontFace(isReflection() ? MGL_CW : MGL_CCW);

        for (size_t groupId = 0; groupId < sample->numIndexGroups(); ++groupId) {
            state().vboProxy().drawTriangles(
                sample, groupId,
                state().normalsType() == kFrontNormals ?
                VBOProxy::kFrontNormals : VBOProxy::kBackNormals,
                useUvs);
        }

        gGLFT->glDisable(MGL_TEXTURE_2D);
        gGLFT->glBindTexture(MGL_TEXTURE_2D, 0);
    }
};

} // unnamed namespace

void CAlembicHolderUI::drawWireframe(const MDrawRequest & request, M3dView & view) const
{
    // Get the surface shape
    auto node = (nozAlembicHolder*)surfaceShape();
    if (!node) return;

    // Extract the cached geometry.
    const auto& rootNode = node->getGeometry();
    if (!rootNode) return;

    const double seconds = MAnimControl::currentTime().as(MTime::kSeconds);
    //const double myseconds = seconds + (node->timeOffset / 24.0f);
    const double myseconds = seconds + node->getTimeOffset().as(MTime::kSeconds);

    MMatrix projMatrix;
    view.projectionMatrix(projMatrix);
    MMatrix modelViewMatrix;
    view.modelViewMatrix(modelViewMatrix);

    MMatrix localToPort = modelViewMatrix * projMatrix;

    view.beginGL();
    {
        // Query current state so it can be restored
        //
        bool lightingWasOn = gGLFT->glIsEnabled(MGL_LIGHTING) == MGL_TRUE;

        // Setup the OpenGL state as necessary
        //
        if (lightingWasOn) {
            gGLFT->glDisable(MGL_LIGHTING);
        }

        gGLFT->glEnable(MGL_LINE_STIPPLE);
        if (request.token() == kDrawWireframeOnShaded) {
            // Wireframe on shaded is affected by wireframe on shaded mode
            DisplayPref::WireframeOnShadedMode wireframeOnShadedMode =
                DisplayPref::wireframeOnShadedMode();
            if (wireframeOnShadedMode == DisplayPref::kWireframeOnShadedReduced) {
                gGLFT->glLineStipple(1, Config::kLineStippleDotted);
            }
            else {
                assert(wireframeOnShadedMode != DisplayPref::kWireframeOnShadedNone);
                gGLFT->glLineStipple(1, Config::kLineStippleShortDashed);
            }
        }
        else {
            gGLFT->glLineStipple(1, Config::kLineStippleShortDashed);
        }

        // Draw the wireframe mesh
        //
        {
            Frustum frustum(localToPort.inverse());
            MMatrix xform(modelViewMatrix);

            DrawWireframeState state(frustum, myseconds);
            DrawWireframeTraversal traveral(state, xform, false, Frustum::kUnknown);
            rootNode->accept(traveral);
        }

        // Restore the state
        //
        if (lightingWasOn) {
            gGLFT->glEnable(MGL_LIGHTING);
        }

        gGLFT->glDisable(MGL_LINE_STIPPLE);
    }
    view.endGL();
}


void CAlembicHolderUI::drawShaded(
    const MDrawRequest & request, M3dView & view, bool depthOffset) const
{
    // Get the surface shape
    const auto& node = (nozAlembicHolder*)surfaceShape();
    if (!node) return;

    // Extract the cached geometry.
    const auto& rootNode = node->getGeometry();
    if (!rootNode) return;

    const auto& textureMap = node->getTextureMap();
    if (!textureMap) return;

    const double seconds = MAnimControl::currentTime().as(MTime::kSeconds);
    //const double myseconds = seconds + (node->timeOffset / 24.0f);
    const double myseconds = seconds + node->getTimeOffset().as(MTime::kSeconds);

    MMatrix projMatrix;
    view.projectionMatrix(projMatrix);
    MMatrix modelViewMatrix;
    view.modelViewMatrix(modelViewMatrix);

    MMatrix localToNDC = modelViewMatrix * projMatrix;

    bool textured = false;
    if (view.textureMode())
    {
        textured = true;
    }

    M3dView::LightingMode lightingMode;
    view.getLightingMode(lightingMode);
    unsigned int lightCount;
    view.getLightCount(lightCount);

    const bool noLightSoDrawAsBlack =
        (lightingMode == M3dView::kLightAll ||
            lightingMode == M3dView::kLightSelected ||
            lightingMode == M3dView::kLightActive)
        && lightCount == 0;

    view.beginGL();
    {
        // Setup the OpenGL state as necessary
        //
        // The most straightforward way to ensure that the OpenGL
        // material parameters are properly restored after drawing is
        // to use push/pop attrib as we have no easy of knowing the
        // current values of all the parameters.
        gGLFT->glPushAttrib(MGL_LIGHTING_BIT);

        // Reset specular and emission materials as we only display diffuse color.
        {
            static const float sBlack[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            gGLFT->glMaterialfv(MGL_FRONT_AND_BACK, MGL_SPECULAR, sBlack);
            gGLFT->glMaterialfv(MGL_FRONT_AND_BACK, MGL_EMISSION, sBlack);
        }

        DrawShadedState::TransparentPruneType transparentPrune =
            DrawShadedState::kPruneTransparent;

        const bool isTransparent = request.isTransparent();
        if (isTransparent) {
            // We use premultiplied alpha
            gGLFT->glBlendFunc(MGL_ONE, MGL_ONE_MINUS_SRC_ALPHA);
            transparentPrune = DrawShadedState::kPruneOpaque;

            gGLFT->glDepthMask(false);
        }

        MColor defaultDiffuseColor;
        DrawShadedTypes::ColorType colorType = DrawShadedTypes::kSubNodeColor;
        if (view.usingDefaultMaterial()) {
            if (!noLightSoDrawAsBlack) {
                MMaterial material = request.material();
                material.setMaterial(request.multiPath(), isTransparent);
                material.getDiffuse(defaultDiffuseColor);
            }

            // We must ignore the alpha channel of the default
            // material when the option "Use default material" is
            // selected.
            defaultDiffuseColor[3] = 1.0f;
            transparentPrune = DrawShadedState::kPruneNone;
            colorType = DrawShadedTypes::kDefaultColor;
        }
        else if (view.xray()) {
            transparentPrune = DrawShadedState::kPruneNone;

            if (noLightSoDrawAsBlack) {
                defaultDiffuseColor = MColor(0, 0, 0, 0.3f);
                colorType = DrawShadedTypes::kDefaultColor;
            }
            else {
                colorType = DrawShadedTypes::kXrayColor;
            }
        }
        else if (noLightSoDrawAsBlack) {
            colorType = DrawShadedTypes::kBlackColor;
        }

        if (noLightSoDrawAsBlack) {
            // The default viewport leaves an unrelated light enabled
            // in the OpenGL state even when there are no light in the
            // scene. We therefore manually disable lighting in that
            // case.
            gGLFT->glDisable(MGL_LIGHTING);
        }

        const bool depthOffsetWasEnabled = (gGLFT->glIsEnabled(MGL_POLYGON_OFFSET_FILL) == MGL_TRUE);
        if (depthOffset && !depthOffsetWasEnabled) {
            // Viewport has set the offset, just enable it
            gGLFT->glEnable(MGL_POLYGON_OFFSET_FILL);
        }

        // We will override the material color for each individual sub-nodes.!
        gGLFT->glColorMaterial(MGL_FRONT_AND_BACK, MGL_AMBIENT_AND_DIFFUSE);
        gGLFT->glEnable(MGL_COLOR_MATERIAL);

        // On Geforce cards, we emulate two-sided lighting by drawing
        // triangles twice because two-sided lighting is 10 times
        // slower than single-sided lighting.
        bool needEmulateTwoSidedLighting = false;
        if (Config::emulateTwoSidedLighting()) {
            // Query face-culling and two-sided lighting state
            bool  cullFace = (gGLFT->glIsEnabled(MGL_CULL_FACE) == MGL_TRUE);
            MGLint twoSidedLighting = MGL_FALSE;
            gGLFT->glGetIntegerv(MGL_LIGHT_MODEL_TWO_SIDE, &twoSidedLighting);

            // Need to emulate two-sided lighting when back-face
            // culling is off (i.e. drawing both sides) and two-sided
            // lLighting is on.
            needEmulateTwoSidedLighting = (!cullFace && twoSidedLighting);
        }

        {
            Frustum frustum(localToNDC.inverse());
            MMatrix xform(modelViewMatrix);

            if (needEmulateTwoSidedLighting) {
                gGLFT->glEnable(MGL_CULL_FACE);
                gGLFT->glLightModeli(MGL_LIGHT_MODEL_TWO_SIDE, 0);

                // first, draw with back-face culling
                {
                    gGLFT->glCullFace(MGL_FRONT);
                    DrawShadedState state(frustum,
                        myseconds,
                        transparentPrune,
                        colorType,
                        defaultDiffuseColor,
                        DrawShadedState::kBackNormals,
                        textured,
                        textureMap,
                        node->getTextRes(),
                        node->getOwnTextures());
                    DrawShadedTraversal traveral(
                        state, xform, xform.det3x3() < 0.0, Frustum::kUnknown);
                    rootNode->accept(traveral);
                }

                // then, draw with front-face culling
                {
                    gGLFT->glCullFace(MGL_BACK);
                    DrawShadedState state(frustum,
                        myseconds,
                        transparentPrune,
                        colorType,
                        defaultDiffuseColor,
                        DrawShadedState::kFrontNormals,
                        textured,
                        textureMap,
                        node->getTextRes(),
                        node->getOwnTextures());
                    DrawShadedTraversal traveral(
                        state, xform, xform.det3x3() < 0.0, Frustum::kUnknown);
                    rootNode->accept(traveral);
                }

                // restore the OpenGL state
                gGLFT->glDisable(MGL_CULL_FACE);
                gGLFT->glLightModeli(MGL_LIGHT_MODEL_TWO_SIDE, 1);
            }
            else {
                DrawShadedState state(frustum,
                    myseconds,
                    transparentPrune,
                    colorType,
                    defaultDiffuseColor,
                    DrawShadedState::kFrontNormals,
                    textured,
                    textureMap,
                    node->getTextRes(),
                    node->getOwnTextures());
                DrawShadedTraversal traveral(
                    state, xform, xform.det3x3() < 0.0, Frustum::kUnknown);
                rootNode->accept(traveral);
            }
        }

        // Restore the state
        //
        if (isTransparent) {
            gGLFT->glDepthMask(true);

            gGLFT->glBlendFunc(MGL_SRC_ALPHA, MGL_ONE_MINUS_SRC_ALPHA);
        }

        if (depthOffset && !depthOffsetWasEnabled) {
            gGLFT->glDisable(MGL_POLYGON_OFFSET_FILL);
        }

        gGLFT->glFrontFace(MGL_CCW);

        gGLFT->glPopAttrib();
    }
    view.endGL();
}

//
// Returns the point in world space corresponding to a given
// depth. The depth is specified as 0.0 for the near clipping plane and
// 1.0 for the far clipping plane.
//
MPoint CAlembicHolderUI::getPointAtDepth(
    MSelectInfo &selectInfo,
    double     depth)
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
    short x, y;
    view.worldToView(cursor, x, y);

    MPoint res, neardb, fardb;
    view.viewToWorld(x, y, neardb, fardb);
    res = neardb + depth*(fardb - neardb);

    return res;
}


bool CAlembicHolderUI::select(
    MSelectInfo &selectInfo,
    MSelectionList &selectionList,
    MPointArray &worldSpaceSelectPts) const
{
    // Initialize GL Function Table.
    InitializeGLFT();

    MSelectionMask mask(kPluginId);
    if (!selectInfo.selectable(mask)) {
        return false;
    }

    // Check plugin display filter. Invisible geometry can't be selected
    if (!selectInfo.pluginObjectDisplayStatus(Config::kDisplayFilter)) {
        return false;
    }

    // Get the geometry information
    //
    auto node = (nozAlembicHolder*)surfaceShape();
    const auto& alembicData = node->alembicData();
    const auto& rootNode = node->getGeometry();
    if (!rootNode) { return false; }

    const double seconds = MAnimControl::currentTime().as(MTime::kSeconds);
    //const double myseconds = seconds + (node->timeOffset / 24.0f);
    const double myseconds = seconds + node->getTimeOffset().as(MTime::kSeconds);

    const bool boundingboxSelection =
        (M3dView::kBoundingBox == selectInfo.displayStyle());

    const bool wireframeSelection =
        (M3dView::kWireFrame == selectInfo.displayStyle() ||
            !selectInfo.singleSelection());

    // If all the model editors are Viewport2.0, we will not use VBO for select
    // because VBO will double the memory consumption.
    VBOProxy::VBOMode vboMode = VBOProxy::kUseVBOIfPossible;
    //if (Config::vp2OverrideAPI() != Config::kMPxDrawOverride) {
    //    vboMode = (sModelEditorState == kViewport2Only) ?
    //        VBOProxy::kDontUseVBO : VBOProxy::kUseVBOIfPossible;
    //}

    // We select base on edges if the object is displayed in wireframe
    // mode or if we are performing a marquee selection. Else, we
    // select using the object faces (i.e. single-click selection in
    // shaded mode).
    GLfloat minZ;
    {
        Select* selector;
        NbPrimitivesVisitor nbPrimitives(myseconds);
        rootNode->accept(nbPrimitives);

        if (boundingboxSelection) {
            // We are only drawing 12 edges so we only use GL picking selection.
            selector = new GLPickingSelect(selectInfo);

            selector->processBoundingBox(rootNode, myseconds);
        }
        else if (wireframeSelection) {
            if (nbPrimitives.numWires() < Config::openGLPickingWireframeThreshold())
                selector = new GLPickingSelect(selectInfo);
            else
                selector = new RasterSelect(selectInfo);

            //selector->processEdges(rootNode, myseconds, nbPrimitives.numWires(), vboMode);
            selector->processEdges(alembicData, alembicData->m_currscenekey, nbPrimitives.numWires());
        }
        else {
            if (nbPrimitives.numTriangles() < Config::openGLPickingSurfaceThreshold())
                selector = new GLPickingSelect(selectInfo);
            else
                selector = new RasterSelect(selectInfo);

            //selector->processTriangles(rootNode, myseconds, nbPrimitives.numTriangles(), vboMode);
            selector->processTriangles(alembicData, alembicData->m_currscenekey, nbPrimitives.numTriangles());
        }
        selector->end();
        minZ = selector->minZ();
        delete selector;
    }

    bool selected = (minZ <= 1.0f);
    if (selected) {
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
            mask, false);
    }

    return selected;
}

bool CAlembicHolderUI::snap(MSelectInfo& snapInfo) const
{
    // Initialize GL Function Table.
    InitializeGLFT();

    // Check plugin display filter. Invisible geometry can't be snapped
    if (!snapInfo.pluginObjectDisplayStatus(Config::kDisplayFilter)) {
        return false;
    }

    // Get the geometry information
    //
    auto node = (nozAlembicHolder*)surfaceShape();
    const auto& rootNode = node->getGeometry();
    if (!rootNode) return false;

    const double seconds = MAnimControl::currentTime().as(MTime::kSeconds);
    //const double myseconds = seconds + (node->timeOffset / 24.0f);
    const double myseconds = seconds + node->getTimeOffset().as(MTime::kSeconds);

    M3dView view = snapInfo.view();

    const MDagPath & path = snapInfo.multiPath();
    const MMatrix inclusiveMatrix = path.inclusiveMatrix();

    MMatrix projMatrix;
    view.projectionMatrix(projMatrix);
    MMatrix modelViewMatrix;
    view.modelViewMatrix(modelViewMatrix);

    unsigned int vpx, vpy, vpw, vph;
    view.viewport(vpx, vpy, vpw, vph);
    double w_over_two = vpw * 0.5;
    double h_over_two = vph * 0.5;
    double vpoff_x = w_over_two + vpx;
    double vpoff_y = h_over_two + vpy;
    MMatrix ndcToPort;
    ndcToPort(0, 0) = w_over_two;
    ndcToPort(1, 1) = h_over_two;
    ndcToPort(2, 2) = 0.5;
    ndcToPort(3, 0) = vpoff_x;
    ndcToPort(3, 1) = vpoff_y;
    ndcToPort(3, 2) = 0.5;

    const MMatrix localToNDC = modelViewMatrix * projMatrix;
    const MMatrix localToPort = localToNDC * ndcToPort;

    AlembicHolder::Frustum frustum(localToNDC.inverse());

    SnapTraversalState state(
        frustum, myseconds, localToPort, inclusiveMatrix, snapInfo);
    SnapTraversal visitor(state, MMatrix::identity, false, Frustum::kUnknown);
    rootNode->accept(visitor);
    return state.selected();
}


namespace AlembicHolder {

//==============================================================================
// Error checking
//==============================================================================

#define MCHECKERROR(STAT,MSG)                   \
if (!STAT) {                                \
    perror(MSG);                            \
    return MS::kFailure;                    \
}

#define MREPORTERROR(STAT,MSG)                  \
if (!STAT) {                                \
    perror(MSG);                            \
}

#define MCHECKERRORVOID(STAT,MSG)               \
if (!STAT) {                                \
    perror(MSG);                            \
    return;                                 \
}


//==============================================================================
// CLASS DisplayPref
//==============================================================================

DisplayPref::WireframeOnShadedMode DisplayPref::fsWireframeOnShadedMode;
MCallbackId DisplayPref::fsDisplayPrefChangedCallbackId;

MStatus DisplayPref::initCallback()
{
    MStatus stat;

    // Register DisplayPreferenceChanged callback
    fsDisplayPrefChangedCallbackId = MEventMessage::addEventCallback(
        "DisplayPreferenceChanged", DisplayPref::displayPrefChanged, NULL, &stat);
    MCHECKERROR(stat, "MEventMessage::addEventCallback(DisplayPreferenceChanged");

    // Trigger the callback manually to init class members
    displayPrefChanged(NULL);

    return MS::kSuccess;
}

MStatus DisplayPref::removeCallback()
{
    MStatus stat;

    // Remove DisplayPreferenceChanged callback
    MEventMessage::removeCallback(fsDisplayPrefChangedCallbackId);
    MCHECKERROR(stat, "MEventMessage::removeCallback(DisplayPreferenceChanged)");

    return MS::kSuccess;
}

void DisplayPref::displayPrefChanged(void*)
{
    MStatus stat;
    // Wireframe on shaded mode: Full/Reduced/None
    MString wireframeOnShadedActive = MGlobal::executeCommandStringResult(
        "displayPref -q -wireframeOnShadedActive", false, false, &stat);
    if (stat) {
        if (wireframeOnShadedActive == "full") {
            fsWireframeOnShadedMode = kWireframeOnShadedFull;
        }
        else if (wireframeOnShadedActive == "reduced") {
            fsWireframeOnShadedMode = kWireframeOnShadedReduced;
        }
        else if (wireframeOnShadedActive == "none") {
            fsWireframeOnShadedMode = kWireframeOnShadedNone;
        }
        else {
            assert(0);
        }
    }
}

DisplayPref::WireframeOnShadedMode DisplayPref::wireframeOnShadedMode()
{
    return fsWireframeOnShadedMode;
}

} // namespace AlembicHolder
