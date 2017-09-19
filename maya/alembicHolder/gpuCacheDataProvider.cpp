#include "gpuCacheDataProvider.h"

#include <Alembic/AbcGeom/Visibility.h>

#include <maya/MFloatVector.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MStringResource.h>
#include <maya/MStringResourceId.h>

#include <boost/foreach.hpp>
#include <boost/unordered_set.hpp>


namespace AlembicHolder {
namespace CacheReaderAlembicPrivate {

namespace {
    const std::string kArbGeomParamsName(".arbGeomParams");
    const std::string kDiffuseColorParamName("diffuse_color_from_attr");
    const std::string kTexturePathParamName("diffuse_texture_from_attr");
} // unnamed namespace

#define kPluginId "alembicHolder"
#define kBadNormalsMsg MStringResourceId(kPluginId, "kBadNormalsMsg",\
                "Bad normals. The number of normals does not match its scope.")
#define kBadUVsMsg MStringResourceId(kPluginId, "kBadUVsMsg",\
                "Bad UVs. The number of UVs does not match its scope.")

inline MString EncodeString(const MString& msg)
{
    MString encodedMsg;

    unsigned int   length = msg.numChars();
    const wchar_t* buffer = msg.asWChar();

    std::wstringstream stream;
    for (unsigned int i = 0; i < length; i++) {
        wchar_t ch = buffer[i];
        switch (ch) {
        case '\n': stream << L"\\n"; continue;
        case '\t': stream << L"\\t"; continue;
        case '\b': stream << L"\\b"; continue;
        case '\r': stream << L"\\r"; continue;
        case '\f': stream << L"\\f"; continue;
        case '\v': stream << L"\\v"; continue;
        case '\a': stream << L"\\a"; continue;
        case '\\': stream << L"\\\\"; continue;
        case '\"': stream << L"\\\""; continue;
        case '\'': stream << L"\\\'"; continue;
        }
        stream << ch;
    }

    std::wstring str = stream.str();
    return MString(str.c_str());
}

inline void DisplayWarning(const MString& msg)
{
    // This is the threadsafe version of MGlobal::displayWarning()
    MGlobal::executeCommandOnIdle("warning \"" + EncodeString(msg) + "\"");
}

inline void DisplayWarning(const MStringResourceId& id)
{
    // Threadsafe displayWarning() bundled with MStringResourceId
    MStatus stat;
    MString msg = MStringResource::getString(id, stat);
    DisplayWarning(msg);
}

//==============================================================================
// CLASS AlembicArray
//==============================================================================

template <class ArrayProperty>
boost::shared_ptr<ReadableArray<typename AlembicArray<ArrayProperty>::T> >
AlembicArray<ArrayProperty>::create(
    const ArraySamplePtr& arraySamplePtr, const Digest& digest)
{
    const size_t size = (arraySamplePtr->size() *
        ArrayProperty::traits_type::dataType().getExtent());
#ifndef NDEBUG
    // Compute the Murmur3 cryptographic hash-key and make sure
    // that the digest found in the Alembic file is correct.
    Digest checkDigest;
    Alembic::Util::MurmurHash3_x64_128(
        arraySamplePtr->get(), size * sizeof(T), sizeof(T), checkDigest.words);
    assert(digest == checkDigest);
#endif

    // We first look if a similar array already exists in the
    // cache. If so, we return the cached array to promote sharing as
    // much as possible.
    boost::shared_ptr<ReadableArray<T> > ret;
    {
        tbb::mutex::scoped_lock lock(ArrayRegistry<T>::mutex());

        // Only accept arrays which contain data we own.  This array may happen on a
        // worker thread, so non-readable arrays can't be converted to readable.
        ret = ArrayRegistry<T>::lookupReadable(digest, size);

        if (!ret) {
            ret = boost::make_shared<AlembicArray<ArrayProperty> >(
                arraySamplePtr, digest);
            ArrayRegistry<T>::insert(ret);
        }
    }

    return ret;
}

template <class ArrayProperty>
AlembicArray<ArrayProperty>::~AlembicArray() {}

template <class ArrayProperty>
const typename AlembicArray<ArrayProperty>::T*
AlembicArray<ArrayProperty>::get() const
{
    return reinterpret_cast<const T*>(fArraySamplePtr->get());
}


//==============================================================================
// CLASS ArrayPropertyCacheWithConverter
//==============================================================================

template <typename PROPERTY>
typename ArrayPropertyCacheWithConverter<PROPERTY>::ConvertionMap
ArrayPropertyCacheWithConverter<PROPERTY>::fsConvertionMap;

template class ArrayPropertyCacheWithConverter<
    Alembic::Abc::IInt32ArrayProperty>;


//==============================================================================
// CLASS DataProvider
//==============================================================================

template<class INFO>
DataProvider::DataProvider(
        Alembic::AbcGeom::IGeomBaseSchema<INFO>& abcGeom,
        Alembic::Abc::TimeSamplingPtr            timeSampling,
        size_t numSamples,
        bool   needUVs)
    : fAnimTimeRange(TimeInterval::kInvalid),
      fBBoxAndVisValidityInterval(TimeInterval::kInvalid),
      fValidityInterval(TimeInterval::kInvalid),
      fNeedUVs(needUVs)
{
    Alembic::Abc::IObject shapeObject = abcGeom.getObject();

    // shape visibility
    Alembic::AbcGeom::IVisibilityProperty visibility =
                    Alembic::AbcGeom::GetVisibilityProperty(shapeObject);
    if (visibility) {
        fVisibilityCache.init(visibility);
    }

    // bounding box
    fBoundingBoxCache.init(abcGeom.getSelfBoundsProperty());

    // Find parent IObjects
    std::vector<Alembic::Abc::IObject> parents;
    Alembic::Abc::IObject current = shapeObject.getParent();
    while (current.valid()) {
        parents.push_back(current);
        current = current.getParent();
    }

    // parent visibility
    fParentVisibilityCache.resize(parents.size());
    for (size_t i = 0; i < fParentVisibilityCache.size(); i++) {
        Alembic::AbcGeom::IVisibilityProperty visibilityProp =
            Alembic::AbcGeom::GetVisibilityProperty(parents[i]);
        if (visibilityProp) {
            fParentVisibilityCache[i].init(visibilityProp);
        }
    }

    // exact animation time range
    fAnimTimeRange = TimeInterval(
        timeSampling->getSampleTime(0),
        timeSampling->getSampleTime(numSamples > 0 ? numSamples-1 : 0)
    );
}

DataProvider::~DataProvider()
{
    fValidityInterval = TimeInterval::kInvalid;

    // free the property readers
    fVisibilityCache.reset();
    fBoundingBoxCache.reset();
    fParentVisibilityCache.clear();
}

bool DataProvider::valid() const
{
    return fBoundingBoxCache.valid();
}

