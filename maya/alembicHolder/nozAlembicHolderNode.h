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


#ifndef _nozAlembicHolderNode
#define _nozAlembicHolderNode

#include "Drawable.h"
#include "Foundation.h"
#include "Scene.h"
#include "SceneManager.h"

#include <maya/MPxSurfaceShape.h>
#include <maya/MPxSurfaceShapeUI.h>

#include <maya/MPxNode.h>
//#include <maya/MPxLocatorNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnData.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MBoundingBox.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MIntArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnStringData.h>
#include <maya/MTypeId.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagPath.h>
#include <maya/MNodeMessage.h>
#include <maya/MMatrix.h>
#include <maya/MFnCamera.h>
#include <maya/MFnRenderLayer.h>

#include <json/json.h>
#include "parseJson.h"

#include <iostream>


class CAlembicDatas
{
public:
    CAlembicDatas();
    MBoundingBox bbox;
    int token;
    bool m_abcdirty;
    std::string m_currscenekey;
	std::string m_currselectionkey;
    bool m_bbextendedmode;
    double time;

	holderPrms *m_params;

    //BufferObject buffer;

    static AlembicHolder::SceneState   abcSceneState;
    static AlembicHolder::SceneManager abcSceneManager;
};


struct MStringComp
{
    bool operator() (MString lhs, MString rhs) const
    {
        int res = strcmp(lhs.asChar(), rhs.asChar());
        if (res < 0) {
            return true;
        }
        else {
            return false;
        }
        return strcmp(lhs.asChar(), rhs.asChar()) <= 0;
    }
};


class nozAlembicHolder : public MPxSurfaceShape
{
public:
    nozAlembicHolder();
    virtual ~nozAlembicHolder();

    virtual void postConstructor();
    virtual MStatus compute( const MPlug& plug, MDataBlock& data );

    bool getInternalValueInContext(const MPlug& plug, MDataHandle& dataHandle, MDGContext& ctx) override;
    bool setInternalValueInContext(const MPlug& plug, const MDataHandle& dataHandle, MDGContext& ctx) override;

    virtual bool isBounded() const;
    virtual MBoundingBox boundingBox()const ;
    int cacheVersion() const { return MPlug(thisMObject(), aUpdateCache).asInt(); }
    int assignmentVersion() const { return MPlug(thisMObject(), aUpdateAssign).asInt(); }
    MStatus setDependentsDirty(MPlug const & inPlug, MPlugArray  & affectedPlugs);

    virtual void copyInternalData( MPxNode* srcNode );

    void setHolderTime();

    static  void*       creator();
    static  MStatus     initialize();

    std::string getSceneKey() const;
    std::string getSelectionKey() const;

    CAlembicDatas* alembicData();
    const CAlembicDatas* alembicData() const { return &fGeometry; }
    AlembicHolder::DrawablePtr getGeometry() const;
    AlembicHolder::MaterialGraphMap::Ptr getMaterial() const;





    MTime getTimeOffset() const { return MPlug(thisMObject(), aTimeOffset).asMTime(); }

    typedef std::map<MString, MGLuint, MStringComp> TextureMap;
    TextureMap* getTextureMap() { return &textureMap; }

    typedef std::set<MString, MStringComp> TextureSet;
    TextureSet* getOwnTextures() { return &ownTextures; }

    short getTextRes() const { return textRes; }

private:
    CAlembicDatas        fGeometry;
    TextureMap           textureMap;
    TextureSet           ownTextures;
    int fCacheVersion;
    int fAssignmentVersion;

    static    MObject    aAbcFile;
    static    MObject    aObjectPath;
    static    MObject    aBoundingExtended;
//    static  MObject    aBooleanAttr; // example boolean attribute
    static    MObject    aTime;
    static    MObject    aTimeOffset;
    static    MObject    aSelectionPath;
    static    MObject    aShaderPath;
    static    MObject    aForceReload;
    
	static    MObject    aJsonFile;
	static    MObject    aJsonFileSecondary;
	static    MObject    aShadersNamespace;
	static    MObject    aShadersAttribute;
	static    MObject    aAbcShaders;
	static    MObject	 aUvsArchive;
	static    MObject	 aShadersAssignation;
	static    MObject	 aAttributes;
	static    MObject	 aDisplacementsAssignation;
	static    MObject	 aLayersOverride;
	static    MObject    aSkipJsonFile;
	static    MObject    aSkipShaders;
	static    MObject    aSkipAttributes;
	static    MObject    aSkipLayers;
	static    MObject    aSkipDisplacements;
	
	static    MObject	 aUpdateAssign;
	static    MObject    aUpdateCache;

    static    MObject    aBoundMin;
    static    MObject    aBoundMax;
    bool isConstant;

    static MObject       aTextureResolution;
    short textRes;

public:
    static  MTypeId     id;

protected:
    int dUpdate;
	int dUpdateA;

};

class CAlembicHolderUI : public MPxSurfaceShapeUI {
public:

    static void* creator();

    CAlembicHolderUI ();
    ~CAlembicHolderUI ();

    virtual void getDrawRequests(const MDrawInfo & info,
        bool objectAndActiveOnly,
        MDrawRequestQueue & queue);

    // Viewport 1.0 draw
    virtual void draw(const MDrawRequest & request, M3dView & view) const;

    virtual bool snap(MSelectInfo &snapInfo) const;
    virtual bool select(MSelectInfo &selectInfo, MSelectionList &selectionList,
        MPointArray &worldSpaceSelectPts) const;


private:
    // Prohibited and not implemented.
    CAlembicHolderUI(const CAlembicHolderUI& obj);
    const CAlembicHolderUI& operator=(const CAlembicHolderUI& obj);

    static MPoint getPointAtDepth(MSelectInfo &selectInfo, double depth);

    // Helper functions for the viewport 1.0 drawing purposes.
    void drawBoundingBox(const MDrawRequest & request, M3dView & view) const;
    void drawWireframe(const MDrawRequest & request, M3dView & view) const;
    void drawShaded(const MDrawRequest & request, M3dView & view, bool depthOffset) const;

    // Draw Tokens
    enum DrawToken {
        kBoundingBox,
        kDrawWireframe,
        kDrawWireframeOnShaded,
        kDrawSmoothShaded,
        kDrawSmoothShadedDepthOffset
    };
};


namespace AlembicHolder {

///////////////////////////////////////////////////////////////////////////////
//
// DisplayPref
//
// Keeps track of the display preference.
//
////////////////////////////////////////////////////////////////////////////////

class DisplayPref {
public:
    enum WireframeOnShadedMode {
        kWireframeOnShadedFull,
        kWireframeOnShadedReduced,
        kWireframeOnShadedNone
    };

    static WireframeOnShadedMode wireframeOnShadedMode();

    static MStatus initCallback();
    static MStatus removeCallback();

private:
    static void displayPrefChanged(void*);

    static WireframeOnShadedMode fsWireframeOnShadedMode;
    static MCallbackId fsDisplayPrefChangedCallbackId;
};

} // namespace AlembicHolder


#endif // header guard
