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

#ifndef _AlembicHolder_Foundation_h_
#define _AlembicHolder_Foundation_h_

#include <Alembic/AbcGeom/All.h>
//#include <Alembic/GLUtil/All.h>
#include <Alembic/Util/All.h>

#include <OpenEXR/ImathMath.h>
#include <OpenEXR/ImathVec.h>
#include <OpenEXR/ImathMatrix.h>
#include <OpenEXR/ImathBox.h>
#include <OpenEXR/ImathQuat.h>
#include <OpenEXR/ImathColor.h>
#include <OpenEXR/ImathFun.h>
#include <OpenEXR/ImathBoxAlgo.h>

/*#include <boost/format.hpp>
#include <boost/preprocessor/stringize.hpp>*/
#include <boost/timer.hpp>
//#include <boost/utility.hpp>
#include <boost/functional/hash.hpp>

#include <iostream>
#include <algorithm>
#include <utility>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <exception>
#include <string>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <maya/MHardwareRenderer.h>
#include <maya/MGLFunctionTable.h>
#include <maya/MPoint.h>
#include <maya/MMatrix.h>
#include <maya/MBoundingBox.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>
#else
#define stat _stat
#endif


namespace AlembicHolder {

//-*****************************************************************************
namespace Abc = Alembic::AbcGeom;
using namespace Abc;

//-*****************************************************************************
//-*****************************************************************************
// FUNCTIONS
//-*****************************************************************************
//-*****************************************************************************

//-*****************************************************************************
template <class T>
inline T degrees( const T &rads ) { return 180.0 * rads / M_PI; }

//-*****************************************************************************
template <class T>
inline T radians( const T &degs ) { return M_PI * degs / 180.0; }

//-*****************************************************************************
template <class T>
inline const T &clamp( const T &x, const T &lo, const T &hi )
{
    return x < lo ? lo : x > hi ? hi : x;
}

template <typename T>
inline MColor mayaFromImath(const Imath::Color3<T>& c)
{
    return MColor(c.x, c.y, c.z);
};

template <typename T>
inline MPoint mayaFromImath(const Imath::Vec3<T>& v)
{
    return MPoint(v.x, v.y, v.z);
};

template <typename T>
inline MMatrix mayaFromImath(const Imath::Matrix44<T>& m)
{
    return MMatrix(m.x);
};

template <typename T>
inline MBoundingBox mayaFromImath(const Imath::Box<T>& bbox)
{
    return MBoundingBox(mayaFromImath(bbox.min), mayaFromImath(bbox.max));
}

//-*****************************************************************************
//-*****************************************************************************
// GL ERROR CHECKING
//-*****************************************************************************
//-*****************************************************************************
inline void GL_CHECK( const std::string &header = "" )
{
    static MGLFunctionTable *gGLFT = NULL;
    if (gGLFT == NULL)
        gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();

    MGLenum errCode = gGLFT->glGetError();
    if ( errCode != MGL_NO_ERROR )
    {
//        std::cerr << "GL ERROR in " << header << ": "
//                  << ( const char * )gluErrorString( errCode )
//                  << std::endl;

        MGLint matrixStackDepth = 0;
        gGLFT->glGetIntegerv( MGL_MODELVIEW_STACK_DEPTH, &matrixStackDepth );
//        std::cout << "Stack depth: " << ( int )matrixStackDepth
//                  << std::endl;
    }
}

#if 0

//-*****************************************************************************
#define GL_NOISY( CODE )                        \
do                                              \
{                                               \
    CODE ;                                      \
    GL_CHECK( BOOST_PP_STRINGIZE( CODE ) );     \
    std::cout << "EXECUTED:"                    \
              << BOOST_PP_STRINGIZE( CODE )     \
              << std::endl;                     \
}                                               \
while( 0 )

#else

//-*****************************************************************************
#define GL_NOISY( CODE )                        \
do                                              \
{                                               \
    CODE ;                                      \
    std::string msg = "Code: ";                 \
    msg += BOOST_PP_STRINGIZE( CODE );          \
    GL_CHECK( msg );                            \
}                                               \
while( 0 )

#endif


// Stores a view to a [start, start+size) memory range.
template <typename T>
struct Span {
    T* start;
    size_t size;

    Span() : start(nullptr), size(0) {}
    Span(T* start_, size_t size_) : start(start_), size(size_) {}
    template <typename U>
    Span(const Span<U>& s) : start(s.start), size(s.size) {}
    template <typename U>
    explicit Span(std::vector<U>& v) : start(v.data()), size(v.size()) {}
    template <typename U>
    explicit Span(const std::vector<U>& v) : start(v.data()), size(v.size()) {}
    template <typename U>
    explicit Span(const TypedArraySample<U>& a) : start(a.get()), size(a.size()) {}
    template <typename U>
    explicit Span(const Alembic::Util::shared_ptr<TypedArraySample<U>>& p)
        : start(p ? p->get() : nullptr), size(p ? p->size() : 0) {}

    T* begin() { return start; }
    const T* begin() const { return start; }
    T* end() { return start + size; }
    const T* end() const { return start + size; }

    bool empty() const { return start == nullptr || size == 0; }
    T& operator[](size_t index) const { assert(index < size); return start[index]; }
};

template <typename T>
bool updateValue(T& out, const T& in)
{
    if (out == in)
        return false;
    out = in;
    return true;
}

// Stores a file path a hash computed from the path, the file's creation time
// and last modification time.
class FileRef {
public:
    FileRef() : m_hash(0) {}
    FileRef(const std::string& path)
        : m_path(path), m_hash(computeHash(path))
    {}
    const std::string& path() const { return m_path; }
    size_t hash() const { return m_hash; }
    bool operator==(const FileRef& rhs) const { return m_path == rhs.m_path && m_hash == rhs.m_hash; }
    bool operator!=(const FileRef& rhs) const { return !(*this == rhs); }
private:
    std::string m_path;
    size_t m_hash;

    static size_t computeHash(const std::string& path)
    {
        auto hash = std::hash<std::string>()(path);
        struct stat file_stat;
        if (stat(path.c_str(), &file_stat) != 0) {
            return hash;
        }
        boost::hash_combine(hash, file_stat.st_ctime);
        boost::hash_combine(hash, file_stat.st_mtime);
        return hash;
    }
};

} // End namespace AlembicHolder

namespace std {
    template<>
    struct hash<AlembicHolder::FileRef> {
        size_t operator()(const AlembicHolder::FileRef& file_hash) const
        {
            return file_hash.hash();
        }
    };
} // namespace std

#endif