boost::shared_ptr<const ShapeSample>
DataProvider::getBBoxPlaceHolderSample(double seconds)
{
    boost::shared_ptr<ShapeSample> sample =
        ShapeSample::createBoundingBoxPlaceHolderSample(
            seconds,
            getBoundingBox(),
            isVisible()
        );
    return sample;
}

void DataProvider::fillBBoxAndVisSample(chrono_t time)
{
    fBBoxAndVisValidityInterval = updateBBoxAndVisCache(time);
    assert(fBBoxAndVisValidityInterval.valid());
}

void DataProvider::fillTopoAndAttrSample(chrono_t time)
{
    fValidityInterval = updateCache(time);
    assert(fValidityInterval.valid());
}

TimeInterval DataProvider::updateBBoxAndVisCache(chrono_t time)
{
    // Notes:
    //
    // When possible, we try to reuse the samples from the previously
    // read sample.

    // update caches
    if (fVisibilityCache.valid()) {
        fVisibilityCache.setTime(time);
    }
    fBoundingBoxCache.setTime(time);
    BOOST_FOREACH (ScalarPropertyCache<Alembic::Abc::ICharProperty>&
            parentVisPropCache, fParentVisibilityCache) {
        if (parentVisPropCache.valid()) {
            parentVisPropCache.setTime(time);
        }
    }

    // return the new cache valid interval
    TimeInterval validityInterval(TimeInterval::kInfinite);
    if (fVisibilityCache.valid()) {
        validityInterval &= fVisibilityCache.getValidityInterval();
    }
    validityInterval &= fBoundingBoxCache.getValidityInterval();
    BOOST_FOREACH (ScalarPropertyCache<Alembic::Abc::ICharProperty>&
            parentVisPropCache, fParentVisibilityCache) {
        if (parentVisPropCache.valid()) {
            validityInterval &= parentVisPropCache.getValidityInterval();
        }
    }
    return validityInterval;
}

TimeInterval DataProvider::updateCache(chrono_t time)
{
    return updateBBoxAndVisCache(time);
}

bool DataProvider::isVisible() const
{
    // shape invisible
    if (fVisibilityCache.valid() &&
            fVisibilityCache.getValue() == char(Alembic::AbcGeom::kVisibilityHidden)) {
        return false;
    }

    // parent invisible
    BOOST_FOREACH (const ScalarPropertyCache<Alembic::Abc::ICharProperty>&
            parentVisPropCache, fParentVisibilityCache) {
        if (parentVisPropCache.valid() &&
                parentVisPropCache.getValue() == char(Alembic::AbcGeom::kVisibilityHidden)) {
            return false;
        }
    }

    // visible
    return true;
}


//==============================================================================
// CLASS PolyDataProvider
//==============================================================================

template<class SCHEMA>
PolyDataProvider::PolyDataProvider(
    SCHEMA&                         abcMesh,
    const bool                      needUVs)
  : DataProvider(abcMesh, abcMesh.getTimeSampling(),
                 abcMesh.getNumSamples(), needUVs)
{
    // polygon counts
    fFaceCountsCache.init(abcMesh.getFaceCountsProperty());

    // positions
    fPositionsCache.init(abcMesh.getPositionsProperty());
}

PolyDataProvider::~PolyDataProvider()
{
    // free the property readers
    fFaceCountsCache.reset();
    fPositionsCache.reset();
}

bool PolyDataProvider::valid() const
{
    return DataProvider::valid() &&
            fFaceCountsCache.valid() &&
            fPositionsCache.valid();
}

TimeInterval
PolyDataProvider::updateCache(chrono_t time)
{
    TimeInterval validityInterval(DataProvider::updateCache(time));

    // update caches
    fFaceCountsCache.setTime(time);
    fPositionsCache.setTime(time);

    // return the new cache valid interval
    validityInterval &= fFaceCountsCache.getValidityInterval();
    validityInterval &= fPositionsCache.getValidityInterval();
    return validityInterval;
}


//==============================================================================
// CLASS Triangulator
//==============================================================================

Triangulator::Triangulator(
    Alembic::AbcGeom::IPolyMeshSchema& abcMesh,
    const bool needUVs)
    : PolyDataProvider(abcMesh, needUVs),
    fNormalsScope(Alembic::AbcGeom::kUnknownScope), fUVsScope(Alembic::AbcGeom::kUnknownScope),
    fDiffuseColor(0.4, 0.4, 0.4, 1.0)
{
    // polygon indices
    fFaceIndicesCache.init(abcMesh.getFaceIndicesProperty());

    // optional normals
    Alembic::AbcGeom::IN3fGeomParam normals = abcMesh.getNormalsParam();
    if (normals.valid()) {
        fNormalsScope = normals.getScope();
        if (fNormalsScope == Alembic::AbcGeom::kVaryingScope ||
                fNormalsScope == Alembic::AbcGeom::kVertexScope ||
                fNormalsScope == Alembic::AbcGeom::kFacevaryingScope) {
            fNormalsCache.init(normals.getValueProperty());
            if (normals.isIndexed()) {
                fNormalIndicesCache.init(normals.getIndexProperty());
            }
        }
    }

    // optional UVs
    if (fNeedUVs) {
        Alembic::AbcGeom::IV2fGeomParam UVs = abcMesh.getUVsParam();
        if (UVs.valid()) {
            fUVsScope = UVs.getScope();
            if (fUVsScope == Alembic::AbcGeom::kVaryingScope ||
                fUVsScope == Alembic::AbcGeom::kVertexScope ||
                fUVsScope == Alembic::AbcGeom::kFacevaryingScope) {
                fUVsCache.init(UVs.getValueProperty());
                if (UVs.isIndexed()) {
                    fUVIndicesCache.init(UVs.getIndexProperty());
                }
            }
        }
    }

    // Get stored diffuse color and texture path if present.
    // These are assumed to be constant in time.
    const auto arbGeomParamsHeader = abcMesh.getPropertyHeader(kArbGeomParamsName);
    if (arbGeomParamsHeader && arbGeomParamsHeader->isCompound()) {
        const Alembic::Abc::ICompoundProperty arbGeomParamsProperty =
            Alembic::Abc::ICompoundProperty(abcMesh.getPtr(), kArbGeomParamsName);

        // Diffuse color.
        Alembic::Abc::IC3fArrayProperty diffuseColorProperty;
        try {
            diffuseColorProperty = Alembic::Abc::IC3fArrayProperty(arbGeomParamsProperty.getPtr(), kDiffuseColorParamName);
        } catch (Alembic::Util::Exception) {
        }
        const auto color_sample = diffuseColorProperty.getValue();
        if (color_sample && color_sample->size() != 0) {
            auto color = color_sample->get();
            fDiffuseColor.r = color->x;
            fDiffuseColor.g = color->y;
            fDiffuseColor.b = color->z;
            fDiffuseColor.a = 1.0f;
        }

        // Texture path.
        Alembic::Abc::IStringArrayProperty texturePathProperty;
        try {
            texturePathProperty = Alembic::Abc::IStringArrayProperty(arbGeomParamsProperty.getPtr(), kTexturePathParamName);
        } catch (Alembic::Util::Exception) {
        }
        const auto string_sample = texturePathProperty.getValue();
        if (string_sample && string_sample->size() != 0) {
            fTexturePath = MString(string_sample->get()->c_str());
        }
    }
}

