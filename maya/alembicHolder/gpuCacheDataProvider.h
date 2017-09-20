#pragma once

#include "gpuCacheTimeInterval.h"
#include "gpuCacheSample.h"

#include <Alembic/AbcCoreAbstract/TimeSampling.h>
#include <Alembic/AbcGeom/IPolyMesh.h>
#include <Alembic/AbcGeom/IXform.h>

#include <maya/MBoundingBox.h>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>


namespace AlembicHolder {
namespace CacheReaderAlembicPrivate {

typedef Alembic::Abc::index_t           index_t;
typedef Alembic::Abc::chrono_t          chrono_t;
typedef Alembic::AbcGeom::IXformSchema  IXformSchema;
typedef Alembic::AbcGeom::XformSample   XformSample;
typedef Alembic::AbcGeom::XformOp       XformOp;
typedef Alembic::AbcCoreAbstract::TimeSamplingPtr TimeSamplingPtr;


//==============================================================================
// CLASS BaseTypeOfElem
//==============================================================================

template <class ElemType>
struct BaseTypeOfElem
{
    // When the element type is already a POD...
    typedef ElemType value_type;
    static const size_t kDimensions = 1;
};

// Alembic stores index buffer as signed integers, while this
// plug-in handles them as unsigned integers...
template <>
struct BaseTypeOfElem<int32_t>
{
    // When the element type is already a POD...
    typedef uint32_t value_type;
    static const size_t kDimensions = 1;
};

template <class T>
struct BaseTypeOfElem<Imath::Vec2<T> >
{
    typedef T value_type;
    static const size_t kDimensions = 2;
};

template <class T>
struct BaseTypeOfElem<Imath::Vec3<T> >
{
    typedef T value_type;
    static const size_t kDimensions = 3;
};


//==============================================================================
// CLASS AlembicArray
//==============================================================================

// A wrapper around alembic sample arrays
template <class ArrayProperty>
class AlembicArray :
    public ReadableArray<
    typename BaseTypeOfElem<
    typename ArrayProperty::traits_type::value_type
    >::value_type
    >
{
public:

    /*----- member functions -----*/
    typedef typename ArrayProperty::sample_ptr_type ArraySamplePtr;
    typedef typename ArrayProperty::traits_type traits_type;
    typedef typename BaseTypeOfElem<typename traits_type::value_type>::value_type T;
    typedef typename Array<T>::Digest Digest;

    static const size_t kDimensions =
        BaseTypeOfElem<typename traits_type::value_type>::kDimensions;

    // Returns a pointer to an Array that has the same content as the
    // buffer passed-in as determined by the computed digest hash-key.
    static boost::shared_ptr<ReadableArray<T> > create(
        const ArraySamplePtr& arraySamplePtr, const Digest& digest);

    virtual ~AlembicArray();
    virtual const T* get() const;

private:

    /*----- member functions -----*/

    // The constructor is declare private to force user to go through
    // the create() factory member function.
    GPUCACHE_DECLARE_MAKE_SHARED_FRIEND_2;

    AlembicArray(
        const ArraySamplePtr& arraySamplePtr, const Digest& digest
    )
        : ReadableArray<T>(
            arraySamplePtr->size() * kDimensions, digest),
        fArraySamplePtr(arraySamplePtr)
    {
        assert(traits_type::dataType().getExtent() == kDimensions);
    }

    /*----- data members -----*/

    const ArraySamplePtr    fArraySamplePtr;
};


//==============================================================================
// CLASS PropertyCache
//==============================================================================

template <typename PROPERTY, typename KEY, typename VALUE, typename DERIVED>
class PropertyCache
{
public:
    typedef PROPERTY    Property;
    typedef KEY         Key;
    typedef VALUE       Value;
    typedef DERIVED     Derived;

    PropertyCache()
        : fValidityInterval(TimeInterval::kInvalid)
    {}

    void reset()
    {
        fProperty = Property();
        fValidityInterval = TimeInterval(TimeInterval::kInvalid);
        fValue = Value();
    }

