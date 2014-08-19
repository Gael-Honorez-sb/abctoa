//
// Copyright (C) 2014 Nozon. 
// 
// Dependency Graph Node: nozAlembicHolder
//
// Author: Gaël Honorez
//

#include <maya/MCommandResult.h>

#include "nozAlembicHolderNode.h"

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
#include <maya/MFnMessageAttribute.h>
#include <maya/MFileObject.h>

#include <stdio.h>
#include <map>


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

//
static MGLFunctionTable *gGLFT = NULL;

//#include "boost/foreach.hpp"

typedef std::map<std::string, GLuint> NodeCache;
NodeCache g_meshCache;
NodeCache g_bboxCache;


// The id is a 32bit value used to identify this type of node in the binary file format.
MTypeId nozAlembicHolder::id(0x00114956); 

MObject nozAlembicHolder::aAbcFile;
MObject nozAlembicHolder::aObjectPath;
MObject nozAlembicHolder::aSelectionPath;
MObject nozAlembicHolder::aTime;
MObject nozAlembicHolder::aTimeOffset;
MObject nozAlembicHolder::aShaderPath;
MObject nozAlembicHolder::aUpdateCache;

// for bbox
MObject nozAlembicHolder::aBoundMin;
MObject nozAlembicHolder::aBoundMinX;
MObject nozAlembicHolder::aBoundMinY;
MObject nozAlembicHolder::aBoundMinZ;
MObject nozAlembicHolder::aBoundMax;
MObject nozAlembicHolder::aBoundMaxX;
MObject nozAlembicHolder::aBoundMaxY;
MObject nozAlembicHolder::aBoundMaxZ;


SimpleAbcViewer::SceneState CAlembicDatas::abcSceneState;
SimpleAbcViewer::SceneManager CAlembicDatas::abcSceneManager;

CAlembicDatas::CAlembicDatas() {

    bbox = MBoundingBox(MPoint(-1.0f, -1.0f, -1.0f), MPoint(1.0f, 1.0f, 1.0f));
    token = 0;
    m_bbmode = 0;
    m_currscenekey = "";
    m_abcdirty = false;

}

void abcDirtiedCallback(MObject & nodeMO, MPlug & plug, void* data) {

    const nozAlembicHolder& node = *(nozAlembicHolder*)data;

    //nozAlembicHolder::nozAlembicHolder *node = (nozAlembicHolder::nozAlembicHolder *) data;
    //cout << "plug.attribute()" << plug.partialName(false, false, false, false, false, true) << endl;

    MFnDagNode fn( nodeMO );
    if (plug.partialName(false, false, false, false, false, true) == "abcFile" || plug.partialName(false, false, false, false, false, true) == "objectPath")
    {
        MFnDagNode fn(node.thisMObject());
        //std::cout << "plugDirtied: " << fn.name() << " " << plug.name() << " = " << plug.asString() << std::endl;
        node.boundingBox();

    }
}

void abcChangedCallback(MNodeMessage::AttributeMessage msg, MPlug & plug,
        MPlug & otherPlug, void* data) {
}

void nodePreRemovalCallback(MObject& obj, void* data) {
    std::string sceneKey = ((nozAlembicHolder*) data)->getSceneKey();
    CAlembicDatas::abcSceneManager.removeScene(sceneKey);
    if(CAlembicDatas::abcSceneManager.hasKey(sceneKey) == 0)
    {
        NodeCache::iterator I = g_meshCache.find(sceneKey);
        if (I != g_meshCache.end())
        {
            glDeleteLists((*I).second, 1);
            g_meshCache.erase(I);
        }
        NodeCache::iterator J = g_bboxCache.find(sceneKey);
        if (J != g_bboxCache.end())
        {
            glDeleteLists((*J).second, 1);
            g_bboxCache.erase(J);
        }

    }
}

nozAlembicHolder::nozAlembicHolder() {
        //std::cout << "class creator" << std::endl;
}

nozAlembicHolder::~nozAlembicHolder() {
}

double nozAlembicHolder::setHolderTime() const {
    nozAlembicHolder* nonConstThis = const_cast<nozAlembicHolder*> (this);
    CAlembicDatas* geom = nonConstThis->alembicData();



    MFnDagNode fn(thisMObject());
    MTime time, timeOffset;

    MPlug plug = fn.findPlug(aTime);
    plug.getValue(time);
    plug = fn.findPlug(aTimeOffset);
    plug.getValue(timeOffset);

    double dtime;
    
    dtime = time.as(MTime::kSeconds) + timeOffset.as(MTime::kSeconds);

    std::string sceneKey = getSceneKey();
    if (geom->abcSceneManager.hasKey(sceneKey))
        geom->abcSceneManager.getScene(sceneKey)->setTime(dtime);

    return dtime;
}