Triangulator::~Triangulator()
{
    // free the property readers
    fFaceIndicesCache.reset();

    fNormalsScope = Alembic::AbcGeom::kUnknownScope;
    fNormalsCache.reset();
    fNormalIndicesCache.reset();

    fUVsScope = Alembic::AbcGeom::kUnknownScope;
    fUVsCache.reset();
    fUVIndicesCache.reset();
}

bool Triangulator::valid() const
{
    return PolyDataProvider::valid() &&
            fFaceIndicesCache.valid();
}

boost::shared_ptr<const ShapeSample>
Triangulator::getSample(double seconds)
{
    // empty mesh
    if (!fWireIndices || !fTriangleIndices) {
        boost::shared_ptr<ShapeSample> sample =
            ShapeSample::createEmptySample(seconds);
        return sample;
    }

    // triangle indices
    // Currently, we only have 1 group
    std::vector<boost::shared_ptr<IndexBuffer> > triangleVertIndices;
    triangleVertIndices.push_back(IndexBuffer::create(fTriangleIndices));


    boost::shared_ptr<ShapeSample> sample = ShapeSample::create(
        seconds,                                           // time (in seconds)
        fWireIndices->size() / 2,                          // number of wireframes
        fMappedPositions->size() / 3,                      // number of vertices
        IndexBuffer::create(fWireIndices),                 // wireframe indices
        triangleVertIndices,                               // triangle indices (1 group)
        VertexBuffer::createPositions(fMappedPositions),   // position
        getBoundingBox(),                                  // bounding box
        fDiffuseColor,                                     // diffuse color
        isVisible()
    );

    if (fMappedNormals) {
        sample->setNormals(
            VertexBuffer::createNormals(fMappedNormals));
    }

    if (fMappedUVs) {
        sample->setUVs(
            VertexBuffer::createUVs(fMappedUVs));
    }

    sample->setTexture(fTexturePath);

    return sample;
}

TimeInterval Triangulator::updateCache(chrono_t time)
{
    // update faceCounts/position cache here so that we can detect topology/position change.
    // next setTime() in DataProvider::updateCache() simply returns early
    bool topologyChanged = fFaceCountsCache.setTime(time);
    bool positionChanged = fPositionsCache.setTime(time);

    TimeInterval validityInterval(PolyDataProvider::updateCache(time));

    // update caches
    topologyChanged = fFaceIndicesCache.setTime(time) || topologyChanged;

    if (fNormalsCache.valid()) {
        fNormalsCache.setTime(time);
        if (fNormalIndicesCache.valid()) {
            fNormalIndicesCache.setTime(time);
        }
    }

    if (fUVsCache.valid()) {
        fUVsCache.setTime(time);
        if (fUVIndicesCache.valid()) {
            fUVIndicesCache.setTime(time);
        }
    }

    // return the new cache valid interval
    validityInterval &= fFaceIndicesCache.getValidityInterval();
    if (fNormalsCache.valid()) {
        validityInterval &= fNormalsCache.getValidityInterval();
        if (fNormalIndicesCache.valid()) {
            validityInterval &= fNormalIndicesCache.getValidityInterval();
        }
    }
    if (fUVsCache.valid()) {
        validityInterval &= fUVsCache.getValidityInterval();
        if (fUVIndicesCache.valid()) {
            validityInterval &= fUVIndicesCache.getValidityInterval();
        }
    }

    // do a minimal check for the consistency
    check();

    // convert the mesh to display-friend format
    if (positionChanged || topologyChanged || !fComputedNormals) {
        // recompute normals on position/topology change
        computeNormals();
    }

    if (topologyChanged || !fVertAttribsIndices) {
        // convert multi-indexed streams on topology change
        convertMultiIndexedStreams();
    }

    remapVertAttribs();

    if (topologyChanged || !fWireIndices) {
        // recompute wireframe indices on topology change
        computeWireIndices();
    }

    if (topologyChanged || !fTriangleIndices) {
        // recompute triangulation on topology change
        triangulate();
    }

    return validityInterval;
}

template<size_t SIZE>
boost::shared_ptr<ReadableArray<float> > Triangulator::convertMultiIndexedStream(
    boost::shared_ptr<ReadableArray<float> > attribArray,
    boost::shared_ptr<ReadableArray<IndexBuffer::index_t> > indexArray)
{
    // map the indexed array to direct array
    size_t numVerts                                 = indexArray->size();
    const float* srcAttribs                         = attribArray->get();
    const IndexBuffer::index_t* srcIndices          = indexArray->get();

    boost::shared_array<float> mappedAttribs(new float[numVerts * SIZE]);
    for (size_t i = 0; i < numVerts; i++) {
        for (size_t j = 0; j < SIZE; j++) {
            mappedAttribs[i * SIZE + j] = srcAttribs[srcIndices[i] * SIZE + j];
        }
    }

    return SharedArray<float>::create(mappedAttribs, numVerts * SIZE);
}

void Triangulator::check()
{
    size_t numFaceIndices = fFaceIndicesCache.getValue()->size();
    size_t numVerts       = fPositionsCache.getValue()->size() / 3;

    // Normals
    size_t numExpectedNormals = 0;
    if (fNormalsScope == Alembic::AbcGeom::kVaryingScope ||
            fNormalsScope == Alembic::AbcGeom::kVertexScope) {
        numExpectedNormals = numVerts;
    }
    else if (fNormalsScope == Alembic::AbcGeom::kFacevaryingScope) {
        numExpectedNormals = numFaceIndices;
    }

    size_t numActualNormals = 0;
    if (fNormalsCache.valid()) {
        if (fNormalIndicesCache.valid()) {
            numActualNormals = fNormalIndicesCache.getValue()->size();
        }
        else {
            numActualNormals = fNormalsCache.getValue()->size() / 3;
        }
    }

    // clear previous result
    fCheckedNormalsScope = Alembic::AbcGeom::kUnknownScope;
    fCheckedNormals.reset();
    fCheckedNormalIndices.reset();

    // forward
    if (numExpectedNormals == numActualNormals) {
        if (fNormalsCache.valid()) {
            fCheckedNormalsScope = fNormalsScope;
            fCheckedNormals      = fNormalsCache.getValue();
            if (fNormalIndicesCache.valid()) {
                fCheckedNormalIndices = fNormalIndicesCache.getValue();
            }
        }
    }
    else {
        DisplayWarning(kBadNormalsMsg);
    }

    // UVs
    size_t numExpectedUVs = 0;
    if (fUVsScope == Alembic::AbcGeom::kVaryingScope ||
        fUVsScope == Alembic::AbcGeom::kVertexScope) {
            numExpectedUVs = numVerts;
    }
    else if (fUVsScope == Alembic::AbcGeom::kFacevaryingScope) {
        numExpectedUVs = numFaceIndices;
    }

    size_t numActualUVs = 0;
    if (fUVsCache.valid()) {
        if (fUVIndicesCache.valid()) {
            numActualUVs = fUVIndicesCache.getValue()->size();
        }
        else {
            numActualUVs = fUVsCache.getValue()->size() / 2;
        }
    }

    // clear previous result
    fCheckedUVsScope = Alembic::AbcGeom::kUnknownScope;
    fCheckedUVs.reset();
    fCheckedUVIndices.reset();

    // forward
    if (numExpectedUVs == numActualUVs) {
        if (fUVsCache.valid()) {
            fCheckedUVsScope = fUVsScope;
            fCheckedUVs      = fUVsCache.getValue();
            if (fUVIndicesCache.valid()) {
                fCheckedUVIndices = fUVIndicesCache.getValue();
            }
        }
    }
    else {
        DisplayWarning(kBadUVsMsg);
    }
}

