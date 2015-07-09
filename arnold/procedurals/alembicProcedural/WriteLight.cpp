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

// reference, public domain code : http://www.fourmilab.ch/documents/specrend/specrend.c

double BBSpectrum(double wavelength, double bbTemp)
{
    double wlm = wavelength * 1e-9;   /* Wavelength in meters */

    return (3.74183e-16 * pow(wlm, -5.0)) /
           (exp(1.4388e-2 / (wlm * bbTemp)) - 1.0);
} 

float ScaleLightExposure(float exposure, ProcArgs &args)
{
    // Scale the light intensity to match the scale of the procedural
    double e = static_cast<double>(exposure);
    AtMatrix m;
    AtVector scaleVector = AiVector(1.0, 0.0, 0.0);
    
    AiNodeGetMatrix(args.proceduralNode, "matrix", m);
    AiM4VectorByMatrixMult (&scaleVector, m, &scaleVector);  
    float scaleFactor = AiV3Length(scaleVector);
    e += log(scaleFactor * scaleFactor) / log(2.0);
    exposure = static_cast<float>(e);
    return exposure;
} 

AtRGB XYZtoRGB(double x, double y, double z)
{   
   // using the standard CIE color space
   
   static const double xr = 0.7355;
   static const double xg = 0.2658;
   static const double xb = 0.1669;
   static const double yr = 0.2645;
   static const double yg = 0.7243;
   static const double yb = 0.0085;
   static const double zr = 1. - (xr + yr);
   static const double zg = 1. - (xg + yg);
   static const double zb = 1. - (xb + yb);
   static const double xw = 0.33333333;
   static const double yw = 0.33333333;
   static const double zw = 1. - (xw + yw);
   
   /* xyz -> rgb matrix, before scaling to white. */   
   double rx = (yg * zb) - (yb * zg);
   double ry = (xb * zg) - (xg * zb);
   double rz = (xg * yb) - (xb * yg);
   double gx = (yb * zr) - (yr * zb);
   double gy = (xr * zb) - (xb * zr);
   double gz = (xb * yr) - (xr * yb);
   double bx = (yr * zg) - (yg * zr);
   double by = (xg * zr) - (xr * zg);
   double bz = (xr * yg) - (xg * yr);

   /* White scaling factors.
      Dividing by yw scales the white luminance to unity, as conventional. */

   double rw = ((rx * xw) + (ry * yw) + (rz * zw)) / yw;
   double gw = ((gx * xw) + (gy * yw) + (gz * zw)) / yw;
   double bw = ((bx * xw) + (by * yw) + (bz * zw)) / yw;

   /* xyz -> rgb matrix, correctly scaled to white. */

   rx = rx / rw;  ry = ry / rw;  rz = rz / rw;
   gx = gx / gw;  gy = gy / gw;  gz = gz / gw;
   bx = bx / bw;  by = by / bw;  bz = bz / bw;

   /* rgb of the desired point */
   AtRGB rgb;

   rgb.r = static_cast<float>((rx * x) + (ry * y) + (rz * z));
   rgb.g = static_cast<float>((gx * x) + (gy * y) + (gz * z));
   rgb.b = static_cast<float>((bx * x) + (by * y) + (bz * z));
   return rgb;
}
 

