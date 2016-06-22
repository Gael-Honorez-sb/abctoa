// Copyright (c) 2014 The Foundry Visionmongers Ltd. All Rights Reserved.

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnGroupBuilder.h>
#include <FnGeolib/op/FnGeolibOp.h>
#include <FnGeolib/util/Path.h> 

#include <FnGeolibServices/FnGeolibCookInterfaceUtilsService.h>

#include "KatanaAttrFileReader.h"
#include "AttrDataCache.h"
#include "pystring\pystring.h"

/** \name Ray Types
* \{
*/
#define AI_RAY_UNDEFINED   0x00         /**< undefined type                                  */
#define AI_RAY_CAMERA      0x01         /**< ray originating at the camera                   */
#define AI_RAY_SHADOW      0x02         /**< shadow ray towards a light source               */
#define AI_RAY_REFLECTED   0x04         /**< mirror reflection ray                           */
#define AI_RAY_REFRACTED   0x08         /**< mirror refraction ray                           */
#define AI_RAY_SUBSURFACE  0x10         /**< subsurface scattering probe ray                 */
#define AI_RAY_DIFFUSE     0x20         /**< indirect diffuse (also known as diffuse GI) ray */
#define AI_RAY_GLOSSY      0x40         /**< glossy/blurred reflection ray                   */
#define AI_RAY_ALL         0xFF         /**< mask for all ray types                          */
#define AI_RAY_GENERIC     AI_RAY_ALL   /**< mask for all ray types                          */
/*\}*/

namespace { //anonymous

class AttributeFile_InOp : public Foundry::Katana::GeolibOp
{
public:
    
    static bool isPathContainsInOtherPath(const std::string &path, const std::string &otherPath)
    {
        std::vector<std::string> pathParts;
        std::vector<std::string> jsonPathParts;

        pystring::split(path, pathParts, "/");
        pystring::split(otherPath, jsonPathParts, "/");

        if (jsonPathParts.size() > pathParts.size())
            return false;

        bool validPath = true;
        for (int i = 0; i < jsonPathParts.size(); i++)
        {
            if (pathParts[i].compare(jsonPathParts[i]) != 0)
                validPath = false;
        }
        if (validPath)
            return validPath;

        return false;
    }


    static void setup(Foundry::Katana::GeolibSetupInterface &interface)
    {
        // This Op can be run concurrently on multiple locations, so
        // provide ThreadModeConcurrent as the threading mode
        interface.setThreading(
            Foundry::Katana::GeolibSetupInterface::ThreadModeConcurrent);
    }