void Triangulator::computeNormals()
{
    // compute normals if the normals are missing
    // later on, we can safely assume that the normals always exist
    //
    if (fCheckedNormals && (fCheckedNormalsScope == Alembic::AbcGeom::kVaryingScope
            || fCheckedNormalsScope == Alembic::AbcGeom::kVertexScope
            || fCheckedNormalsScope == Alembic::AbcGeom::kFacevaryingScope)) {
        // the normals exist and we recognize these normals
        fComputedNormals      = fCheckedNormals;
        fComputedNormalsScope = fCheckedNormalsScope;
        fComputedNormalIndices.reset();
        if (fCheckedNormalIndices) {
            fComputedNormalIndices = fCheckedNormalIndices;
        }
        return;
    }

    // input data
    size_t numFaceCounts           = fFaceCountsCache.getValue()->size();
    const unsigned int* faceCounts = fFaceCountsCache.getValue()->get();

    const IndexBuffer::index_t* faceIndices = fFaceIndicesCache.getValue()->get();

    size_t    numPositions = fPositionsCache.getValue()->size();
    const float* positions = fPositionsCache.getValue()->get();

    size_t numVerts = numPositions / 3;

    if (numVerts == 0) {
        fComputedNormalsScope = Alembic::AbcGeom::kUnknownScope;
        fComputedNormals.reset();
        fComputedNormalIndices.reset();
        return;
    }

    // allocate buffers for the new normals
    boost::shared_array<float> computedFaceNormals(new float[numFaceCounts * 3]);
    boost::shared_array<float> computedNormals(new float[numVerts * 3]);

    // compute the face normals
    for (size_t i = 0, polyVertOffset = 0; i < numFaceCounts; polyVertOffset += faceCounts[i], i++) {
        size_t numPoints = faceCounts[i];

        // Newell's Method
        MFloatVector faceNormal(0.0f, 0.0f, 0.0f);
        for (size_t j = 0; j < numPoints; j++) {
            size_t thisJ = numPoints - j - 1;
            size_t nextJ = numPoints - ((j + 1) % numPoints) - 1;
            const float* thisPoint = &positions[faceIndices[polyVertOffset + thisJ] * 3];
            const float* nextPoint = &positions[faceIndices[polyVertOffset + nextJ] * 3];
            faceNormal.x += (thisPoint[1] - nextPoint[1]) * (thisPoint[2] + nextPoint[2]);
            faceNormal.y += (thisPoint[2] - nextPoint[2]) * (thisPoint[0] + nextPoint[0]);
            faceNormal.z += (thisPoint[0] - nextPoint[0]) * (thisPoint[1] + nextPoint[1]);
        }
        faceNormal.normalize();

        computedFaceNormals[i * 3 + 0] = faceNormal.x;
        computedFaceNormals[i * 3 + 1] = faceNormal.y;
        computedFaceNormals[i * 3 + 2] = faceNormal.z;
    }

    // compute the normals
    memset(&computedNormals[0], 0, sizeof(float) * numVerts * 3);
    for (size_t i = 0, polyVertOffset = 0; i < numFaceCounts; polyVertOffset += faceCounts[i], i++) {
        size_t numPoints = faceCounts[i];
        const float* faceNormal = &computedFaceNormals[i * 3];

        // accumulate the face normal
        for (size_t j = 0; j < numPoints; j++) {
            float* normal = &computedNormals[faceIndices[polyVertOffset + j] * 3];
            normal[0] += faceNormal[0];
            normal[1] += faceNormal[1];
            normal[2] += faceNormal[2];
        }
    }

    // normalize normals, MFloatVector functions are inline functions
    for (size_t i = 0; i < numVerts; i++) {
        float* normal = &computedNormals[i * 3];

        MFloatVector vector(normal[0], normal[1], normal[2]);
        vector.normalize();

        normal[0] = vector.x;
        normal[1] = vector.y;
        normal[2] = vector.z;
    }

    fComputedNormalsScope = Alembic::AbcGeom::kVertexScope;
    fComputedNormals      = SharedArray<float>::create(computedNormals, numVerts * 3);
    fComputedNormalIndices.reset();
}

namespace {

// This class converts multi-indexed streams to single-indexed streams.
//
template<class INDEX_TYPE, size_t MAX_NUM_STREAMS = 16>
class MultiIndexedStreamsConverter
{
public:
    typedef INDEX_TYPE index_type;

    MultiIndexedStreamsConverter(size_t numFaceIndices, const index_type* faceIndices)
        : fNumFaceIndices(numFaceIndices), fFaceIndices(faceIndices), fNumStreams(0),
        fNumVertices(0)
    {
        // position stream
        addMultiIndexedStream(faceIndices);
    }

    void addMultiIndexedStream(const index_type* indices)
    {
        // indices can be NULL, sequence 0,1,2,3,4,5... is assumed
        fStreams[fNumStreams++] = indices;
        assert(fNumStreams <= MAX_NUM_STREAMS);
    }

