#include "WriteLight.h"

#include "ArbGeomParams.h"
#include "parseAttributes.h"
#include "WriteTransform.h"
#include "WriteOverrides.h"
#include "PathUtil.h"
#include "parseAttributes.h"


#include <ai.h>
#include <sstream>


#include "json/json.h"

void ProcessLight( ILight &light, ProcArgs &args,
        MatrixSampleMap * xformSamples)
{
    if (!light.valid())
        return;

    SampleTimeSet sampleTimes;

    ILightSchema ps = light.getSchema();

    TimeSamplingPtr ts = ps.getTimeSampling();
    sampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );

    SampleTimeSet singleSampleTimes;
    singleSampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );

    ICompoundProperty arbGeomParams = ps.getArbGeomParams();
    ISampleSelector frameSelector( *singleSampleTimes.begin() );
    bool gotType = false;

    AtNode * lightNode;

    const PropertyHeader * lightTypeHeader = arbGeomParams.getPropertyHeader("light_type");
    if (IInt32GeomParam::matches( *lightTypeHeader ))
    {
        IInt32GeomParam param( arbGeomParams,  "light_type" );
        if ( param.valid() )
        {
            if ( param.getScope() == kConstantScope || param.getScope() == kUnknownScope)
            {
                IInt32GeomParam::prop_type::sample_ptr_type valueSample = param.getExpandedValue().getVals();
                Alembic::Abc::int32_t lightType = valueSample->get()[0];
                switch(lightType)
                {
                    case 0:
                        lightNode = AiNode("distant_light");
                        break;
                    case 1:
                        lightNode = AiNode("point_light");
                        break;
                    case 2:
                        lightNode = AiNode("spot_light");
                        break;
                    case 3:
                        lightNode = AiNode("area_light");
                        break;
                    default:
                        return;
                }
                gotType = true;
            }
        }

    }
    if(!gotType)
        return;

    std::string originalName = light.getFullName();
    std::string name = args.nameprefix + originalName;
    
    AiNodeSetStr(lightNode, "name", name.c_str());

  //get tags
    std::vector<std::string> tags;
    getAllTags(light, tags, &args);


    if(args.linkAttributes)
        ApplyOverrides(originalName, lightNode, tags, args);

    // adding arbitary parameters
    AddArbitraryGeomParams(
            arbGeomParams,
            frameSelector,
            lightNode );

    // Xform
    ApplyTransformation( lightNode, xformSamples, args );

    args.createdNodes.push_back(lightNode);
}
