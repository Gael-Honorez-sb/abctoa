#include "alembicHolderOverride.h"



#include <maya/MHWGeometryUtilities.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MUserData.h>
#include <maya/MDrawContext.h>

#include <maya/MAnimControl.h>

namespace{
    const char* shaderUniforms = "#version 150\n"
"uniform mat4 modelViewProj;\n"
"uniform vec4 scale;"
"uniform vec4 offset;"
"uniform vec4 shadeColor;\n";

    const char* vertexShader =
"in vec3 position;\n"
"void main()\n"
"{\n"
"gl_Position = modelViewProj * vec4(position * scale.xyz + offset.xyz, 1.0f);\n"
"}\n";

    const char* fragmentShader =
"out vec4 frag_color;\n"
"void main() { frag_color = shadeColor;}\n";
}

// Wireframe line style defines
#define LINE_STIPPLE_SHORTDASHED    0x0303

class UserData : public MUserData
{
public:
    UserData(nozAlembicHolder* node)
        : MUserData(false),
          fShapeNode(node),
          fSeconds(0.0),
          fIsSelected(false)
    {
        fWireframeColor[0] = 1.0f;
        fWireframeColor[1] = 1.0f;
        fWireframeColor[2] = 1.0f;
    }

    void set(const double& seconds,
             const MColor& wireframeColor,
             bool          isSelected)
    {
        fSeconds = seconds;

        fWireframeColor[0] = wireframeColor[0];
        fWireframeColor[1] = wireframeColor[1];
        fWireframeColor[2] = wireframeColor[2];

        fIsSelected = isSelected;
    }


    virtual ~UserData() {}

    nozAlembicHolder* fShapeNode;
    double           fSeconds;
    MGLfloat          fWireframeColor[3];
    bool             fIsSelected;
};



MHWRender::MPxDrawOverride* AlembicHolderOverride::Creator(const MObject& obj)
{
    return new AlembicHolderOverride(obj);
}

AlembicHolderOverride::AlembicHolderOverride(const MObject& obj) :
    MHWRender::MPxDrawOverride(obj, draw)
{
}

AlembicHolderOverride::~AlembicHolderOverride()
{
}

MHWRender::DrawAPI AlembicHolderOverride::supportedDrawAPIs() const
{
    return (MHWRender::kOpenGL); // | MHWRender::kDirectX11); TODO support dx11 later
}

bool AlembicHolderOverride::isBounded(
        const MDagPath& objPath,
        const MDagPath& cameraPath) const
{
    return true;
}

MBoundingBox AlembicHolderOverride::boundingBox(
        const MDagPath& objPath,
        const MDagPath& cameraPath) const
{
    MBoundingBox bbox = MBoundingBox(MPoint(-1.0f, -1.0f, -1.0f), MPoint(1.0f, 1.0f, 1.0f));

    MStatus status;
    MFnDependencyNode node(objPath.node(), &status);
    if (status)
    {
        CAlembicDatas* geom = dynamic_cast<nozAlembicHolder*>(node.userNode())->alembicData();
        if(geom != NULL)
            bbox = geom->bbox;
    }
    return bbox;

}

bool AlembicHolderOverride::disableInternalBoundingBoxDraw() const
{
    return true;
} 


MUserData* AlembicHolderOverride::prepareForDraw(
    const MDagPath& objPath,
    const MDagPath& cameraPath,
    #ifndef MAYA_2013
    const MHWRender::MFrameContext& frameContext,
    #endif
    MUserData* oldData
    )
{
    using namespace MHWRender;

    MObject object    = objPath.node();
    MObject transform = objPath.transform();

    // Retrieve data cache (create if does not exist)
    UserData* data = dynamic_cast<UserData*>(oldData);
    if (!data)
    {
        // get the real ShapeNode from the MObject
        MStatus status;
        MFnDependencyNode node(object, &status);
        if (status)
        {
            data = new UserData(
                dynamic_cast<nozAlembicHolder*>(node.userNode()));
        }
    }

    if (data) {
        // compute data and cache it

        const MColor wireframeColor =
            MGeometryUtilities::wireframeColor(objPath);

        const DisplayStatus displayStatus =
            MGeometryUtilities::displayStatus(objPath);
        const bool isSelected =
            (displayStatus == kActive) ||
            (displayStatus == kLead)   ||
            (displayStatus == kHilite);

        data->set(
            MAnimControl::currentTime().as(MTime::kSeconds),
            wireframeColor,
            isSelected);
    }

    return data;
}