    void compute()
    {
        // pre-allocate buffers for the worst case
        boost::shared_array<index_type> indicesRegion(
            new index_type[fNumStreams * fNumFaceIndices]);

        // the hash map to find unique combination of multi-indices
        typedef boost::unordered_map<IndexTuple, size_t, typename IndexTuple::Hash, typename IndexTuple::EqualTo> IndicesMap;
        IndicesMap indicesMap(size_t(fNumFaceIndices / 0.75f));

        // fill the hash map with multi-indices
        size_t vertexAttribIndex = 0;  // index to the remapped vertex attribs
        boost::shared_array<index_type> mappedFaceIndices(new index_type[fNumFaceIndices]);

        for (size_t i = 0; i < fNumFaceIndices; i++) {
            // find the location in the region to copy multi-indices
            index_type* indices = &indicesRegion[i * fNumStreams];

            // make a tuple consists of indices for positions, normals, UVs..
            for (unsigned int j = 0; j < fNumStreams; j++) {
                indices[j] = fStreams[j] ? fStreams[j][i] : (index_type)i;
            }

            // try to insert the multi-indices tuple to the hash map
            IndexTuple tuple(indices, fNumStreams, (unsigned int)i);
            std::pair<typename IndicesMap::iterator, bool> ret = indicesMap.insert(std::make_pair(tuple, 0));

            if (ret.second) {
                // a success insert, allocate a vertex attrib index to the multi-index combination
                ret.first->second = vertexAttribIndex++;
            }

            // remap face indices
            mappedFaceIndices[i] = (index_type)ret.first->second;
        }

        // the number of unique combination is the size of vertex attrib arrays
        size_t numVertex = vertexAttribIndex;
        assert(vertexAttribIndex == indicesMap.size());

        // allocate memory for the indices into faceIndices
        boost::shared_array<unsigned int> vertAttribsIndices(new unsigned int[numVertex]);

        // build the indices (how the new vertex maps to the poly vert)
        BOOST_FOREACH(const typename IndicesMap::value_type& pair, indicesMap) {
            vertAttribsIndices[pair.second] = pair.first.faceIndex();
        }

        fMappedFaceIndices = mappedFaceIndices;
        fVertAttribsIndices = vertAttribsIndices;
        fNumVertices = numVertex;
    }

    unsigned int                       numStreams() { return fNumStreams; }
    size_t                             numVertices() { return fNumVertices; }
    boost::shared_array<unsigned int>& vertAttribsIndices() { return fVertAttribsIndices; }
    boost::shared_array<index_type>&   mappedFaceIndices() { return fMappedFaceIndices; }

private:
    // Prohibited and not implemented.
    MultiIndexedStreamsConverter(const MultiIndexedStreamsConverter&);
    const MultiIndexedStreamsConverter& operator= (const MultiIndexedStreamsConverter&);

    // This class represents a multi-index combination
    class IndexTuple
    {
    public:
        IndexTuple(index_type* indices, unsigned int size, unsigned int faceIndex)
            : fIndices(indices), fSize(size), fFaceIndex(faceIndex)
        {}

        const index_type& operator[](unsigned int index) const
        {
            assert(index < fSize);
            return fIndices[index];
        }

        unsigned int faceIndex() const
        {
            return fFaceIndex;
        }

        struct Hash : std::unary_function<IndexTuple, std::size_t>
        {
            std::size_t operator()(const IndexTuple& tuple) const
            {
                std::size_t seed = 0;
                for (unsigned int i = 0; i < tuple.fSize; i++) {
                    boost::hash_combine(seed, tuple.fIndices[i]);
                }
                return seed;
            }
        };

        struct EqualTo : std::binary_function<IndexTuple, IndexTuple, bool>
        {
            bool operator()(const IndexTuple& x, const IndexTuple& y) const
            {
                if (x.fSize == y.fSize) {
                    return memcmp(x.fIndices, y.fIndices, sizeof(index_type) * x.fSize) == 0;
                }
                return false;
            }
        };

    private:
        index_type*  fIndices;
        unsigned int fFaceIndex;
        unsigned int fSize;
    };

    // Input
    size_t            fNumFaceIndices;
    const index_type* fFaceIndices;

    const index_type* fStreams[MAX_NUM_STREAMS];
    unsigned int      fNumStreams;

    // Output
    size_t                            fNumVertices;
    boost::shared_array<unsigned int> fVertAttribsIndices;
    boost::shared_array<index_type>   fMappedFaceIndices;
};

// This class remaps multi-indexed vertex attribs (drop indices).
//
template<class INDEX_TYPE, size_t MAX_NUM_STREAMS = 16>
class MultiIndexedStreamsRemapper
{
public:
    typedef INDEX_TYPE index_type;

    MultiIndexedStreamsRemapper(const index_type* faceIndices,
        size_t numNewVertices, const unsigned int* vertAttribsIndices)
        : fFaceIndices(faceIndices), fNumNewVertices(numNewVertices),
        fVertAttribsIndices(vertAttribsIndices), fNumStreams(0)
    {}

    void addMultiIndexedStream(const float* attribs, const index_type* indices, bool faceVarying, int stride)
    {
        fAttribs[fNumStreams] = attribs;
        fIndices[fNumStreams] = indices;
        fFaceVarying[fNumStreams] = faceVarying;
        fStride[fNumStreams] = stride;
        fNumStreams++;
    }

    void compute()
    {
        // remap vertex attribs
        for (unsigned int i = 0; i < fNumStreams; i++) {
            const float*      attribs = fAttribs[i];
            const index_type* indices = fIndices[i];
            bool              faceVarying = fFaceVarying[i];
            int               stride = fStride[i];

            // allocate memory for remapped vertex attrib arrays
            boost::shared_array<float> mappedVertAttrib(
                new float[fNumNewVertices * stride]);

            for (size_t j = 0; j < fNumNewVertices; j++) {
                // new j-th vertices maps to polyVertIndex-th poly vert
                unsigned int polyVertIndex = fVertAttribsIndices[j];

                // if the scope is varying/vertex, need to convert poly vert
                // index to vertex index
                index_type pointOrPolyVertIndex = faceVarying ?
                    polyVertIndex : fFaceIndices[polyVertIndex];

                // look up the vertex attrib index
                index_type attribIndex = indices ?
                    indices[pointOrPolyVertIndex] : pointOrPolyVertIndex;

                if (stride == 3) {
                    mappedVertAttrib[j * 3 + 0] = attribs[attribIndex * 3 + 0];
                    mappedVertAttrib[j * 3 + 1] = attribs[attribIndex * 3 + 1];
                    mappedVertAttrib[j * 3 + 2] = attribs[attribIndex * 3 + 2];
                }
                else if (stride == 2) {
                    mappedVertAttrib[j * 2 + 0] = attribs[attribIndex * 2 + 0];
                    mappedVertAttrib[j * 2 + 1] = attribs[attribIndex * 2 + 1];
                }
                else {
                    assert(0);
                }
            }

            fMappedVertAttribs[i] = mappedVertAttrib;
        }
    }

    boost::shared_array<float>& mappedVertAttribs(unsigned int index)
    {
        assert(index < fNumStreams);
        return fMappedVertAttribs[index];
    }

private:
    // Prohibited and not implemented.
    MultiIndexedStreamsRemapper(const MultiIndexedStreamsRemapper&);
    const MultiIndexedStreamsRemapper& operator= (const MultiIndexedStreamsRemapper&);

