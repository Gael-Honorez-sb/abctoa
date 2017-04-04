//-*****************************************************************************
//
// Copyright (c) 2009-2010,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#ifndef _AlembicHolder_IObjectDrw_h_
#define _AlembicHolder_IObjectDrw_h_

//#include "Foundation.h"
#include "Drawable.h"
#include <maya/MAnimControl.h>
#include "cmds/ArchiveHelper.h"

namespace AlembicHolder {

//-*****************************************************************************
//! Draw an object
//! This is for an object that doesn't have any particular interpretation
//! and therefore applies no transform to its children, nor
//! does it have any particular graphic presence
class IObjectDrw : public Drawable
{
public:
    IObjectDrw( IObject &iObj, bool iResetIfNoChildren, std::vector<std::string> path );

    virtual ~IObjectDrw();

    virtual chrono_t getMinTime() const;
    virtual chrono_t getMaxTime() const;

    virtual bool valid() const;

    virtual void setTime( chrono_t iSeconds );

    virtual Box3d getBounds() const;

    virtual int getNumTriangles() const;

    virtual void draw( const DrawContext & iCtx );

    void accept(DrawableVisitor& visitor) const override
    {
        for (auto& child : m_children)
            child->accept(visitor);
    }

protected:
    IObject m_object;

    chrono_t m_minTime;
    chrono_t m_maxTime;

    chrono_t m_currentTime;

	chrono_t m_currentFrame;

    bool m_visible;

    DrawablePtrVec m_children;
    IBox3dProperty m_boundsProp;
    ISampleSelector m_ss;
    Box3d m_bounds;
};

} // End namespace AlembicHolder

#endif
