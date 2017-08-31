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
#include <maya/MFnStringArrayData.h>
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


class nozAlembicHolder : public MPxSurfaceShape
{
public:
    nozAlembicHolder();
    virtual ~nozAlembicHolder();

    virtual void postConstructor();
    virtual MStatus compute( const MPlug& plug, MDataBlock& data );
    void GetPlugData() const;
//
//    virtual bool getInternalValueInContext     (     const MPlug &      plug,
//            MDataHandle &      dataHandle,
//            MDGContext &      ctx
//        )     ;
//
//    virtual bool setInternalValueInContext     (     const MPlug &      plug,
//                MDataHandle &      dataHandle,
//                MDGContext &      ctx
//            )     ;

    virtual bool isBounded() const;
    virtual MBoundingBox boundingBox()const ;

    bool match(const MSelectionMask & mask,
        const MObjectArray& componentList) const override;
    MSelectionMask getShapeSelectionMask() const override;

    MStatus setDependentsDirty(MPlug const & inPlug, MPlugArray  & affectedPlugs);

    virtual void copyInternalData( MPxNode* srcNode );

    void setHolderTime() const;

    static  void*       creator();
    static  MStatus     initialize();

    std::string getSceneKey() const;
    std::string getSelectionKey() const;

    CAlembicDatas* alembicData() const;


    static const char* selectionMaskName;



private:
    CAlembicDatas *fGeometry;
    static    MObject    aAbcFiles;
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

public:
    static  MTypeId     id;

private:
    int dUpdate;
	int dUpdateA;

};
// UI class    - defines the UI part of a shape node
class CAlembicHolderUI: public MPxSurfaceShapeUI {
public:
    CAlembicHolderUI();
    virtual ~CAlembicHolderUI();
    /*virtual void getDrawRequests(const MDrawInfo & info,
            bool objectAndActiveOnly, MDrawRequestQueue & requests);
    virtual void draw(const MDrawRequest & request, M3dView & view) const;

    void drawBoundingBox( const MDrawRequest & request, M3dView & view ) const;
    void drawingMeshes( std::string sceneKey, CAlembicDatas * cache, std::string selectionKey) const;


    void getDrawRequestsWireFrame(MDrawRequest&, const MDrawInfo&);
    void getDrawRequestsBoundingBox(MDrawRequest&, const MDrawInfo&);
    void            getDrawRequestsShaded(      MDrawRequest&,
                                              const MDrawInfo&,
                                              MDrawRequestQueue&,
                                              MDrawData& data );

*/
    

    static void * creator();

}; // class CArnoldStandInShapeUI




#endif