AtRGB ConvertKelvinToRGB(float kelvin)
{
   double bbTemp = (double)kelvin;
   static double cie_colour_match[81][3] = {
      {0.0014,0.0000,0.0065}, {0.0022,0.0001,0.0105}, {0.0042,0.0001,0.0201},
      {0.0076,0.0002,0.0362}, {0.0143,0.0004,0.0679}, {0.0232,0.0006,0.1102},
      {0.0435,0.0012,0.2074}, {0.0776,0.0022,0.3713}, {0.1344,0.0040,0.6456},
      {0.2148,0.0073,1.0391}, {0.2839,0.0116,1.3856}, {0.3285,0.0168,1.6230},
      {0.3483,0.0230,1.7471}, {0.3481,0.0298,1.7826}, {0.3362,0.0380,1.7721},
      {0.3187,0.0480,1.7441}, {0.2908,0.0600,1.6692}, {0.2511,0.0739,1.5281},
      {0.1954,0.0910,1.2876}, {0.1421,0.1126,1.0419}, {0.0956,0.1390,0.8130},
      {0.0580,0.1693,0.6162}, {0.0320,0.2080,0.4652}, {0.0147,0.2586,0.3533},
      {0.0049,0.3230,0.2720}, {0.0024,0.4073,0.2123}, {0.0093,0.5030,0.1582},
      {0.0291,0.6082,0.1117}, {0.0633,0.7100,0.0782}, {0.1096,0.7932,0.0573},
      {0.1655,0.8620,0.0422}, {0.2257,0.9149,0.0298}, {0.2904,0.9540,0.0203},
      {0.3597,0.9803,0.0134}, {0.4334,0.9950,0.0087}, {0.5121,1.0000,0.0057},
      {0.5945,0.9950,0.0039}, {0.6784,0.9786,0.0027}, {0.7621,0.9520,0.0021},
      {0.8425,0.9154,0.0018}, {0.9163,0.8700,0.0017}, {0.9786,0.8163,0.0014},
      {1.0263,0.7570,0.0011}, {1.0567,0.6949,0.0010}, {1.0622,0.6310,0.0008},
      {1.0456,0.5668,0.0006}, {1.0026,0.5030,0.0003}, {0.9384,0.4412,0.0002},
      {0.8544,0.3810,0.0002}, {0.7514,0.3210,0.0001}, {0.6424,0.2650,0.0000},
      {0.5419,0.2170,0.0000}, {0.4479,0.1750,0.0000}, {0.3608,0.1382,0.0000},
      {0.2835,0.1070,0.0000}, {0.2187,0.0816,0.0000}, {0.1649,0.0610,0.0000},
      {0.1212,0.0446,0.0000}, {0.0874,0.0320,0.0000}, {0.0636,0.0232,0.0000},
      {0.0468,0.0170,0.0000}, {0.0329,0.0119,0.0000}, {0.0227,0.0082,0.0000},
      {0.0158,0.0057,0.0000}, {0.0114,0.0041,0.0000}, {0.0081,0.0029,0.0000},
      {0.0058,0.0021,0.0000}, {0.0041,0.0015,0.0000}, {0.0029,0.0010,0.0000},
      {0.0020,0.0007,0.0000}, {0.0014,0.0005,0.0000}, {0.0010,0.0004,0.0000},
      {0.0007,0.0002,0.0000}, {0.0005,0.0002,0.0000}, {0.0003,0.0001,0.0000},
      {0.0002,0.0001,0.0000}, {0.0002,0.0001,0.0000}, {0.0001,0.0000,0.0000},
      {0.0001,0.0000,0.0000}, {0.0001,0.0000,0.0000}, {0.0000,0.0000,0.0000}
   };
   
   double X = 0;
   double Y = 0;
   double Z = 0;
   double lambda = 380.0;
   for (int i = 0; lambda < 780.1; i++, lambda += 5.0)
   {
      double Me;
      Me = BBSpectrum(lambda, bbTemp);
      X += Me * cie_colour_match[i][0];
      Y += Me * cie_colour_match[i][1];
      Z += Me * cie_colour_match[i][2];
   }
   const double XYZ = (X + Y + Z);
   X /= XYZ;
   Y /= XYZ;
   Z /= XYZ;
   
   AtRGB rgb = XYZtoRGB(X, Y, Z);
   float w;
   w = (0.f < rgb.r) ? 0.f : rgb.r;
   w = (w < rgb.g) ? w : rgb.g;
   w = (w < rgb.b) ? w : rgb.b;
   
   if (w < 0.f)
   {
      rgb.r -= w;
      rgb.g -= w;
      rgb.b -= w;
   }
   
   float greatest = MAX(rgb.r, MAX(rgb.g, rgb.b));
    
   if (greatest > 0)
   {
      rgb.r /= greatest;
      rgb.g /= greatest;
      rgb.b /= greatest;
   }

   return rgb;
} 


void GetColorTemperatureOverride(std::string name, std::vector<std::string> tags, ProcArgs & args, bool & use_temperature, float & temperature)
{
    bool foundInPath = false;
    int pathSize = 0;
    for(std::vector<std::string>::iterator it=args.attributes.begin(); it!=args.attributes.end(); ++it)
    {
        Json::Value overrides;
        if(it->find("/") != std::string::npos)
        {
            bool curFoundInPath = isPathContainsInOtherPath(name, *it);
            if(curFoundInPath)
            {
                foundInPath = true;
                std::string overridePath = *it;
                if(overridePath.length() > pathSize)
                {
                    pathSize = overridePath.length();
                    overrides = args.attributesRoot[*it];
                }
            }
        }
        else if(matchPattern(name,*it)) // based on wildcard expression
        {
            foundInPath = true;
            std::string overridePath = *it;
            if(overridePath.length() > pathSize)
            {   
                pathSize = overridePath.length();
                overrides = args.attributesRoot[*it];
            }
            
        }
        else if(foundInPath == false)
        {
            if (std::find(tags.begin(), tags.end(), *it) != tags.end())
            {
                std::string overridePath = *it;
                if(overridePath.length() > pathSize)
                {
                    pathSize = overridePath.length();
                    overrides = args.attributesRoot[*it];
                }
            }
        }
        if(overrides.size() > 0)
        {
            for( Json::ValueIterator itr = overrides.begin() ; itr != overrides.end() ; itr++ )
            {
                std::string attribute = itr.key().asString();
                
                if(attribute.compare("use_color_temperature") == 0)
                {
                    Json::Value val = args.attributesRoot[*it][itr.key().asString()];
                    use_temperature = val.asBool();
                }
                else if (attribute.compare("color_temperature") == 0)
                {
                    Json::Value val = args.attributesRoot[*it][itr.key().asString()];
                    temperature = val.asFloat();
                }
            }
        }
    }
}