    bool valid() const
    {
        return fProperty.valid();
    }

    void init(const Property& property)
    {
        this->fProperty = property;

        const index_t numSamples = this->fProperty.getNumSamples();
        const TimeSamplingPtr sampling = this->fProperty.getTimeSampling();

        if (this->fProperty.isConstant()) {
            // Delay the read of constant properties until the first call to setTime()
            fValidityInterval = TimeInterval(TimeInterval::kInvalid);
        }
        else {
            // We need to read-in all the sample keys in
            // sequential order to determine which keys are
            // truly unique. This has to be done at init time
            // because later on, it is possible that the samples
            // be asked in random order and it will be difficult
            // to determine the validity interval of the returned
            // sample.

            // There is always a sample at index 0!
            this->fUniqueSampleIndexes.push_back(0);
            this->fTimeBoundaries.push_back(-std::numeric_limits<chrono_t>::infinity());
            Key prevKey;
            static_cast<Derived*>(this)->getKey(prevKey, 0);
            for (index_t i = 1; i<numSamples; ++i) {
                Key key;
                static_cast<Derived*>(this)->getKey(key, i);

                if (key != prevKey) {
                    this->fUniqueSampleIndexes.push_back(i);
                    // We store the time at which a sample stop being
                    // valid. This is reprented by the mid-way point
                    // between 2 samples.
                    this->fTimeBoundaries.push_back(
                        0.5 * (sampling->getSampleTime(i - 1) + sampling->getSampleTime(i))
                    );
                    prevKey = key;
                }
            }
            this->fTimeBoundaries.push_back(+std::numeric_limits<chrono_t>::infinity());
        }
    }

    bool setTime(chrono_t time)
    {
        if (fProperty.isConstant())
        {
            // Delayed read of constant properties.
            if (!fValidityInterval.valid()) {
                static_cast<Derived*>(this)->readValue(0);
                // If an IXform node is constant identity, getNumSamples() returns 0
                fValidityInterval = TimeInterval(TimeInterval::kInfinite);
            }
            return false;
        }

        if (fValidityInterval.contains(time)) {
            return false;
        }

        std::vector<chrono_t>::const_iterator bgn = fTimeBoundaries.begin();
        std::vector<chrono_t>::const_iterator end = fTimeBoundaries.end();
        std::vector<chrono_t>::const_iterator it = std::upper_bound(
            bgn, end, time);
        assert(it != bgn);

        index_t idx = fUniqueSampleIndexes[std::distance(bgn, it) - 1];

        // Do this first for exception safety.
        static_cast<Derived*>(this)->readValue(idx);

        // Ok, we have successfully read the value. We can
        // now update the time information.
        fValidityInterval = TimeInterval(*(it - 1), *it);
        return true;
    }

    const Value& getValue() const
    {
        return fValue;
    }

    TimeInterval getValidityInterval() const
    {
        return fValidityInterval;
    }

protected:
    Property              fProperty;
    std::vector<index_t>  fUniqueSampleIndexes;
    std::vector<chrono_t> fTimeBoundaries;

