#pragma once

#include "Drawable.h"
#include "IXformDrw.h"
#include "IPolyMeshDrw.h"
#include "gpuCacheSample.h"
#include <boost/shared_ptr.hpp>
#include <maya/MBoundingBox.h>

namespace AlembicHolder {

typedef Drawable SubNode;
typedef DrawablePtr SubNodePtr;

/*==============================================================================
 * CLASS SubNodeVisitor
 *============================================================================*/

// Visitor for sub nodes.
//
// The visitor dispatches on the sub node data type, i.e. transform vs
// shape. It is up to the visitor to recurse into the children of the
// sub node. This allows the visitor to control the traversal of the
// sub nodes. Note that this is somewhat different from the canonical
// visitor design pattern.
typedef DrawableVisitor SubNodeVisitor;
//class SubNodeVisitor
//{
//public:
//    virtual void visit(const XformData&   xform,
//                       const SubNode&     subNode) = 0;

//    virtual void visit(const ShapeData&   shape,
//                       const SubNode&     subNode) = 0;

//    virtual ~SubNodeVisitor();
//};

// This class returns the top-level bounding box of a sub-node hierarchy.
class BoundingBoxVisitor :  public SubNodeVisitor
{
public:
    BoundingBoxVisitor(double timeInSeconds)
      : fTimeInSeconds(timeInSeconds)
    {}
    virtual ~BoundingBoxVisitor() {}

    // Returns the current bounding box.
    const MBoundingBox& boundingBox() const
    { return fBoundingBox; }

    // Get the bounding box from a xform node.
    void visit(const AlembicHolder::IXformDrw& xform) override
    {
        //const std::shared_ptr<const XformSample>& sample =
        //    xform.getSample(fTimeInSeconds);
        //if (sample) {
        //    fBoundingBox = sample->boundingBox();
        //}
        fBoundingBox = xform.getBoundsMaya();
    }

    // Get the bounding box from a shape node.
    void visit(const AlembicHolder::IPolyMeshDrw& shape) override
    {
        //const std::shared_ptr<const ShapeSample>& sample =
        //    shape.getSample(fTimeInSeconds);
        //if (sample) {
        //    fBoundingBox = sample->boundingBox();
        //}
        fBoundingBox = shape.getBoundsMaya();
    }

    // Helper method to get the bounding box in one line.
    static MBoundingBox boundingBox(const SubNodePtr& subNode,
                                    const double timeInSeconds)
    {
        if (subNode) {
            BoundingBoxVisitor visitor(timeInSeconds);
            subNode->accept(visitor);
            return visitor.boundingBox();
        }
        return MBoundingBox();
    }

private:
    const double fTimeInSeconds;
    MBoundingBox fBoundingBox;
};

} // namespace AlembicHolder