    // Input
    const index_type*   fFaceIndices;
    size_t              fNumNewVertices;
    const unsigned int* fVertAttribsIndices;

    const float*      fAttribs[MAX_NUM_STREAMS];
    const index_type* fIndices[MAX_NUM_STREAMS];
    bool              fFaceVarying[MAX_NUM_STREAMS];
    int               fStride[MAX_NUM_STREAMS];
    unsigned int      fNumStreams;

    // Output, NULL means no change
    boost::shared_array<float> fMappedVertAttribs[MAX_NUM_STREAMS];
};

} // unnamed namespace

void Triangulator::convertMultiIndexedStreams()
{
    // convert multi-indexed streams to single-indexed streams
    // assume the scope is kVarying/kVertex/kFacevarying
    //

    // input polygons data
    size_t                      numFaceIndices = fFaceIndicesCache.getValue()->size();
    const IndexBuffer::index_t* faceIndices = fFaceIndicesCache.getValue()->get();

    // input normals
    bool                        normalFaceVarying = false;
    const IndexBuffer::index_t* normalIndices     = NULL;
    if (fComputedNormals) {
        normalFaceVarying = (fComputedNormalsScope == Alembic::AbcGeom::kFacevaryingScope);
        if (fComputedNormalIndices) {
            normalIndices = fComputedNormalIndices->get();
            assert(fComputedNormalIndices->size() == numFaceIndices);
        }
    }

    // input UV indices
    bool                        uvFaceVarying = false;
    const IndexBuffer::index_t* uvIndices     = NULL;
    if (fCheckedUVs) {
        uvFaceVarying = (fCheckedUVsScope == Alembic::AbcGeom::kFacevaryingScope);
        if (fCheckedUVIndices) {
            uvIndices = fCheckedUVIndices->get();
            assert(fCheckedUVIndices->size() == numFaceIndices);
        }
    }

    // determine the number of multi-indexed streams
    MultiIndexedStreamsConverter<IndexBuffer::index_t> converter(
            numFaceIndices, faceIndices);

    if (normalFaceVarying) {
        converter.addMultiIndexedStream(normalIndices);
    }
    if (uvFaceVarying) {
        converter.addMultiIndexedStream(uvIndices);
    }

    // only one multi-indexed streams, no need to convert it
    if (converter.numStreams() == 1) {
        fVertAttribsIndices.reset();
        fMappedFaceIndices = fFaceIndicesCache.getValue();
        fNumVertices       = fPositionsCache.getValue()->size() / 3;
        return;
    }

    // convert the multi-indexed streams
    converter.compute();

    // the mapped face indices
    fMappedFaceIndices = SharedArray<IndexBuffer::index_t>::create(
        converter.mappedFaceIndices(), numFaceIndices);

    // indices to remap streams
    fVertAttribsIndices = converter.vertAttribsIndices();
    fNumVertices        = converter.numVertices();
}

void Triangulator::remapVertAttribs()
{
    // remap vertex attribute streams according to the result of convertMultiIndexedStreams()
    // assume the scope is kVarying/kVertex/kFacevarying
    //

    // no multi-index streams, just drop indices
    if (!fVertAttribsIndices) {
        // positions is the only stream, just use it
        fMappedPositions = fPositionsCache.getValue();

        // get rid of normal indices
        if (fComputedNormals) {
            if (fComputedNormalIndices) {
                fMappedNormals = convertMultiIndexedStream<3>(
                    fComputedNormals, fComputedNormalIndices);
            }
            else {
                fMappedNormals = fComputedNormals;
            }
        }

        // get rid of UV indices
        if (fCheckedUVs) {
            if (fCheckedUVIndices) {
                fMappedUVs = convertMultiIndexedStream<2>(
                    fCheckedUVs, fCheckedUVIndices);
            }
            else {
                fMappedUVs = fCheckedUVs;
            }
        }

        return;
    }

    // input polygons data
    const float*                positions   = fPositionsCache.getValue()->get();
    const IndexBuffer::index_t* faceIndices = fFaceIndicesCache.getValue()->get();

    // input normals
    const float*                normals       = NULL;
    const IndexBuffer::index_t* normalIndices = NULL;
    if (fComputedNormals) {
        normals = fComputedNormals->get();
        if (fComputedNormalIndices) {
            normalIndices = fComputedNormalIndices->get();
        }
    }

    // input UV indices
    const float*                UVs       = NULL;
    const IndexBuffer::index_t* uvIndices = NULL;
    if (fCheckedUVs) {
        UVs = fCheckedUVs->get();
        if (fCheckedUVIndices) {
            uvIndices = fCheckedUVIndices->get();
        }
    }

    // set up multi-indexed streams remapper
    MultiIndexedStreamsRemapper<IndexBuffer::index_t> remapper(
        faceIndices, fNumVertices, fVertAttribsIndices.get());

    remapper.addMultiIndexedStream(positions, NULL, false, 3);

    if (normals) {
        remapper.addMultiIndexedStream(normals, normalIndices,
            fComputedNormalsScope == Alembic::AbcGeom::kFacevaryingScope, 3);
    }

    if (UVs) {
        remapper.addMultiIndexedStream(UVs, uvIndices,
            fCheckedUVsScope == Alembic::AbcGeom::kFacevaryingScope, 2);
    }

    // remap streams
    remapper.compute();

    fMappedPositions = SharedArray<float>::create(
        remapper.mappedVertAttribs(0), fNumVertices * 3);

    unsigned int streamIndex = 1;
    if (normals) {
        fMappedNormals = SharedArray<float>::create(
            remapper.mappedVertAttribs(streamIndex++), fNumVertices * 3);
    }

    if (UVs) {
        fMappedUVs = SharedArray<float>::create(
            remapper.mappedVertAttribs(streamIndex++), fNumVertices * 2);
    }
}

namespace {

// This class is used for generating wireframe indices.
//
template<class INDEX_TYPE>
class WireIndicesGenerator
{
public:
    typedef INDEX_TYPE index_type;

    WireIndicesGenerator(size_t numFaceCounts, const unsigned int* faceCounts,
        size_t numFaceIndices, const index_type* faceIndices,
        const index_type* mappedFaceIndices)
        : fNumFaceCounts(numFaceCounts), fFaceCounts(faceCounts),
        fNumFaceIndices(numFaceIndices), fFaceIndices(faceIndices),
        fMappedFaceIndices(mappedFaceIndices),
        fNumWires(0)
    {}