void AlembicHolderOverride::draw(const MHWRender::MDrawContext& context, const MUserData* userData)
{
    const UserData* data = dynamic_cast<const UserData*>(userData);

    // get state data
    MStatus status;
    const MMatrix transform =
        context.getMatrix(MHWRender::MDrawContext::kWorldViewMtx, &status);
    if (status != MStatus::kSuccess) return;
    const MMatrix projection =
        context.getMatrix(MHWRender::MDrawContext::kProjectionMtx, &status);
    if (status != MStatus::kSuccess) return;
    const int displayStyle = context.getDisplayStyle();

    // get renderer
    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer) return;

    if (theRenderer->drawAPIIsOpenGL())
    {


        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadMatrixd(projection.matrix[0]);


        // set world matrix
        glMatrixMode(MGL_MODELVIEW);
        glPushMatrix();
        glLoadMatrixd(transform.matrix[0]);


        nozAlembicHolder* shapeNode = data->fShapeNode;

        std::string sceneKey = shapeNode->getSceneKey();
        CAlembicDatas* cache = shapeNode->alembicData();
        std::string selectionKey = shapeNode->getSelectionKey();

        // draw bounding box
        if(selectionKey != "" || displayStyle == 0)
        {

            MBoundingBox box = shapeNode->boundingBox();
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

        if(displayStyle != 0)
        {
            if(displayStyle & MHWRender::MDrawContext::kWireFrame || data->fIsSelected)
            {
                glColor3fv(data->fWireframeColor);
                glPolygonMode(GL_FRONT_AND_BACK, MGL_LINE);
                if (cache->abcSceneManager.hasKey(sceneKey))
                    if(selectionKey != "")
                        cache->abcSceneManager.getScene(sceneKey)->drawOnly(cache->abcSceneState, selectionKey);
                    else
                        cache->abcSceneManager.getScene(sceneKey)->draw(cache->abcSceneState);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
        }

        if(displayStyle & MHWRender::MDrawContext::kGouraudShaded)
        {
            glPushMatrix();
            bool anyLights = setupLightingGL(context);
            glPopMatrix();
            //glPushAttrib( GL_CURRENT_BIT );

            const bool drawAsBlack = !anyLights;

            MColor diffuseColor(0.7, 0.7, 0.7, 1.0f);

            if(drawAsBlack)
                glColor4f(0.0f, 0.0f,0.0f, 1.0f);
             else {
            // set colour
            glColor4f(.7f,.7f,.7f,1.0f);
            }

            if (cache->abcSceneManager.hasKey(sceneKey))
            {
                if(selectionKey != "")
                    cache->abcSceneManager.getScene(sceneKey)->drawOnly(cache->abcSceneState, selectionKey);
                else
                    cache->abcSceneManager.getScene(sceneKey)->draw(cache->abcSceneState);
            }


            unsetLightingGL(context);

        }

        glMatrixMode(MGL_PROJECTION);
        glPopMatrix();
        glMatrixMode(MGL_MODELVIEW);
        glPopMatrix();
    }
}

bool AlembicHolderOverride::setupLightingGL(const MHWRender::MDrawContext& context)
{
    MStatus status;

    // Take into account only the 8 lights supported by the basic
    // OpenGL profile.
    const unsigned int nbLights =
        std::min(context.numberOfActiveLights(&status), 8u);
    if (status != MStatus::kSuccess) return false;

    if (nbLights > 0) {
        // Lights are specified in world space and needs to be
        // converted to view space.
        const MMatrix worldToView =
            context.getMatrix(MHWRender::MDrawContext::kViewMtx, &status);
        if (status != MStatus::kSuccess) return false;
        glLoadMatrixd(worldToView.matrix[0]);

        glEnable(MGL_LIGHTING);
        glColorMaterial(MGL_FRONT_AND_BACK, MGL_AMBIENT_AND_DIFFUSE);
        glEnable(MGL_COLOR_MATERIAL) ;
        glEnable(MGL_NORMALIZE) ;

        {
            const MGLfloat ambient[4]  = { 0.0f, 0.0f, 0.0f, 1.0f };
            const MGLfloat specular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

            glMaterialfv(MGL_FRONT_AND_BACK, MGL_AMBIENT,  ambient);
            glMaterialfv(MGL_FRONT_AND_BACK, MGL_SPECULAR, specular);

            glLightModelfv(MGL_LIGHT_MODEL_AMBIENT, ambient);

            // Two sided-lighting seems is always enabled in VP2.0.
            /*if (Config::emulateTwoSidedLighting()) {*/
                glLightModeli(MGL_LIGHT_MODEL_TWO_SIDE, 0);
           /* }
            else {*/
                //glLightModeli(MGL_LIGHT_MODEL_TWO_SIDE, 1);
            //}
        }

        for (unsigned int i=0; i<nbLights; ++i) {
            MFloatVector direction;
            float intensity;
            MColor color;
            bool hasDirection;
            bool hasPosition;
            MFloatPointArray positions;
            status = context.getLightInformation(
                i, positions, direction, intensity, color,
                hasDirection, hasPosition);

            if (status != MStatus::kSuccess) return false;

            // if (hasPosition)
            //     fprintf(stderr, "   -> Light%d position  = (%lf, %lf, %lf, %lf)\n",
            //             i, position[0], position[1], position[2], position[3]);
            // if (hasDirection)
            //     fprintf(stderr, "   -> Light%d direction = (%lf, %lf, %lf) - %lf\n",
            //             i, direction[0], direction[1], direction[2], direction.length());
            // fprintf(stderr, "   -> Light%d color = %lf x (%lf, %lf, %lf, %lf)\n",
            //         i, intensity, color[0], color[1], color[2], color[3]);

            if (hasDirection) {
                if (hasPosition) {
                    // Assumes a Maya Spot Light!
                    MFloatPoint position = positions[0];
                    const MGLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                    const MGLfloat diffuse[4] = { intensity * color[0],
                                                  intensity * color[1],
                                                  intensity * color[2],
                                                  1.0f };
                    const MGLfloat pos[4] = { position[0],
                                              position[1],
                                              position[2],
                                              1.0f };
                    const MGLfloat dir[3] = { direction[0],
                                              direction[1],
                                              direction[2]};


                    glLightfv(MGL_LIGHT0+i, MGL_AMBIENT,  ambient);
                    glLightfv(MGL_LIGHT0+i, MGL_DIFFUSE,  diffuse);
                    glLightfv(MGL_LIGHT0+i, MGL_POSITION, pos);
                    glLightfv(MGL_LIGHT0+i, MGL_SPOT_DIRECTION, dir);

                    // Maya's default value's for spot lights.
                    glLightf(MGL_LIGHT0+i,  MGL_SPOT_EXPONENT, 0.0);
                    glLightf(MGL_LIGHT0+i,  MGL_SPOT_CUTOFF,  20.0);
                }
                else {
                    // Assumes a Maya Directional Light!
                    const MGLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                    const MGLfloat diffuse[4] = { intensity * color[0],
                                                  intensity * color[1],
                                                  intensity * color[2],
                                                  1.0f };
                    const MGLfloat pos[4] = { -direction[0],
                                              -direction[1],
                                              -direction[2],
                                              0.0f };


                    glLightfv(MGL_LIGHT0+i, MGL_AMBIENT,  ambient);
                    glLightfv(MGL_LIGHT0+i, MGL_DIFFUSE,  diffuse);
                    glLightfv(MGL_LIGHT0+i, MGL_POSITION, pos);
                    glLightf(MGL_LIGHT0+i, MGL_SPOT_CUTOFF, 180.0);
                }
            }
            else if (hasPosition) {
                // Assumes a Maya Point Light!
                MFloatPoint position = positions[0];
                const MGLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                const MGLfloat diffuse[4] = { intensity * color[0],
                                              intensity * color[1],
                                              intensity * color[2],
                                              1.0f };
                const MGLfloat pos[4] = { position[0],
                                          position[1],
                                          position[2],
                                          1.0f };


                glLightfv(MGL_LIGHT0+i, MGL_AMBIENT,  ambient);
                glLightfv(MGL_LIGHT0+i, MGL_DIFFUSE,  diffuse);
                glLightfv(MGL_LIGHT0+i, MGL_POSITION, pos);
                glLightf(MGL_LIGHT0+i, MGL_SPOT_CUTOFF, 180.0);
            }
            else {
                // Assumes a Maya Ambient Light!
                const MGLfloat ambient[4] = { intensity * color[0],
                                              intensity * color[1],
                                              intensity * color[2],
                                              1.0f };
                const MGLfloat diffuse[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                const MGLfloat pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };


                glLightfv(MGL_LIGHT0+i, MGL_AMBIENT,  ambient);
                glLightfv(MGL_LIGHT0+i, MGL_DIFFUSE,  diffuse);
                glLightfv(MGL_LIGHT0+i, MGL_POSITION, pos);
                glLightf(MGL_LIGHT0+i, MGL_SPOT_CUTOFF, 180.0);
            }

            glEnable(MGL_LIGHT0+i);
        }
    }

    return nbLights > 0;
}


void AlembicHolderOverride::unsetLightingGL(
    const MHWRender::MDrawContext& context)
{
    MStatus status;

    // Take into account only the 8 lights supported by the basic
    // OpenGL profile.
    const unsigned int nbLights =
        std::min(context.numberOfActiveLights(&status), 8u);
    if (status != MStatus::kSuccess) return;

    // Restore OpenGL default values for anything that we have
    // modified.

    if (nbLights > 0) {
        for (unsigned int i=0; i<nbLights; ++i) {
            glDisable(MGL_LIGHT0+i);

            const MGLfloat ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            glLightfv(MGL_LIGHT0+i, MGL_AMBIENT,  ambient);

            if (i==0) {
                const MGLfloat diffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glLightfv(MGL_LIGHT0+i, MGL_DIFFUSE,  diffuse);

                const MGLfloat spec[4]    = { 1.0f, 1.0f, 1.0f, 1.0f };
                glLightfv(MGL_LIGHT0+i, MGL_SPECULAR, spec);
            }
            else {
                const MGLfloat diffuse[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                glLightfv(MGL_LIGHT0+i, MGL_DIFFUSE,  diffuse);

                const MGLfloat spec[4]    = { 0.0f, 0.0f, 0.0f, 1.0f };
                glLightfv(MGL_LIGHT0+i, MGL_SPECULAR, spec);
            }

            const MGLfloat pos[4]     = { 0.0f, 0.0f, 1.0f, 0.0f };
            glLightfv(MGL_LIGHT0+i, MGL_POSITION, pos);

            const MGLfloat dir[3]     = { 0.0f, 0.0f, -1.0f };
            glLightfv(MGL_LIGHT0+i, MGL_SPOT_DIRECTION, dir);

            glLightf(MGL_LIGHT0+i,  MGL_SPOT_EXPONENT,  0.0);
            glLightf(MGL_LIGHT0+i,  MGL_SPOT_CUTOFF,  180.0);
        }

        glDisable(MGL_LIGHTING);
        glDisable(MGL_COLOR_MATERIAL) ;
        glDisable(MGL_NORMALIZE) ;

        const MGLfloat ambient[4]  = { 0.2f, 0.2f, 0.2f, 1.0f };
        const MGLfloat specular[4] = { 0.8f, 0.8f, 0.8f, 1.0f };

        glMaterialfv(MGL_FRONT_AND_BACK, MGL_AMBIENT,  ambient);
        glMaterialfv(MGL_FRONT_AND_BACK, MGL_SPECULAR, specular);

        glLightModelfv(MGL_LIGHT_MODEL_AMBIENT, ambient);
        //glLightModeli(MGL_LIGHT_MODEL_TWO_SIDE, 0);
    }
}