    TimeInterval          fValidityInterval;
    Value                 fValue;
};


//==============================================================================
// CLASS ScalarPropertyCache
//==============================================================================

template <typename PROPERTY>
class ScalarPropertyCache
    : public PropertyCache<
    PROPERTY,
    typename PROPERTY::value_type,
    typename PROPERTY::value_type,
    ScalarPropertyCache<PROPERTY>
    >
{

public:
    typedef PropertyCache<
        PROPERTY,
        typename PROPERTY::value_type,
        typename PROPERTY::value_type,
        ScalarPropertyCache<PROPERTY>
    > BaseClass;
    typedef typename BaseClass::Key Key;

    void readValue(index_t idx)
    {
        // For scalar properties, the value is the key...
        getKey(this->fValue, idx);
    }

    void getKey(Key& key, index_t idx)
    {
        this->fProperty.get(key, idx);
    }

};


//==============================================================================
// CLASS XformPropertyCache
//==============================================================================

class XformPropertyCache
    : public PropertyCache<
    IXformSchema,
    MMatrix,
    MMatrix,
    XformPropertyCache
    >
{
public:
    void readValue(index_t idx)
    {
        // For xform properties, the value is the key...
        getKey(fValue, idx);
    }

    void getKey(Key& key, index_t idx)
    {
        XformSample sample;
        fProperty.get(sample, idx);
        key = toMatrix(sample);
    }

private:
    // Helper function to extract the transformation matrix out of
    // an XformSample.
    static MMatrix toMatrix(const XformSample& sample)
    {
        Alembic::Abc::M44d matrix = sample.getMatrix();
        return MMatrix(matrix.x);
    }

};


//==============================================================================
// CLASS ArrayPropertyCache
//==============================================================================

template <typename PROPERTY>
class ArrayPropertyCache
    : public PropertyCache<
    PROPERTY,
    Alembic::AbcCoreAbstract::ArraySampleKey,
    boost::shared_ptr<
    ReadableArray<
    typename BaseTypeOfElem<
    typename PROPERTY::traits_type::value_type
    >::value_type> >,
    ArrayPropertyCache<PROPERTY>
    >
{
public:
    typedef PropertyCache<
        PROPERTY,
        Alembic::AbcCoreAbstract::ArraySampleKey,
        boost::shared_ptr<
        ReadableArray<
        typename BaseTypeOfElem<
        typename PROPERTY::traits_type::value_type
        >::value_type> >,
        ArrayPropertyCache<PROPERTY>
    > BaseClass;

    typedef typename BaseClass::Property Property;
    typedef typename BaseClass::Key Key;
    typedef typename BaseClass::Value Value;

    typedef typename Property::sample_ptr_type ArraySamplePtr;
    typedef typename Property::traits_type traits_type;
    typedef typename BaseTypeOfElem<typename traits_type::value_type>::value_type BaseType;
    static const size_t kDimensions =
        BaseTypeOfElem<typename traits_type::value_type>::kDimensions;

    void readValue(index_t idx)
    {
        Key key;
        getKey(key, idx);

        // Can't figure out how this can differs... It seems like a
        // provision for a future feature. Unfortunately, I can't
        // figure out if the key digest would be relative the read
        // or the orig POD!
        assert(key.origPOD == key.readPOD);
        assert(key.origPOD == traits_type::dataType().getPod());
        assert(sizeof(BaseType) == PODNumBytes(traits_type::dataType().getPod()));
        assert(kDimensions == traits_type::dataType().getExtent());

        const size_t size = size_t(key.numBytes / sizeof(BaseType));

        // First, let's try to an array out of the global
        // registry.
        //
        // Important, we first have to get rid of the previously
        // referenced value outside of the lock or else we are
        // risking a dead-lock on Linux and Mac (tbb::mutex is
        // non-recursive on these platforms).
        this->fValue = Value();
        {
            tbb::mutex::scoped_lock lock(ArrayRegistry<BaseType>::mutex());
            this->fValue = ArrayRegistry<BaseType>::lookupReadable(key.digest, size);

            if (this->fValue) return;
        }

        // Sample not found. Read it.
        ArraySamplePtr sample;
        this->fProperty.get(sample, idx);

#ifndef NDEBUG
        Key key2 = sample->getKey();
        const size_t size2 = (sample->size() *
            Property::traits_type::dataType().getExtent());
        assert(key == key2);
        assert(size == size2);
#endif

        // Insert the read sample into the cache.
        this->fValue = AlembicArray<Property>::create(sample, key.digest);
    }

    void getKey(Key& key, index_t idx)
    {
#ifndef NDEBUG
        bool result =
#endif
            this->fProperty.getKey(key, idx);
        // There should always be a key...
        assert(result);
    }
};


template <typename PROPERTY>
class ArrayPropertyCacheWithConverter
    : public PropertyCache<
    PROPERTY,
    Alembic::AbcCoreAbstract::ArraySampleKey,
    boost::shared_ptr<
    ReadableArray<
    typename BaseTypeOfElem<
    typename PROPERTY::traits_type::value_type
    >::value_type> >,
    ArrayPropertyCacheWithConverter<PROPERTY>
    >
{
public:
    typedef PropertyCache<
        PROPERTY,
        Alembic::AbcCoreAbstract::ArraySampleKey,
        boost::shared_ptr<
        ReadableArray<
        typename BaseTypeOfElem<
        typename PROPERTY::traits_type::value_type
        >::value_type> >,
        ArrayPropertyCacheWithConverter<PROPERTY>
    > BaseClass;

    typedef typename BaseClass::Property Property;
    typedef typename BaseClass::Key Key;
    typedef typename BaseClass::Value Value;

    typedef typename Property::sample_ptr_type ArraySamplePtr;
    typedef typename Property::traits_type traits_type;
    typedef typename BaseTypeOfElem<typename traits_type::value_type>::value_type BaseType;
    typedef Alembic::Util::Digest Digest;

    typedef boost::shared_ptr<ReadableArray<BaseType> >
        (*Converter)(const typename PROPERTY::sample_ptr_type& sample);

    static const size_t kDimensions =
        BaseTypeOfElem<typename traits_type::value_type>::kDimensions;

    ArrayPropertyCacheWithConverter(Converter converter)
        : fConverter(converter)
    {}

    void readValue(index_t idx)
    {
        Key key;
        getKey(key, idx);

        // Can't figure out how this can differs... It seems like a
        // provision for a future feature. Unfortunately, I can't
        // figure out if the key digest would be relative the read
        // or the orig POD!
        assert(key.origPOD == key.readPOD);
        assert(key.origPOD == traits_type::dataType().getPod());
        assert(sizeof(BaseType) == PODNumBytes(traits_type::dataType().getPod()));
        assert(kDimensions == traits_type::dataType().getExtent());

        const size_t size = size_t(key.numBytes / sizeof(BaseType));

        // First, let's try to an array out of the global
        // registry.
        //
        // Important, we first have to get rid of the previously
        // referenced value outside of the lock or else we are
        // risking a dead-lock on Linux and Mac (tbb::mutex is
        // non-recursive on these platforms).
        this->fValue = Value();
        typename ConvertionMap::const_iterator it = fsConvertionMap.find(key.digest);
        if (it != fsConvertionMap.end()) {
            tbb::mutex::scoped_lock lock(ArrayRegistry<BaseType>::mutex());
            this->fValue = ArrayRegistry<BaseType>::lookupReadable(it->second, size);

            if (this->fValue) return;
        }

        // Sample not found. Read it.
        ArraySamplePtr sample;
        this->fProperty.get(sample, idx);

#ifndef NDEBUG
        Key key2 = sample->getKey();
        const size_t size2 = (sample->size() *
            Property::traits_type::dataType().getExtent());
        assert(key == key2);
        assert(size == size2);
#endif

        // Insert the read sample into the cache.
        this->fValue = fConverter(sample);

        fsConvertionMap[key.digest] = this->fValue->digest();
    }

    void getKey(Key& key, index_t idx)
    {
#ifndef NDEBUG
        bool result =
#endif
            this->fProperty.getKey(key, idx);
        // There should always be a key...
        assert(result);
    }

private:

    struct DigestHash : std::unary_function<Digest, std::size_t>
    {
        std::size_t operator()(Digest const& v) const
        {
            std::size_t seed = 0;
            boost::hash_combine(seed, v.words[0]);
            boost::hash_combine(seed, v.words[1]);
            return seed;
        }
    };

    typedef boost::unordered_map<Digest, Digest, DigestHash> ConvertionMap;
    static ConvertionMap fsConvertionMap;

    const Converter fConverter;
};


//==============================================================================
// CLASS DataProvider
//==============================================================================

// An abstract class to wrap the details of different data sources.
// Currently, we have four kinds of Alembic geometries:
//     1) IPolyMesh from gpuCache command
//     2) IPolyMesh from arbitrary Alembic exporter such as AbcExport
//     3) INuPatch  from arbitrary Alembic exporter such as AbcExport
//     4) ISubD     from arbitrary Alembic exporter such as AbcExport
// Of course, 1) is much faster than 2).
// Caller is responsible for locking.
//
class DataProvider
{
public:
    virtual ~DataProvider();

