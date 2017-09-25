#ifndef _gpuCacheMakeSharedHelper_h_
#define _gpuCacheMakeSharedHelper_h_

//-
//**************************************************************************/
// Copyright 2012 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
//**************************************************************************/
//+

/////////////////////////////////////////////////////////////////////////////// 
//
// Make Shared Helper
//
// Helpers to declare std::make_shared friend.
//
////////////////////////////////////////////////////////////////////////////////

#include <memory>

namespace AlembicHolder {

// make_shared friends for allocators
#if defined(__clang__) && !defined(__linux__)
    #define GPUCACHE_DECLARE_MAKE_SHARED_FRIEND template< class _T1, class _T2, unsigned > friend class _VSTD::__libcpp_compressed_pair_imp
#elif defined(__GNUC__) || defined(__linux__)
    #define GPUCACHE_DECLARE_MAKE_SHARED_FRIEND template< typename _Tp > friend class __gnu_cxx::new_allocator
#elif defined(_MSC_VER)
    #define GPUCACHE_DECLARE_MAKE_SHARED_FRIEND template< class _Ty > friend class std::_Ref_count_obj
#else
    #error Not implemented
#endif

} // namespace AlembicHolder

#endif