    static void cook(Foundry::Katana::GeolibCookInterface &interface)
    {

        std::string locationPath = interface.getOutputLocationPath();

        
        bool canMatchChildren = false;

        std::string assetLocation = FnAttribute::StringAttribute(interface.getOpArg("assetLocation")).getValue("", false);
       
       
        if (assetLocation.empty() ||
            (!FnGeolibUtil::Path::IsAncestorOrEqual(locationPath, assetLocation) &&
                !FnGeolibUtil::Path::IsAncestor(assetLocation, locationPath)))
        {
            // No root provided, or we're not in the same location tree
            interface.stopChildTraversal();
            return;
        }

        if (!FnGeolibUtil::Path::IsAncestorOrEqual(assetLocation, locationPath))
        {
            // We haven't reached our root location yet, so stop cooking,
            // but don't stop child traversal
            return;
        }

        

        const std::string relativePath = "/" +
            FnGeolibUtil::Path::NormalizedRelativePath(assetLocation, locationPath);

        std::cout << "relativePath " << relativePath << std::endl;

        // Get the string attributes passed in as opArgs
        FnAttribute::StringAttribute filePathAttr =
            interface.getOpArg("filepath");
        FnAttribute::StringAttribute groupNameAttr =
            interface.getOpArg("groupname");

        

        // Ensure the validity of the arguments
        if (!filePathAttr.isValid())
        {
            Foundry::Katana::ReportError(interface,
                                         "Invalid \"filepath\" attribute.");
            return;
        }

        // Get the string value of the attribute
        const std::string filePath = filePathAttr.getValue();

        // If we don't explicitly want to query validity, we can request the
        // value of the attribute, and both suppress errors and provide a
        // default value for if they're not valid. getValue() takes a default
        // value (used when a failure state is encountered) and a boolean
        // indicating whether to throw an exception when the attribute is
        // invalid. By calling getValue("", false) here, we're getting the
        // value of the attribute, but if the attribute is invalid, getValue()
        // returns "", and no exception will be thrown.
        const std::string groupName = groupNameAttr.getValue("", false);

        try
        {
            // Load the attribute data from the file provided, using the
            // shared object path if needed
            const AttrData* attributeData = loadFileData(filePath);

            if (!attributeData)
            {
                // Make sure we actually got some data back
                return;
            }

            // getOutputName() returns the name of the scene graph location
            // that we're creating data for now, so check to see if there's a
            // match for the name in the attribute cache. getOutputName()
            // returns the leaf name for the outputLocation, e.g. if we're
            // cooking at "/root/world/geo", then "geo" will be returned.
            // To get the full location path, we could use
            // getOutputLocationPath().


            bool foundInPath = false;
            int pathSize = 0;
            for (std::vector<std::string>::const_iterator it = attributeData->attributes.begin(); it != attributeData->attributes.end(); ++it)
            {
                Json::Value overrides;
                if (it->find("/") != std::string::npos)
                {
                    if (isPathContainsInOtherPath(relativePath, *it))
                    {
                        foundInPath = true;
                        
                        std::string overridePath = *it;
                        if (overridePath.length() > pathSize)
                        {
                            pathSize = overridePath.length();
                            overrides = attributeData->AttrDataRoot[*it];
                        }
                        
                    }

                }
                if (overrides.size() > 0)
                {
                    FnAttribute::GroupBuilder gb;

                    for (Json::ValueIterator itr = overrides.begin(); itr != overrides.end(); itr++)
                    {
                        std::string attribute = itr.key().asString();

                        if (attribute == "subdiv_iterations")
                            attribute = "iterations";
                        else if (attribute == "disp_zero_value")
                            attribute = "zero_value";

                        Json::Value val = attributeData->AttrDataRoot[*it][itr.key().asString()];
                        
                        switch (val.type())
                        {
                            case Json::ValueType::booleanValue:
                                std::cout << attribute << " = " << val.asBool() << std::endl;
                                gb.set(attribute,
                                    FnAttribute::IntAttribute(
                                        val.asBool()), false);
                                break;
                            case Json::ValueType::intValue:
                            case Json::ValueType::uintValue:
                            
                                if(attribute == "subdiv_type")
                                    switch (val.asInt())
                                    {
                                        case 0:
                                            gb.set(attribute,
                                                FnAttribute::StringAttribute(
                                                    "none"), false);
                                            break;
                                        case 1:
                                            gb.set(attribute,
                                                FnAttribute::StringAttribute(
                                                    "catclark"), false);
                                            break;
                                        case 2:
                                            gb.set(attribute,
                                                FnAttribute::StringAttribute(
                                                    "linear"), false);
                                            break;
                                    }
                                else if (attribute == "subdiv_uv_smoothing")
                                    switch (val.asInt())
                                    {
                                    case 0:
                                        gb.set(attribute,
                                            FnAttribute::StringAttribute(
                                                "pin_corners"), false);
                                        break;
                                    case 1:
                                        gb.set(attribute,
                                            FnAttribute::StringAttribute(
                                                "pin_borders"), false);
                                        break;
                                    case 2:
                                        gb.set(attribute,
                                            FnAttribute::StringAttribute(
                                                "linear"), false);
                                        break;
                                    case 3:
                                        gb.set(attribute,
                                            FnAttribute::StringAttribute(
                                                "smooth"), false);
                                        break;
                                    }
                                else if (attribute == "visibility")
                                {
                                    int visibilityMask = val.asInt();
                                    if (visibilityMask > (AI_RAY_ALL & ~AI_RAY_GLOSSY))
                                    {
                                        gb.set(attribute + ".AI_RAY_GLOSSY", FnAttribute::IntAttribute(1), false);
                                        visibilityMask = visibilityMask - AI_RAY_GLOSSY;
                                    }
                                    else
                                        gb.set(attribute + ".AI_RAY_GLOSSY", FnAttribute::IntAttribute(0), false);

                                    if(visibilityMask > (AI_RAY_ALL & ~(AI_RAY_GLOSSY | AI_RAY_DIFFUSE)))
                                    {
                                        gb.set(attribute + ".AI_RAY_DIFFUSE", FnAttribute::IntAttribute(1), false);
                                        visibilityMask = visibilityMask - AI_RAY_DIFFUSE;
                                    }
                                    else
                                        gb.set(attribute + ".AI_RAY_DIFFUSE", FnAttribute::IntAttribute(0), false);

                                    if (visibilityMask > (AI_RAY_ALL & ~(AI_RAY_GLOSSY | AI_RAY_DIFFUSE | AI_RAY_SUBSURFACE)))
                                    {
                                        gb.set(attribute + ".AI_RAY_SUBSURFACE", FnAttribute::IntAttribute(1), false);
                                        visibilityMask = visibilityMask - AI_RAY_SUBSURFACE;
                                    }
                                    else
                                        gb.set(attribute + ".AI_RAY_SUBSURFACE", FnAttribute::IntAttribute(0), false);

                                    if (visibilityMask > (AI_RAY_ALL & ~(AI_RAY_GLOSSY | AI_RAY_DIFFUSE | AI_RAY_SUBSURFACE | AI_RAY_REFRACTED)))
                                    {
                                        gb.set(attribute + ".AI_RAY_REFRACTED", FnAttribute::IntAttribute(1), false);
                                        visibilityMask = visibilityMask - AI_RAY_REFRACTED;
                                    }
                                    else
                                        gb.set(attribute + ".AI_RAY_REFRACTED", FnAttribute::IntAttribute(0), false);

                                    if (visibilityMask > (AI_RAY_ALL & ~(AI_RAY_GLOSSY | AI_RAY_DIFFUSE | AI_RAY_SUBSURFACE | AI_RAY_REFRACTED | AI_RAY_REFLECTED)))
                                    {
                                        gb.set(attribute + ".AI_RAY_REFLECTED", FnAttribute::IntAttribute(1), false);
                                        visibilityMask = visibilityMask - AI_RAY_REFLECTED;
                                    }
                                    else
                                        gb.set(attribute + ".AI_RAY_REFLECTED", FnAttribute::IntAttribute(0), false);

                                    if (visibilityMask > (AI_RAY_ALL & ~(AI_RAY_GLOSSY | AI_RAY_DIFFUSE | AI_RAY_SUBSURFACE | AI_RAY_REFRACTED | AI_RAY_REFLECTED | AI_RAY_SHADOW)))
                                    {
                                        gb.set(attribute + ".AI_RAY_SHADOW", FnAttribute::IntAttribute(1), false);
                                        visibilityMask = visibilityMask - AI_RAY_SHADOW;
                                    }
                                    else
                                        gb.set(attribute + ".AI_RAY_SHADOW", FnAttribute::IntAttribute(0), false);

                                    if (visibilityMask > (AI_RAY_ALL & ~(AI_RAY_GLOSSY | AI_RAY_DIFFUSE | AI_RAY_SUBSURFACE | AI_RAY_REFRACTED | AI_RAY_REFLECTED | AI_RAY_SHADOW | AI_RAY_CAMERA)))
                                        gb.set(attribute + ".AI_RAY_CAMERA", FnAttribute::IntAttribute(1), false);
                                    else
                                        gb.set(attribute + ".AI_RAY_CAMERA", FnAttribute::IntAttribute(0), false);
                                }
                                else
                                {
                                
                                    gb.set(attribute,
                                        FnAttribute::IntAttribute(
                                            val.asInt()), false);
                                }
                                break;

                            case Json::ValueType::realValue:
                                std::cout << attribute << " = " << val.asFloat() << std::endl;
                                gb.set(attribute,
                                    FnAttribute::FloatAttribute(
                                        val.asFloat()), false);
                                break;
                            case Json::ValueType::stringValue:
                                std::cout << attribute << " = " << val.asString() << std::endl;
                                gb.set(attribute,
                                    FnAttribute::StringAttribute(
                                        val.asString()), false);
                                break;
                            default:
                                break;
                        }
                            

                    }
                    interface.setAttr("arnoldStatements", gb.build());


                }
            }
        }
        catch (std::runtime_error &e)
        {
            // File load failed. Report the error.
            Foundry::Katana::ReportError(interface, e.what());
            return;
        }
    }

