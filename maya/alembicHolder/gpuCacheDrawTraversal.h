#ifndef _gpuCacheDrawTraversal_h_
#define _gpuCacheDrawTraversal_h_

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

#include "Drawable.h"
#include "IPolyMeshDrw.h"
#include "IXformDrw.h"
#include "gpuCacheFrustum.h"
#include "gpuCacheVBOProxy.h"

#include <boost/foreach.hpp>

namespace AlembicHolder {

//==========================================================================
// CLASS DrawTraversalState
//==========================================================================

using ::AlembicHolder::Drawable;
using ::AlembicHolder::IPolyMeshDrw;
using ::AlembicHolder::IXformDrw;

// Minimal traveral state
class DrawTraversalState
{
public:

    enum TransparentPruneType {
        kPruneNone,
        kPruneOpaque,
        kPruneTransparent
    };

    DrawTraversalState(
        const Frustum&             frustrum,
        const double               seconds,
        const TransparentPruneType transparentPrune)
        : fFrustum(frustrum),
          fSeconds(seconds),
          fTransparentPrune(transparentPrune)
    {}
    virtual ~DrawTraversalState() {}

    const Frustum&  frustum() const { return fFrustum; }
    double          seconds() const { return fSeconds; }
    TransparentPruneType transparentPrune() const { return fTransparentPrune; }

    VBOProxy&       vboProxy()      { return fVBOProxy; }

private:
    DrawTraversalState(const DrawTraversalState&);
    const DrawTraversalState& operator=(const DrawTraversalState&);

private:
    const Frustum              fFrustum;
    const double               fSeconds;
    const TransparentPruneType fTransparentPrune;
    VBOProxy                   fVBOProxy;
};


//==============================================================================
// CLASS DrawTraversal
//==============================================================================

// A traversal template implementing coordinate transformation and
// hierarchical view frustum culling. The user only to implement a
// draw function with the following signature:
//
// void draw(const boost::shared_ptr<const ShapeSample>& sample);
//
// Implemented using the "Curiously recurring template pattern".

template <class Derived, class State>
class DrawTraversal : public AlembicHolder::DrawableVisitor
{
public:

    State& state() const            { return fState; }
    const MMatrix& xform() const    { return fXform; }


protected:

    DrawTraversal(
        State&                  state,
        const MMatrix&          xform,
        bool                    isReflection,
        Frustum::ClippingResult parentClippingResult
    )
        : fState(state),
          fXform(xform),
          fIsReflection(isReflection),
          fParentClippingResult(parentClippingResult)
    {}

    bool isReflection() const { return fIsReflection; }
    const Drawable& subNode() const { return *fDrawable; }


private:

    virtual void visit(const IXformDrw&   xform)
    {
        /*
        if (state().transparentPrune() == State::kPruneOpaque &&
                subNode.transparentType() == Drawable::kOpaque) {
            return;
        }
        else if (state().transparentPrune() == State::kPruneTransparent &&
                subNode.transparentType() == Drawable::kTransparent) {
            return;
        }
        */

        if (!xform.isVisible()) return;

        // Perform view frustum culling
        // All bounding boxes are already in the axis of the root xform sub-node
        Frustum::ClippingResult clippingResult = Frustum::kInside;
        if (fParentClippingResult != Frustum::kInside) {
            // Only check the bounding box if the bounding box intersects with
            // the view frustum
            clippingResult = state().frustum().test(
                    xform.getBoundsMaya(), fParentClippingResult);

            // Prune this sub-hierarchy if the bounding box is
            // outside the view frustum
            if (clippingResult == Frustum::kOutside) {
                return;
            }
        }

        // Flip the global reflection flag back-and-​forth depending on
        // the reflection of the local matrix.
        Derived traversal(fState, xform.getMMatrix() * fXform,
                          bool(fIsReflection ^ xform.isReflection()),
                          clippingResult);

        // Recurse into children sub nodes. Expand all instances.
        BOOST_FOREACH(const DrawablePtr& child,
                      xform.getChildren() ) {
            child->accept(traversal);
        }
    }

    virtual void visit(const IPolyMeshDrw&   shape)
    {
        const boost::shared_ptr<const ShapeSample>& sample =
            shape.getSample(fState.seconds());
        if (!sample) return;

        fDrawable = &shape;
        static_cast<Derived*>(this)->draw(sample);
    }

private:
    DrawTraversal(const DrawTraversal&);
    const DrawTraversal& operator=(const DrawTraversal&);


private:

    State&                        fState;
    const Drawable*               fDrawable;
    const MMatrix                 fXform;
    const bool                    fIsReflection;
    const Frustum::ClippingResult fParentClippingResult;
};

} // namespace AlembicHolder

#endif
