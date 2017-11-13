///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2013 DreamWorks Animation LLC
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DreamWorks Animation nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// IN NO EVENT SHALL THE COPYRIGHT HOLDERS' AND CONTRIBUTORS' AGGREGATE
// LIABILITY FOR ALL CLAIMS REGARDLESS OF THEIR BASIS EXCEED US$250.00.
//
///////////////////////////////////////////////////////////////////////////

#include "RenderModules.h"
#include <math.h>


namespace AlembicHolder {

// BufferObject
static MGLFunctionTable *gGLFT = NULL;

BufferObject::BufferObject():
    mVertexBuffer(0),
    mNormalBuffer(0),
    mIndexBuffer(0),
    mColorBuffer(0),
    mPrimType(MGL_POINTS),
    mPrimNum(0)
{
    if (gGLFT == NULL)
        gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();
}

BufferObject::~BufferObject() { }

void
BufferObject::render(bool normalFlipped) const
{
    if (gGLFT == NULL || mPrimNum == 0 || !gGLFT->glIsBufferARB(mIndexBuffer) || !gGLFT->glIsBufferARB(mVertexBuffer))
        return;

    const bool usesColorBuffer = gGLFT->glIsBufferARB(mColorBuffer);
    const bool usesNormalBuffer = gGLFT->glIsBufferARB(mNormalBuffer);

    gGLFT->glBindBufferARB(MGL_ARRAY_BUFFER_ARB, mVertexBuffer);
    gGLFT->glEnableClientState(MGL_VERTEX_ARRAY);
    gGLFT->glVertexPointer(3, MGL_FLOAT, 0, 0);

    if (usesColorBuffer) {
        gGLFT->glBindBufferARB(MGL_ARRAY_BUFFER_ARB, mColorBuffer);
        gGLFT->glEnableClientState(MGL_COLOR_ARRAY);
        gGLFT->glColorPointer(3, MGL_FLOAT, 0, 0);
    }

    if (usesNormalBuffer) {      
        gGLFT->glEnableClientState(MGL_NORMAL_ARRAY);
        if(normalFlipped)
            gGLFT->glBindBufferARB(MGL_ARRAY_BUFFER_ARB, mNormalBufferFlipped);
        else
            gGLFT->glBindBufferARB(MGL_ARRAY_BUFFER_ARB, mNormalBuffer);
        gGLFT->glNormalPointer(MGL_FLOAT, 0, 0);
    }

    gGLFT->glBindBufferARB(MGL_ELEMENT_ARRAY_BUFFER_ARB, mIndexBuffer);
    gGLFT->glDrawElements(mPrimType, mPrimNum, MGL_UNSIGNED_INT, 0);

    // disable client-side capabilities
    if (usesColorBuffer) gGLFT->glDisableClientState(MGL_COLOR_ARRAY);
    if (usesNormalBuffer) gGLFT->glDisableClientState(MGL_NORMAL_ARRAY);

    // release vbo's
    gGLFT->glBindBufferARB(MGL_ARRAY_BUFFER_ARB, 0);
    gGLFT->glBindBufferARB(MGL_ELEMENT_ARRAY_BUFFER_ARB, 0);
}

void
BufferObject::genIndexBuffer(const Span<const uint32_t>& indices, MGLenum primType)
{
    if(gGLFT == NULL)
        return;

    // clear old buffer
    if (gGLFT->glIsBufferARB(mIndexBuffer) == MGL_TRUE) gGLFT->glDeleteBuffersARB(1, &mIndexBuffer);

    // gen new buffer
    gGLFT->glGenBuffersARB(1, &mIndexBuffer);
    gGLFT->glBindBufferARB(MGL_ELEMENT_ARRAY_BUFFER_ARB, mIndexBuffer);
    if (gGLFT->glIsBufferARB(mIndexBuffer) == MGL_FALSE) throw "Error: Unable to create index buffer";

    // upload data
    gGLFT->glBufferDataARB(MGL_ELEMENT_ARRAY_BUFFER_ARB,
        sizeof(MGLuint) * indices.size, indices.start, MGL_STATIC_DRAW_ARB); // upload data
    if (MGL_NO_ERROR != gGLFT->glGetError()) throw "Error: Unable to upload index buffer data";

    // release buffer
    gGLFT->glBindBufferARB(MGL_ELEMENT_ARRAY_BUFFER_ARB, 0);

    mPrimNum = indices.size;
    mPrimType = primType;
}

void
BufferObject::genVertexBuffer(const Span<const V3f>& vertices)

{
    if(gGLFT == NULL)
        return;

    if (gGLFT->glIsBufferARB(mVertexBuffer) == MGL_TRUE) gGLFT->glDeleteBuffersARB(1, &mVertexBuffer);

    gGLFT->glGenBuffersARB(1, &mVertexBuffer);
    gGLFT->glBindBufferARB(MGL_ARRAY_BUFFER_ARB, mVertexBuffer);
    if (gGLFT->glIsBufferARB(mVertexBuffer) == MGL_FALSE) throw "Error: Unable to create vertex buffer";

    gGLFT->glBufferDataARB(MGL_ARRAY_BUFFER_ARB, sizeof(MGLfloat) * 3 * vertices.size, vertices.start, MGL_STATIC_DRAW_ARB);
    if (MGL_NO_ERROR != gGLFT->glGetError()) throw "Error: Unable to upload vertex buffer data";

    gGLFT->glBindBufferARB(MGL_ARRAY_BUFFER_ARB, 0);
}

void
BufferObject::genNormalBuffer(const Span<const V3f>& normals, bool flipped)
{
    if(gGLFT == NULL)
        return;
    if(flipped)
    {
    if (gGLFT->glIsBufferARB(mNormalBuffer) == MGL_TRUE) gGLFT->glDeleteBuffersARB(1, &mNormalBufferFlipped);

        gGLFT->glGenBuffersARB(1, &mNormalBufferFlipped);
        gGLFT->glBindBufferARB(MGL_ARRAY_BUFFER_ARB, mNormalBufferFlipped);
        if (gGLFT->glIsBufferARB(mNormalBufferFlipped) == MGL_FALSE) throw "Error: Unable to create normal buffer";


    }
    else
    {
        if (gGLFT->glIsBufferARB(mNormalBuffer) == MGL_TRUE) gGLFT->glDeleteBuffersARB(1, &mNormalBuffer);

        gGLFT->glGenBuffersARB(1, &mNormalBuffer);
        gGLFT->glBindBufferARB(MGL_ARRAY_BUFFER_ARB, mNormalBuffer);
        if (gGLFT->glIsBufferARB(mNormalBuffer) == MGL_FALSE) throw "Error: Unable to create normal buffer";
    }

    gGLFT->glBufferDataARB(MGL_ARRAY_BUFFER_ARB, sizeof(MGLfloat) * 3 * normals.size, normals.start, MGL_STATIC_DRAW_ARB);
    if (MGL_NO_ERROR != gGLFT->glGetError()) throw "Error: Unable to upload normal buffer data";
    gGLFT->glBindBufferARB(MGL_ARRAY_BUFFER_ARB, 0);

}

void
BufferObject::genColorBuffer(const std::vector<MGLfloat>& v)
{
    if(gGLFT == NULL)
        return;

    if (gGLFT->glIsBufferARB(mColorBuffer) == MGL_TRUE) gGLFT->glDeleteBuffersARB(1, &mColorBuffer);

    gGLFT->glGenBuffersARB(1, &mColorBuffer);
    gGLFT->glBindBufferARB(MGL_ARRAY_BUFFER_ARB, mColorBuffer);
    if (gGLFT->glIsBufferARB(mColorBuffer) == MGL_FALSE) throw "Error: Unable to create color buffer";

    gGLFT->glBufferDataARB(MGL_ARRAY_BUFFER_ARB, sizeof(MGLfloat) * v.size(), &v[0], MGL_STATIC_DRAW_ARB);
    if (MGL_NO_ERROR != gGLFT->glGetError()) throw "Error: Unable to upload color buffer data";

    gGLFT->glBindBufferARB(MGL_ARRAY_BUFFER_ARB, 0);
}

void
BufferObject::clear()
{
    if(gGLFT != NULL)
    {
        if (gGLFT->glIsBufferARB(mIndexBuffer) == MGL_TRUE) gGLFT->glDeleteBuffersARB(1, &mIndexBuffer);
        if (gGLFT->glIsBufferARB(mVertexBuffer) == MGL_TRUE) gGLFT->glDeleteBuffersARB(1, &mVertexBuffer);
        if (gGLFT->glIsBufferARB(mColorBuffer) == MGL_TRUE) gGLFT->glDeleteBuffersARB(1, &mColorBuffer);
        if (gGLFT->glIsBufferARB(mNormalBuffer) == MGL_TRUE) gGLFT->glDeleteBuffersARB(1, &mNormalBuffer);
    }
    mPrimType = MGL_POINTS;
    mPrimNum = 0;
}

} // namespace openvdb_viewer

// Copyright (c) 2012-2013 DreamWorks Animation LLC
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