    static void flush()
    {
        // Flush the data cache. This can be prompted by the user clicking the
        // "Flush Caches" button in the UI, or elsewhere by Katana internally.
        // Reset the data cache to an empty one.
        m_dataCache.reset(new AttrDataCache);
    }

private:

    // Loads the contents of the file with the given name using the reader
    // plug-in with the given .so filename. If no soFilename is given, the
    // KatanaAttrFileReader will be used by default.
    static const AttrData* loadFileData(const std::string &filename)
    {
        // Query the AttributeKeyedCache cache for the AttrData associated
        // with the given file name and .so file name. The implementation of
        // the AttrDataCache will handle loading the data from the file, if
        // it's not yet been loaded.
        AttrDataCache::IMPLPtr result = m_dataCache->getValue(
            FnAttribute::GroupBuilder()
                .set("filename", FnAttribute::StringAttribute(filename))
                .build()
        );

        return result.get();
    }

    // Static cache mapping filenames to loaded data
    static AttrDataCache::Ptr m_dataCache;
};

AttrDataCache::Ptr AttributeFile_InOp::m_dataCache(new AttrDataCache());

DEFINE_GEOLIBOP_PLUGIN(AttributeFile_InOp)

} // anonymous

void registerPlugins()
{
    REGISTER_PLUGIN(AttributeFile_InOp, "AttributeJsonFile_In", 0, 1);
}