bool nozAlembicHolder::isBounded() const {
    return true;
}



MBoundingBox nozAlembicHolder::boundingBox() const
{

    nozAlembicHolder* nonConstThis = const_cast<nozAlembicHolder*> (this);
    CAlembicDatas* geom = nonConstThis->alembicData();

    MBoundingBox bbox = MBoundingBox(MPoint(-1.0f, -1.0f, -1.0f), MPoint(1.0f, 1.0f, 1.0f));

    
    if(geom != NULL)
        bbox = geom->bbox;
    else
        cout << "Geom not valid" << endl;
    
    return bbox;

}

void* nozAlembicHolder::creator() {
    return new nozAlembicHolder();
}

void nozAlembicHolder::postConstructor() {
    // This call allows the shape to have shading groups assigned
    setRenderable(true);

    // callbacks
    MObject node = thisMObject();
    MNodeMessage::addAttributeChangedCallback(node, abcChangedCallback, this);
    MNodeMessage::addNodePreRemovalCallback(node, nodePreRemovalCallback, this);


}

void nozAlembicHolder::copyInternalData(MPxNode* srcNode) {
    // here we ensure that the scene manager stays up to date when duplicating nodes

    const nozAlembicHolder& node = *(nozAlembicHolder*)srcNode;
/*    nozAlembicHolder::nozAlembicHolder *node =
            (nozAlembicHolder::nozAlembicHolder *) srcNode;*/

    nozAlembicHolder* nonConstThis = const_cast<nozAlembicHolder*> (this);
    CAlembicDatas* geom = nonConstThis->alembicData();
    
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


    geom->abcSceneManager.addScene(abcfile.asChar(), objectPath.asChar());
    geom->m_currscenekey = getSceneKey();

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
    //tAttr.setUsedAsFilename(true);
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

    aShaderPath = mAttr.create("shaders", "shds", &stat);
    //    tAttr.setWritable(true);
    //    tAttr.setReadable(true);
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


    aUpdateCache = nAttr.create("updateCache", "upc", MFnNumericData::kInt, 0);
    nAttr.setHidden(true);


    aBoundMinX = nAttr.create( "outBoundMinX", "obminx", MFnNumericData::kFloat, 0, &stat);
    nAttr.setWritable( false );
    nAttr.setStorable( false );
                
    aBoundMinY = nAttr.create( "outBoundMinY", "cobminy", MFnNumericData::kFloat, 0, &stat );
    nAttr.setWritable( false );
    nAttr.setStorable( false );
        
    aBoundMinZ = nAttr.create( "outBoundMinZ", "obminz", MFnNumericData::kFloat, 0, &stat );
    nAttr.setWritable( false );
    nAttr.setStorable( false );
        
    aBoundMin = nAttr.create( "outBoundMin", "obmin", aBoundMinX, aBoundMinY, aBoundMinZ, &stat );
    nAttr.setWritable( false );
    nAttr.setStorable( false );
        
    aBoundMaxX = nAttr.create( "outBoundMaxX", "obmaxx", MFnNumericData::kFloat, 0, &stat );
    nAttr.setWritable( false );
    nAttr.setStorable( false );
                
    aBoundMaxY = nAttr.create( "outBoundMaxY", "cobmaxy", MFnNumericData::kFloat, 0, &stat );
    nAttr.setWritable( false );
    nAttr.setStorable( false );
        
    aBoundMaxZ = nAttr.create( "outBoundMaxZ", "obmaxz", MFnNumericData::kFloat, 0, &stat );
    nAttr.setWritable( false );
    nAttr.setStorable( false );
        
    aBoundMax = nAttr.create( "outBoundMax", "obmax", aBoundMaxX, aBoundMaxY, aBoundMaxZ, &stat );
    nAttr.setWritable( false );
    nAttr.setStorable( false );

    // Add the attributes we have created to the node
    //
    addAttribute( aAbcFile);
    addAttribute( aObjectPath);
    addAttribute( aSelectionPath);
    addAttribute( aShaderPath);    
    addAttribute( aTime);
    addAttribute( aTimeOffset);
    addAttribute ( aUpdateCache );
    addAttribute( aBoundMin);
    addAttribute( aBoundMax);

    attributeAffects ( aAbcFile, aUpdateCache );
    attributeAffects ( aObjectPath, aUpdateCache );
    attributeAffects ( aSelectionPath, aUpdateCache );
    attributeAffects ( aTime, aUpdateCache );
    attributeAffects ( aTimeOffset, aUpdateCache );

    attributeAffects ( aUpdateCache, aBoundMin );
    attributeAffects ( aUpdateCache, aBoundMax );


    return MS::kSuccess;

}

