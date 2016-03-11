#include "WriteCamera.h"


#include <ai.h>
#include <sstream>

#include "json/json.h"

void ProcessCamera( ICamera &camera, const ProcArgs &args,
        MatrixSampleMap * xformSamples)
{
    if (!camera.valid())
        return;

    SampleTimeSet sampleTimes;

    ICameraSchema ps = camera.getSchema();

    TimeSamplingPtr ts = ps.getTimeSampling();
    sampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );


    std::string name = args.nameprefix + camera.getFullName();

    SampleTimeSet singleSampleTimes;
    singleSampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );

    ISampleSelector sampleSelector( *singleSampleTimes.begin() );

    Alembic::AbcGeom::CameraSample sample = ps.getValue( sampleSelector );

    AtNode * cameraNode = AiNode("persp_camera");
    AiNodeSetStr(cameraNode, "name", name.c_str());

    AiNodeSetFlt(cameraNode, "fov", sample.getFocalLength());


}