    // Check if all the properties are valid
    virtual bool valid() const;

    // The following two methods are used when reading hierarchy.
    //
    // Fill minimal property caches that will display a bounding box place holder
    void fillBBoxAndVisSample(chrono_t time);

    // Return the validity interval of a bounding box place holder sample
    TimeInterval getBBoxAndVisValidityInterval() const { return fBBoxAndVisValidityInterval; }

    // The following two methods are used when reading shapes.
    //
    // Fill property caches with the data at the specified index
    void fillTopoAndAttrSample(chrono_t time);

    // Return the combined validity interval of the the property
    // caches for the last updated index.
    TimeInterval getValidityInterval() const { return fValidityInterval; }


    // Check for the visibility
    bool isVisible() const;

    // Retrieve the current bounding box proxy sample from property cache
    boost::shared_ptr<const ShapeSample>
    getBBoxPlaceHolderSample(double seconds);

    // Retrieve the current sample from property cache
    virtual boost::shared_ptr<const ShapeSample>
    getSample(double seconds) = 0;

    // Return the bounding box and validity interval for the current
    // sample, i.e. the time of the last call to sample.
    MBoundingBox getBoundingBox() const
    {
        const Imath::Box<Imath::V3d> boundingBox =
            fBoundingBoxCache.getValue();
        if (boundingBox.isEmpty()) {
            return MBoundingBox();
        }
        return MBoundingBox(
            MPoint(boundingBox.min.x, boundingBox.min.y, boundingBox.min.z),
            MPoint(boundingBox.max.x, boundingBox.max.y, boundingBox.max.z));
    }

