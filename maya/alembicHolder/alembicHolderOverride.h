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
        const MHWRender::MFrameContext& frameContext,
        MUserData* oldData);

    static void draw(const MHWRender::MDrawContext& context, const MUserData* userData);
    static bool setupLightingGL(const MHWRender::MDrawContext& context);
    static void unsetLightingGL(const MHWRender::MDrawContext& context);


private:
    AlembicHolderOverride(const MObject& obj);
};