MStatus nozAlembicHolder::doSomething() {
    MStatus stat;

    cout << "just did something!! :D" << endl;
    return MS::kSuccess;
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
    MString abcfile;
    MPlug plug = fn.findPlug(aAbcFile);
    plug.getValue(abcfile);

    MFileObject fileObject;
    fileObject.setRawFullName(abcfile.expandFilePath());
    fileObject.setResolveMethod(MFileObject::kInputFile);
    abcfile = fileObject.resolvedFullName();

    MString objectPath;
    plug = fn.findPlug(aObjectPath);
    plug.getValue(objectPath);
    return std::string((abcfile + "|" + objectPath).asChar());
}


MStatus nozAlembicHolder::compute( const MPlug& plug, MDataBlock& block )
{
    if (plug == aUpdateCache)
    {
    
        MString file = block.inputValue(aAbcFile).asString();
        MFileObject fileObject;
        fileObject.setRawFullName(file.expandFilePath());
        fileObject.setResolveMethod(MFileObject::kInputFile);
        file = fileObject.resolvedFullName();

        MString objectPath = block.inputValue(aObjectPath).asString();
        MString selectionPath = block.inputValue(aSelectionPath).asString();

        float time = block.inputValue(aTime).asFloat() + block.inputValue(aTimeOffset).asFloat();

        bool hasToReload = false;

        if (fGeometry.time != time)
        {
            fGeometry.m_abcdirty = true;
            fGeometry.time = time;
            hasToReload = true;
        }

        MString mkey = file + "|" + objectPath;

        std::string key = mkey.asChar();

        if ((fGeometry.m_currscenekey != key && mkey != "|" ) || hasToReload)
        {    
            if (fGeometry.m_currscenekey != key && mkey != "|" )
            {
                CAlembicDatas::abcSceneManager.removeScene(fGeometry.m_currscenekey);
                if(CAlembicDatas::abcSceneManager.hasKey(fGeometry.m_currscenekey) == 0)
                {
                    NodeCache::iterator I = g_meshCache.find(fGeometry.m_currscenekey);
                    if (I != g_meshCache.end())
                    {
                        glDeleteLists((*I).second, 1);
                        g_meshCache.erase(I);
                    }
                    NodeCache::iterator J = g_bboxCache.find(fGeometry.m_currscenekey);
                    if (J != g_bboxCache.end())
                    {
                        glDeleteLists((*J).second, 1);
                        g_bboxCache.erase(J);
                    }

                }


                fGeometry.m_currscenekey = "";
                CAlembicDatas::abcSceneManager.addScene(file.asChar(), objectPath.asChar());
                fGeometry.m_currscenekey = key;
            }

            if (CAlembicDatas::abcSceneManager.hasKey(key))
            {
                fGeometry.bbox = MBoundingBox();
                SimpleAbcViewer::Box3d bb;
                setHolderTime();
                bb = CAlembicDatas::abcSceneManager.getScene(key)->getBounds();
                fGeometry.bbox.expand(MPoint(bb.min.x, bb.min.y, bb.min.z));
                fGeometry.bbox.expand(MPoint(bb.max.x, bb.max.y, bb.max.z));
            }

        
            fGeometry.m_abcdirty = true;

        }

    }
    else
    {
        return ( MS::kUnknownParameter );
    }

    block.setClean(plug);
    return MS::kSuccess;

}

bool nozAlembicHolder::GetPlugData()
{
    MObject thisNode = thisMObject();
    int update = 0;
    MPlug updatePlug(thisNode, aUpdateCache );
    updatePlug.getValue( update );
    if (update != dUpdate)
    {
        dUpdate = update;
        return true;
    }
    else
    {
        return false;
    }
    return false;

}

CAlembicDatas* nozAlembicHolder::alembicData() 
{
    if (MRenderView::doesRenderEditorExist())
    {
        GetPlugData();
        return &fGeometry;
    }
}

// UI IMPLEMENTATION

CAlembicHolderUI::~CAlembicHolderUI() {
}

void* CAlembicHolderUI::creator() {
    return new CAlembicHolderUI();
}

