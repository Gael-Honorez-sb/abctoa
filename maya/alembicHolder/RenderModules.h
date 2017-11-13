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

#include "Foundation.h"

#ifndef VIEWER_RENDERMODULES_HAS_BEEN_INCLUDED
#define VIEWER_RENDERMODULES_HAS_BEEN_INCLUDED

namespace AlembicHolder {

// OpenGL helper objects.

class BufferObject
{
public:
    BufferObject();
    ~BufferObject();

    void render(bool normalFlipped=false) const;

    /// @note accepted @c primType: GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP,
    /// GL_LINES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES,
    /// GL_QUAD_STRIP, GL_QUADS and GL_POLYGON
    void genIndexBuffer(const Span<const uint32_t>& indices, MGLenum primType);

    void genVertexBuffer(const Span<const V3f>& vertices);
    void genNormalBuffer(const Span<const V3f>& normals, bool flipped = false);
    void genColorBuffer(const std::vector<MGLfloat>&);

    void clear();

private:
    MGLuint mVertexBuffer, mNormalBuffer, mNormalBufferFlipped, mIndexBuffer, mColorBuffer;
    MGLenum mPrimType;
    MGLsizei mPrimNum;
};


} // namespace openvdb_viewer

#endif // OPENVDB_VIEWER_RENDERMODULES_HAS_BEEN_INCLUDED

// Copyright (c) 2012-2013 DreamWorks Animation LLC
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