    void compute()
    {
        if (fNumFaceCounts == 0 || fNumFaceIndices == 0) {
            return;
        }

        // pre-allocate buffers for the worst case
        size_t maxNumWires = fNumFaceIndices;
        boost::unordered_set<WirePair, typename WirePair::Hash, typename WirePair::EqualTo>
            wireSet(size_t(maxNumWires / 0.75f));

        // insert all wires to the set
        size_t polyIndex = 0;
        size_t endOfPoly = fFaceCounts[polyIndex];
        for (size_t i = 0; i < fNumFaceIndices; i++) {
            // find the two vertices of the wireframe
            // v1 and v2 (face indices before splitting vertices) are hashed to
            // remove duplicated wireframe lines.
            // mappedV1 and mappedV2 are the actual indices to remapped
            // positions/normals/UVs
            index_type v1, v2, mappedV1, mappedV2;
            v1 = fFaceIndices[i];
            mappedV1 = fMappedFaceIndices[i];

            size_t v2Index;
            if (i + 1 == endOfPoly) {
                // wrap at the end of the polygon
                v2Index = i + 1 - fFaceCounts[polyIndex];
                if (++polyIndex < fNumFaceCounts) {
                    endOfPoly += fFaceCounts[polyIndex];
                }
            }
            else {
                v2Index = i + 1;
            }

            v2 = fFaceIndices[v2Index];
            mappedV2 = fMappedFaceIndices[v2Index];

            // insert
            wireSet.insert(WirePair(v1, v2, mappedV1, mappedV2));
        }

        // the number of wireframes
        size_t numWires = wireSet.size();

        // allocate buffers for wireframe indices
        boost::shared_array<index_type> wireIndices(new index_type[numWires * 2]);
        size_t wireCount = 0;
        BOOST_FOREACH(const WirePair& pair, wireSet) {
            wireIndices[wireCount * 2 + 0] = pair.fMappedV1;
            wireIndices[wireCount * 2 + 1] = pair.fMappedV2;
            wireCount++;
        }

        fNumWires = numWires;
        fWireIndices = wireIndices;
    }

    size_t                           numWires() { return fNumWires; }
    boost::shared_array<index_type>& wireIndices() { return fWireIndices; }

private:
    // Prohibited and not implemented.
    WireIndicesGenerator(const WireIndicesGenerator&);
    const WireIndicesGenerator& operator= (const WireIndicesGenerator&);

    // This class represents an unordered pair for wireframe indices
    struct WirePair
    {
        WirePair(index_type v1, index_type v2,
            index_type mappedV1, index_type mappedV2)
            : fV1(v1), fV2(v2), fMappedV1(mappedV1), fMappedV2(mappedV2)
        {}

        struct Hash : std::unary_function<WirePair, std::size_t>
        {
            std::size_t operator()(const WirePair& pair) const
            {
                std::size_t seed = 0;
                if (pair.fV1 < pair.fV2) {
                    boost::hash_combine(seed, pair.fV1);
                    boost::hash_combine(seed, pair.fV2);
                }
                else {
                    boost::hash_combine(seed, pair.fV2);
                    boost::hash_combine(seed, pair.fV1);
                }
                return seed;
            }
        };

        struct EqualTo : std::binary_function<WirePair, WirePair, bool>
        {
            bool operator()(const WirePair& x, const WirePair& y) const
            {
                if (x.fV1 < x.fV2) {
                    if (y.fV1 < y.fV2) {
                        return (x.fV1 == y.fV1 && x.fV2 == y.fV2);
                    }
                    else {
                        return (x.fV1 == y.fV2 && x.fV2 == y.fV1);
                    }
                }
                else {
                    if (y.fV1 < y.fV2) {
                        return (x.fV2 == y.fV1 && x.fV1 == y.fV2);
                    }
                    else {
                        return (x.fV2 == y.fV2 && x.fV1 == y.fV1);
                    }
                }
            }
        };

        index_type fV1, fV2;
        index_type fMappedV1, fMappedV2;
    };

    // Input
    size_t              fNumFaceCounts;
    const unsigned int* fFaceCounts;

    size_t              fNumFaceIndices;
    const index_type*   fFaceIndices;
    const index_type*   fMappedFaceIndices;

    // Output
    size_t                          fNumWires;
    boost::shared_array<index_type> fWireIndices;
};

} // unnamed namespace

void Triangulator::computeWireIndices()
{
    // compute the wireframe indices
    //

    // input data
    size_t           numFaceCounts = fFaceCountsCache.getValue()->size();
    const unsigned int* faceCounts = fFaceCountsCache.getValue()->get();

    size_t                   numFaceIndices = fFaceIndicesCache.getValue()->size();
    const IndexBuffer::index_t* faceIndices = fFaceIndicesCache.getValue()->get();
    const IndexBuffer::index_t* mappedFaceIndices = fMappedFaceIndices->get();

    // Compute wireframe indices
    WireIndicesGenerator<IndexBuffer::index_t> wireIndicesGenerator(
            numFaceCounts, faceCounts, numFaceIndices, faceIndices, mappedFaceIndices);
    wireIndicesGenerator.compute();

    if (wireIndicesGenerator.numWires() == 0) {
        fWireIndices.reset();
        return;
    }

    fWireIndices = SharedArray<IndexBuffer::index_t>::create(
        wireIndicesGenerator.wireIndices(), wireIndicesGenerator.numWires() * 2);
}

namespace {

    // This class triangulates polygons.
    //
    template<class INDEX_TYPE>
    class PolyTriangulator
{
public:
    typedef INDEX_TYPE index_type;

    PolyTriangulator(size_t numFaceCounts, const unsigned int* faceCounts,
        const index_type* faceIndices, bool faceIndicesCW,
        const float* positions, const float* normals)
        : fNumFaceCounts(numFaceCounts), fFaceCounts(faceCounts),
        fFaceIndices(faceIndices), fFaceIndicesCW(faceIndicesCW),
        fPositions(positions), fNormals(normals),
        fNumTriangles(0)
    {}