    TimeInterval getBoundingBoxValidityInterval() const
    { return fBoundingBoxCache.getValidityInterval(); }

    TimeInterval getAnimTimeRange() const
    { return fAnimTimeRange; }

protected:
    // Prohibited and not implemented.
    DataProvider(const DataProvider&);
    const DataProvider& operator= (const DataProvider&);

    template<class INFO>
    DataProvider(Alembic::AbcGeom::IGeomBaseSchema<INFO>& abcGeom,
                 Alembic::Abc::TimeSamplingPtr            timeSampling,
                 size_t numSamples,
                 bool   needUVs);

    // Update Bounding Box and Visibility property caches
    TimeInterval updateBBoxAndVisCache(chrono_t time);

    // Update the property caches
    virtual TimeInterval updateCache(chrono_t time);

    // Whether UV coordinates should be read or generated.
    const bool fNeedUVs;

    // Exact animation time range
    TimeInterval fAnimTimeRange;

    // The valid range of bbox and visibility in property caches
    TimeInterval fBBoxAndVisValidityInterval;

    // The valid range of the current data in property caches
    TimeInterval fValidityInterval;

    // Shape Visibility
    ScalarPropertyCache<Alembic::Abc::ICharProperty>  fVisibilityCache;

    // Bounding Box
    ScalarPropertyCache<Alembic::Abc::IBox3dProperty> fBoundingBoxCache;

    // Parent Visibility
    std::vector<ScalarPropertyCache<Alembic::Abc::ICharProperty> > fParentVisibilityCache;
};


//==============================================================================
// CLASS PolyDataProvider
//==============================================================================

// Base class for all polygon data sources.
//
class PolyDataProvider : public DataProvider
{
public:
    virtual ~PolyDataProvider();

    // Check if all the properties are valid
    virtual bool valid() const;

protected:
    // Prohibited and not implemented.
    PolyDataProvider(const PolyDataProvider&);
    const PolyDataProvider& operator= (const PolyDataProvider&);

    template<class SCHEMA>
    PolyDataProvider(SCHEMA&                         abcMesh,
                     bool                            needUVs);

    // Update the property caches
    virtual TimeInterval updateCache(chrono_t time);