void ProcessLight( ILight &light, ProcArgs &args,
        MatrixSampleMap * xformSamples)
{
    if (!light.valid())
        return;

    std::string originalName = light.getFullName();
    std::string name = args.nameprefix + originalName;

    SampleTimeSet sampleTimes;

    ILightSchema ps = light.getSchema();

    TimeSamplingPtr ts = ps.getTimeSampling();
    sampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );

    SampleTimeSet singleSampleTimes;
    singleSampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );

    ICompoundProperty arbGeomParams = ps.getArbGeomParams();
    ISampleSelector frameSelector( *singleSampleTimes.begin() );
    bool gotType = false;

    //get tags
    std::vector<std::string> tags;
    getAllTags(light, tags, &args);

    // Checking if the light must be exported.
    const PropertyHeader * lightIntensityHeader = arbGeomParams.getPropertyHeader("intensity");
    if (IFloatGeomParam::matches( *lightIntensityHeader ))
    {
        IFloatGeomParam param( arbGeomParams,  "intensity" );
        if ( param.valid() )
        {
            if ( param.getScope() == kConstantScope || param.getScope() == kUnknownScope)
            {
                IFloatGeomParam::prop_type::sample_ptr_type valueSample = param.getExpandedValue().getVals();
                if(valueSample->get()[0] <= 0.0)
                    return;
            }
        }
    }

    if(args.linkAttributes)
    {
        for(std::vector<std::string>::iterator it=args.attributes.begin(); it!=args.attributes.end(); ++it)
        {
            if(name.find(*it) != string::npos || std::find(tags.begin(), tags.end(), *it) != tags.end() || matchPattern(name,*it))
            {
                const Json::Value overrides = args.attributesRoot[*it];
                if(overrides.size() > 0)
                {
                    for( Json::ValueIterator itr = overrides.begin() ; itr != overrides.end() ; itr++ )
                    {
                        std::string attribute = itr.key().asString();
                        if (attribute=="intensity")
                        {
                            Json::Value val = args.attributesRoot[*it][itr.key().asString()];
                            if (val.type() == Json::realValue )
                              if(val.asDouble() <= 0.0)
                                  return;
                        }
                    }
                }
            }
        }
    }


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
                        lightNode = AiNode("quad_light");
                        AtPoint vertices[4];
                        AiV3Create(vertices[3], 1, 1, 0);
                        AiV3Create(vertices[0], 1, -1, 0);
                        AiV3Create(vertices[1], -1, -1, 0);
                        AiV3Create(vertices[2], -1, 1, 0);
                        AiNodeSetArray(lightNode, "vertices", AiArrayConvert(4, 1, AI_TYPE_POINT, vertices));
                        break;
                    case 4:
                        lightNode = AiNode("photometric_light");
                        break;
                    case 5:
                        lightNode = AiNode("disk_light");
                        break;
                    case 6:
                        lightNode = AiNode("cylinder_light");
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

    
    AiNodeSetStr(lightNode, "name", name.c_str());


    // adding arbitary parameters
    AddArbitraryGeomParams(
            arbGeomParams,
            frameSelector,
            lightNode );

    // adding attributes from procedural
    AddArbitraryProceduralParams(args.proceduralNode, lightNode);

    bool useTemperature = false;
    float colorTemperature = 6500.0;

    // Eventually override color with temperature
    const PropertyHeader * useTempHeader = arbGeomParams.getPropertyHeader("use_color_temperature");
    if (IBoolGeomParam::matches( *useTempHeader ))
    {
        IBoolGeomParam param( arbGeomParams,  "use_color_temperature" );
        if ( param.valid() )
        {
            if ( param.getScope() == kConstantScope || param.getScope() == kUnknownScope)
            {
                IBoolGeomParam::prop_type::sample_ptr_type valueSample = param.getExpandedValue().getVals();
                useTemperature = valueSample->get()[0];
            }
        }
    }


    const PropertyHeader * TempHeader = arbGeomParams.getPropertyHeader("color_temperature");
                    
    if (IFloatGeomParam::matches( *TempHeader ))
    {
        IFloatGeomParam param( arbGeomParams,  "color_temperature" );
        if ( param.valid() )
            if ( param.getScope() == kConstantScope || param.getScope() == kUnknownScope)
            {
                IFloatGeomParam::prop_type::sample_ptr_type valueSample = param.getExpandedValue().getVals();
                colorTemperature = valueSample->get()[0];
            }
    }

    GetColorTemperatureOverride(originalName, tags, args, useTemperature, colorTemperature);

    if(useTemperature)
    {
        AtRGB color = ConvertKelvinToRGB(colorTemperature);
        AiNodeSetRGB(lightNode, "color", color.r, color.g, color.b);
    }

    if(args.linkAttributes)
        ApplyOverrides(originalName, lightNode, tags, args);

    // Xform
    ApplyTransformation( lightNode, xformSamples, args );

    // Scale the light intensity
    float exposure = AiNodeGetFlt(lightNode, "exposure");
    AiNodeSetFlt(lightNode, "exposure", ScaleLightExposure(exposure, args));

    //args.createdNodes.push_back(lightNode);
}