CAlembicHolderUI::CAlembicHolderUI() {
}
void CAlembicHolderUI::getDrawRequests(const MDrawInfo & info,
        bool /*objectAndActiveOnly*/, MDrawRequestQueue & queue) {

    MDrawData data;
    MDrawRequest request = info.getPrototype(*this);
    nozAlembicHolder* shapeNode = (nozAlembicHolder*) surfaceShape();
    CAlembicDatas* geom = shapeNode->alembicData();
    getDrawData(geom, data);
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

    case M3dView::kGouraudShaded:
        request.setToken(kDrawSmoothShaded);
        getDrawRequestsShaded(request, info, queue, data);
        queue.add(request);
        break;

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

void CAlembicHolderUI::draw(const MDrawRequest & request, M3dView & view) const 
{

    int token = request.token();

    MDrawData data = request.drawData();

    nozAlembicHolder* shapeNode = (nozAlembicHolder*) surfaceShape();
    CAlembicDatas * cache = (CAlembicDatas*) data.geometry();

    bool refresh = cache->m_abcdirty;
    int oldToken = cache->token;

    cache->token = token;

    std::string sceneKey = shapeNode->getSceneKey();

    std::string selectionKey = shapeNode->getSelectionKey();
    
    if(selectionKey != "")
        token = kDrawBoundingBox;

    view.beginGL(); 
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // handling refreshes
    GLuint glcache = -1;
    if(token == kDrawBoundingBox)
    {
        NodeCache::iterator I = g_bboxCache.find(sceneKey);
        if (I == g_bboxCache.end())
            refresh=true;
        else
            glcache = (*I).second;
    }
    else
    {
        NodeCache::iterator I = g_meshCache.find(sceneKey);
        if (I == g_meshCache.end())
        {
            refresh=true;
        }
        else
            glcache = (*I).second;
    }

    if(refresh == true)
    {
        if(token == kDrawBoundingBox)
        {
            if (glcache != -1)
            {
                glDeleteLists(glcache,1);
            }
            glcache = glGenLists(1);
            glNewList(glcache, MGL_COMPILE);
            drawBoundingBox( request, view );
            glEndList();
            g_bboxCache[sceneKey] = glcache;
        }
        else
        {
            if (glcache != -1)
            {
                glDeleteLists(glcache, 1);
            }
            glcache = glGenLists(1);
            glNewList(glcache, MGL_COMPILE);
            if (cache->abcSceneManager.hasKey(sceneKey))
                if (MGlobal::mayaState() == MGlobal::kInteractive)
                    cache->abcSceneManager.getScene(sceneKey)->draw(cache->abcSceneState);
            glEndList();
            g_meshCache[sceneKey] = glcache;

        }
    }

    switch (token)
    {
    case kDrawBoundingBox :
        glCallList(glcache);
        break;

    case kDrawWireframe:
    case kDrawWireframeOnShaded:

        gGLFT->glPolygonMode(GL_FRONT_AND_BACK, MGL_LINE);
            gGLFT->glPushMatrix();
            glCallList(glcache);
            gGLFT->glPopMatrix();
        gGLFT->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    case kDrawSmoothShaded:
    case kDrawFlatShaded:
        MMaterial material = request.material();
        material.setMaterial( request.multiPath(), request.isTransparent() );
            gGLFT->glPushMatrix();
            glCallList(glcache);
            gGLFT->glPopMatrix();
        break;
    }

    if(selectionKey != "")
    {
        GLuint glcacheSel = glGenLists(1);
        glNewList(glcacheSel, MGL_COMPILE);
        if (cache->abcSceneManager.hasKey(sceneKey))
            if (MGlobal::mayaState() == MGlobal::kInteractive)
                cache->abcSceneManager.getScene(sceneKey)->drawOnly(cache->abcSceneState, selectionKey);
        glEndList();

        switch (cache->token)
        {
        case kDrawBoundingBox :
            break;
        case kDrawWireframe:
        case kDrawWireframeOnShaded:
            gGLFT->glPolygonMode(GL_FRONT_AND_BACK, MGL_LINE);
            gGLFT->glPushMatrix();
            glCallList(glcacheSel);
            gGLFT->glPopMatrix();
            gGLFT->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        case kDrawSmoothShaded:
        case kDrawFlatShaded:
            MMaterial material = request.material();
            material.setMaterial( request.multiPath(), request.isTransparent() );
            gGLFT->glPushMatrix();
            glCallList(glcacheSel);
            gGLFT->glPopMatrix();
            break;
        }
    }

    cache->m_abcdirty = false;
    glPopAttrib();
    view.endGL();
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
                queue.add( wireRequest );
        }
}

bool CAlembicHolderUI::select(MSelectInfo &selectInfo,
        MSelectionList &selectionList, MPointArray &worldSpaceSelectPts) const
//
// Select function. Gets called when the bbox for the object is selected.
// This function just selects the object without doing any intersection tests.
//
{

    //cout << "select" << endl;
    MSelectionMask priorityMask(MSelectionMask::kSelectObjectsMask);
    MSelectionList item;
    item.add(selectInfo.selectPath());
    MPoint xformedPt;
    selectInfo.addSelection(item, xformedPt, selectionList,
            worldSpaceSelectPts, priorityMask, false);
    return true;
}
