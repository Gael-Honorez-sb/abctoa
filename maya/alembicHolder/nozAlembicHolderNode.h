#ifndef _nozAlembicHolderNode
#define _nozAlembicHolderNode
//
// Copyright (C) 2014 Nozon.
//
// File: nozAlembicHolderNode.h
//
// Dependency Graph Node: nozAlembicHolder
//
// Author: Gaël Honorez
//

#include "Foundation.h"
#include "Transport.h"
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

#include <iostream>

class CAlembicDatas
{
public:
    CAlembicDatas();
       MBoundingBox bbox;
       int token;
       bool m_abcdirty;
       std::string m_currscenekey;
       int m_bbmode;
       float time;

       //BufferObject buffer;

       static SimpleAbcViewer::SceneState   abcSceneState;

       static SimpleAbcViewer::SceneManager abcSceneManager;


};


class nozAlembicHolder : public MPxSurfaceShape
{
public:
    nozAlembicHolder();
    virtual ~nozAlembicHolder();

    virtual void postConstructor();
    virtual MStatus compute( const MPlug& plug, MDataBlock& data );
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
    bool GetPlugData();


    virtual void copyInternalData( MPxNode* srcNode );

    double setHolderTime() const;

    static  void*       creator();
    static  MStatus     initialize();

    MStatus     doSomething();

    std::string getSceneKey() const;
    std::string getSelectionKey() const;

    CAlembicDatas* alembicData();






private:
    CAlembicDatas        fGeometry;
    static  MObject     aAbcFile;
    static  MObject     aObjectPath;
//    static  MObject     aBooleanAttr; // example boolean attribute
    static  MObject     aTime;
    static  MObject     aTimeOffset;
    static  MObject        aSelectionPath;
    static  MObject     aShaderPath;
    static    MObject        aUpdateCache;
    static    MObject        aBoundMin;
    static    MObject        aBoundMinX;
    static    MObject        aBoundMinY;
    static    MObject        aBoundMinZ;
    static    MObject        aBoundMax;
    static    MObject        aBoundMaxX;
    static    MObject        aBoundMaxY;
    static    MObject        aBoundMaxZ;

public:
    static  MTypeId     id;

protected:
    int dUpdate;

};
// UI class    - defines the UI part of a shape node
class CAlembicHolderUI: public MPxSurfaceShapeUI {
public:
    CAlembicHolderUI();
    virtual ~CAlembicHolderUI();
    virtual void getDrawRequests(const MDrawInfo & info,
            bool objectAndActiveOnly, MDrawRequestQueue & requests);
    virtual void draw(const MDrawRequest & request, M3dView & view) const;

    void drawBoundingBox( const MDrawRequest & request, M3dView & view ) const;
    virtual bool select(MSelectInfo &selectInfo, MSelectionList &selectionList,
            MPointArray &worldSpaceSelectPts) const;

    void getDrawRequestsWireFrame(MDrawRequest&, const MDrawInfo&);
    void getDrawRequestsBoundingBox(MDrawRequest&, const MDrawInfo&);
    void            getDrawRequestsShaded(      MDrawRequest&,
                                              const MDrawInfo&,
                                              MDrawRequestQueue&,
                                              MDrawData& data );


    static void * creator();
    // Draw Tokens
    //
    enum {
        kDrawWireframe,
        kDrawWireframeOnShaded,
        kDrawSmoothShaded,
        kDrawFlatShaded,
        kDrawBoundingBox,
        kLastToken
    };

}; // class CArnoldStandInShapeUI




#endif