    // Polygons
    ArrayPropertyCache<Alembic::Abc::IInt32ArrayProperty> fFaceCountsCache;
    ArrayPropertyCache<Alembic::Abc::IP3fArrayProperty>   fPositionsCache;
};


//==============================================================================
// CLASS Triangulator
//==============================================================================

// This class reads mesh data that is written by an arbitrary Alembic exporter.
// Triangulate a polygon mesh and convert multi-indexed streams to single-indexed streams.
//
class Triangulator : public PolyDataProvider
{
public:
    Triangulator(Alembic::AbcGeom::IPolyMeshSchema& abcMesh,
                 bool needUVs);
    virtual ~Triangulator();

    // Check if all the properties are valid
    virtual bool valid() const;

    // Retrieve the current sample from property cache
    virtual boost::shared_ptr<const ShapeSample>
    getSample(double seconds);

protected:
    // Prohibited and not implemented.
    Triangulator(const Triangulator&);
    const Triangulator& operator= (const Triangulator&);

    // Update the property caches
    virtual TimeInterval updateCache(chrono_t time);

    // Polygon Indices
    ArrayPropertyCache<Alembic::Abc::IInt32ArrayProperty> fFaceIndicesCache;

    // Normals
    Alembic::AbcGeom::GeometryScope                        fNormalsScope;
    ArrayPropertyCache<Alembic::Abc::IN3fArrayProperty>    fNormalsCache;
    ArrayPropertyCache<Alembic::Abc::IUInt32ArrayProperty> fNormalIndicesCache;

    // UVs
    Alembic::AbcGeom::GeometryScope                        fUVsScope;
    ArrayPropertyCache<Alembic::Abc::IV2fArrayProperty>    fUVsCache;
    ArrayPropertyCache<Alembic::Abc::IUInt32ArrayProperty> fUVIndicesCache;

    // Display properties.
    MColor fDiffuseColor;
    MString fTexturePath;

private:
    template<size_t SIZE>
    boost::shared_ptr<ReadableArray<float> > convertMultiIndexedStream(
        boost::shared_ptr<ReadableArray<float> > attribArray,
        boost::shared_ptr<ReadableArray<IndexBuffer::index_t> > indexArray);

    void check();
    void computeNormals();
    void convertMultiIndexedStreams();
    void remapVertAttribs();
    void computeWireIndices();
    void triangulate();

    // compute in check();
    Alembic::AbcGeom::GeometryScope                 fCheckedNormalsScope;
    boost::shared_ptr<ReadableArray<float> >                fCheckedNormals;
    boost::shared_ptr<ReadableArray<IndexBuffer::index_t> > fCheckedNormalIndices;
    Alembic::AbcGeom::GeometryScope                 fCheckedUVsScope;
    boost::shared_ptr<ReadableArray<float> >                fCheckedUVs;
    boost::shared_ptr<ReadableArray<IndexBuffer::index_t> > fCheckedUVIndices;

    // compute in computeNormals()
    Alembic::AbcGeom::GeometryScope                 fComputedNormalsScope;
    boost::shared_ptr<ReadableArray<float> >                fComputedNormals;
    boost::shared_ptr<ReadableArray<IndexBuffer::index_t> > fComputedNormalIndices;

    // compute in convertMultiIndexedStreams()
    size_t                                          fNumVertices;
    boost::shared_array<unsigned int>               fVertAttribsIndices;
    boost::shared_ptr<ReadableArray<IndexBuffer::index_t> > fMappedFaceIndices;

    // compute in remapVertAttribs()
    boost::shared_ptr<ReadableArray<float> >                fMappedPositions;
    boost::shared_ptr<ReadableArray<float> >                fMappedNormals;
    boost::shared_ptr<ReadableArray<float> >                fMappedUVs;

    // compute in computeWireIndices()
    boost::shared_ptr<ReadableArray<IndexBuffer::index_t> > fWireIndices;

    // compute in triangulate()
    boost::shared_ptr<ReadableArray<IndexBuffer::index_t> > fTriangleIndices;
};

} // namespace CacheReaderAlembicPrivate
} // namespace AlembicHolder
