#include <maya/MPxDrawOverride.h>
#include <maya/MUserData.h>
#include <maya/MDrawContext.h>
#include <maya/MObject.h>

#include <maya/MGLFunctionTable.h>

#include "nozAlembicHolderNode.h"

class AlembicHolderOverride : public MHWRender::MPxDrawOverride
{
public:
    static MHWRender::MPxDrawOverride* Creator(const MObject& obj);

    virtual ~AlembicHolderOverride();

    virtual MHWRender::DrawAPI supportedDrawAPIs() const;

    
    virtual bool disableInternalBoundingBoxDraw() const;

    virtual bool isBounded(
        const MDagPath& objPath,
        const MDagPath& cameraPath) const;

    virtual MBoundingBox boundingBox(
        const MDagPath& objPath,
        const MDagPath& cameraPath) const;

    virtual MUserData* prepareForDraw(
        const MDagPath& objPath,
        const MDagPath& cameraPath,
        #ifndef MAYA_2013
        const MHWRender::MFrameContext& frameContext,
        #endif
        MUserData* oldData);

    static void draw(const MHWRender::MDrawContext& context, const MUserData* userData);
    static bool setupLightingGL(const MHWRender::MDrawContext& context);
    static void unsetLightingGL(const MHWRender::MDrawContext& context);


private:
    AlembicHolderOverride(const MObject& obj);
};