    void compute()
    {
        // empty mesh
        if (fNumFaceCounts == 0) {
            return;
        }

        // scan the polygons to estimate the buffer size
        size_t maxPoints = 0;  // the max number of vertices in one polygon
        size_t totalTriangles = 0;  // the number of triangles in the mesh

        for (size_t i = 0; i < fNumFaceCounts; i++) {
            size_t numPoints = fFaceCounts[i];

            // ignore degenerate polygon
            if (numPoints < 3) {
                continue;
            }

            // max number of points in a polygon
            maxPoints = std::max(numPoints, maxPoints);

            // the number of triangles expected in the polygon
            size_t numTriangles = numPoints - 2;
            totalTriangles += numTriangles;
        }

        size_t maxTriangles = maxPoints - 2;  // the max number of triangles in a polygon

                                              // allocate buffers for the worst case
        boost::shared_array<index_type>     indices(new index_type[maxPoints]);
        boost::shared_array<unsigned short> triangles(new unsigned short[maxTriangles * 3]);
        boost::shared_array<float>          aPosition(new float[maxPoints * 2]);
        boost::shared_array<float>          aNormal;
        if (fNormals) {
            aNormal.reset(new float[maxPoints * 3]);
        }

        boost::shared_array<index_type> triangleIndices(new index_type[totalTriangles * 3]);

        // triangulate each polygon
        size_t triangleCount = 0;  // the triangles
        for (size_t i = 0, polyVertOffset = 0; i < fNumFaceCounts; polyVertOffset += fFaceCounts[i], i++) {
            size_t numPoints = fFaceCounts[i];

            // ignore degenerate polygon
            if (numPoints < 3) {
                continue;
            }

            // no need to triangulate a triangle
            if (numPoints == 3) {
                if (fFaceIndicesCW) {
                    triangleIndices[triangleCount * 3 + 0] = fFaceIndices[polyVertOffset + 2];
                    triangleIndices[triangleCount * 3 + 1] = fFaceIndices[polyVertOffset + 1];
                    triangleIndices[triangleCount * 3 + 2] = fFaceIndices[polyVertOffset + 0];
                }
                else {
                    triangleIndices[triangleCount * 3 + 0] = fFaceIndices[polyVertOffset + 0];
                    triangleIndices[triangleCount * 3 + 1] = fFaceIndices[polyVertOffset + 1];
                    triangleIndices[triangleCount * 3 + 2] = fFaceIndices[polyVertOffset + 2];
                }
                triangleCount++;
                continue;
            }

            // 1) correct the polygon winding from CW to CCW
            if (fFaceIndicesCW)
            {
                for (size_t j = 0; j < numPoints; j++) {
                    size_t jCCW = numPoints - j - 1;
                    indices[j] = fFaceIndices[polyVertOffset + jCCW];
                }
            }
            else {
                for (size_t j = 0; j < numPoints; j++) {
                    indices[j] = fFaceIndices[polyVertOffset + j];
                }
            }

            // 2) compute the face normal (Newell's Method)
            MFloatVector faceNormal(0.0f, 0.0f, 0.0f);
            for (size_t j = 0; j < numPoints; j++) {
                const float* thisPoint = &fPositions[indices[j] * 3];
                const float* nextPoint = &fPositions[indices[(j + numPoints - 1) % numPoints] * 3];
                faceNormal.x += (thisPoint[1] - nextPoint[1]) * (thisPoint[2] + nextPoint[2]);
                faceNormal.y += (thisPoint[2] - nextPoint[2]) * (thisPoint[0] + nextPoint[0]);
                faceNormal.z += (thisPoint[0] - nextPoint[0]) * (thisPoint[1] + nextPoint[1]);
            }
            faceNormal.normalize();

            // 3) project the vertices to 2d plane by faceNormal
            float cosa, sina, cosb, sinb, cacb, sacb;
            sinb = -sqrtf(faceNormal[0] * faceNormal[0] + faceNormal[1] * faceNormal[1]);
            if (sinb < -1e-5) {
                cosb = faceNormal[2];
                sina = faceNormal[1] / sinb;
                cosa = -faceNormal[0] / sinb;

                cacb = cosa * cosb;
                sacb = sina * cosb;
            }
            else {
                cacb = 1.0f;
                sacb = 0.0f;
                sinb = 0.0f;
                sina = 0.0f;
                if (faceNormal[2] > 0.0f) {
                    cosa = 1.0f;
                    cosb = 1.0f;
                }
                else {
                    cosa = -1.0f;
                    cosb = -1.0f;
                }
            }

            for (size_t j = 0; j < numPoints; j++) {
                const float* point = &fPositions[indices[j] * 3];
                aPosition[j * 2 + 0] = cacb * point[0] - sacb * point[1] + sinb * point[2];
                aPosition[j * 2 + 1] = sina * point[0] + cosa * point[1];
            }

            // 4) copy normals
            if (fNormals) {
                for (size_t j = 0; j < numPoints; j++) {
                    aNormal[j * 3 + 0] = fNormals[indices[j] * 3 + 0];
                    aNormal[j * 3 + 1] = fNormals[indices[j] * 3 + 1];
                    aNormal[j * 3 + 2] = fNormals[indices[j] * 3 + 2];
                }
            }

            // 5) do triangulation
            int numResultTriangles = 0;
            MFnMesh::polyTriangulate(
                aPosition.get(),
                (unsigned int)numPoints,
                (unsigned int)numPoints,
                0,
                fNormals != NULL,
                aNormal.get(),
                triangles.get(),
                numResultTriangles);

            if (numResultTriangles == int(numPoints - 2)) {
                // triangulation success
                for (size_t j = 0; j < size_t(numResultTriangles); j++) {
                    triangleIndices[triangleCount * 3 + 0] = indices[triangles[j * 3 + 0]];
                    triangleIndices[triangleCount * 3 + 1] = indices[triangles[j * 3 + 1]];
                    triangleIndices[triangleCount * 3 + 2] = indices[triangles[j * 3 + 2]];
                    triangleCount++;
                }
            }
            else {
                // triangulation failure, use the default triangulation
                for (size_t j = 1; j < numPoints - 1; j++) {
                    triangleIndices[triangleCount * 3 + 0] = indices[0];
                    triangleIndices[triangleCount * 3 + 1] = indices[j];
                    triangleIndices[triangleCount * 3 + 2] = indices[j + 1];
                    triangleCount++;
                }
            }
        }

        fNumTriangles = totalTriangles;
        fTriangleIndices = triangleIndices;
    }

    size_t numTriangles() { return fNumTriangles; }
    boost::shared_array<index_type>& triangleIndices() { return fTriangleIndices; }

private:
    // Prohibited and not implemented.
    PolyTriangulator(const PolyTriangulator&);
    const PolyTriangulator& operator= (const PolyTriangulator&);

    // Input
    size_t              fNumFaceCounts;
    const unsigned int* fFaceCounts;
    const index_type*   fFaceIndices;
    bool                fFaceIndicesCW;
    const float*        fPositions;
    const float*        fNormals;

    // Output
    size_t                          fNumTriangles;
    boost::shared_array<index_type> fTriangleIndices;
};

} // unnamed namespace

void Triangulator::triangulate()
{
    // triangulate the polygons
    // assume that there are no holes
    //

    // input data
    size_t           numFaceCounts = fFaceCountsCache.getValue()->size();
    const unsigned int* faceCounts = fFaceCountsCache.getValue()->get();

    const IndexBuffer::index_t* faceIndices = fMappedFaceIndices->get();

    const float* positions = fMappedPositions->get();
    const float* normals   = fMappedNormals ? fMappedNormals->get() : NULL;

    if (numFaceCounts == 0) {
        fTriangleIndices.reset();
        return;
    }

    // Triangulate polygons
    PolyTriangulator<IndexBuffer::index_t> polyTriangulator(
        numFaceCounts, faceCounts, faceIndices, true, positions, normals);

    polyTriangulator.compute();

    fTriangleIndices = SharedArray<IndexBuffer::index_t>::create(
        polyTriangulator.triangleIndices(), polyTriangulator.numTriangles() * 3);
}

} // namespace CacheReaderAlembicPrivate
} // namespace AlembicHolder
