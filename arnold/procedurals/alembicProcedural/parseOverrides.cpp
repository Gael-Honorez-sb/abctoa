
#include "parseOverrides.h"
#include <pystring.h>

namespace
{
    using namespace Alembic::AbcGeom;
    using namespace Alembic::AbcMaterial;
}

bool isVisible(IObject child, IXformSchema xs, ProcArgs* args)
{
    if(GetVisibility( child, ISampleSelector( args->frame / args->fps ) ) == kVisibilityHidden)
    {
        // check if the object is not forced to be visible
        std::string name = args->nameprefix + child.getFullName();

        if(args->linkOverride)
        {
            for(std::vector<std::string>::iterator it=args->overrides.begin(); it!=args->overrides.end(); ++it)
            {
                    Json::Value overrides;
                    if(it->find("/") != string::npos)
                        if(name.find(*it) != string::npos)
                            overrides = args->overrideRoot[*it];


                    if(overrides.size() > 0)
                    {
                        for( Json::ValueIterator itr = overrides.begin() ; itr != overrides.end() ; itr++ )
                        {
                            std::string attribute = itr.key().asString();
                            if (attribute == "forceVisible")
                            {

                                bool vis = args->overrideRoot[*it][itr.key().asString()].asBool();
                                if(vis)
                                    return true;
                                else
                                    return false;
                            }

                        }

                    }
            }
        }
        return false;
    }

    // Check it's a xform and that xform has a tag "DISPLAY" to skip it.
    ICompoundProperty prop = xs.getArbGeomParams();
    if ( prop != NULL && prop.valid() )
    {
        std::vector<std::string> tags;
        if (prop.getPropertyHeader("mtoa_constant_tags") != NULL)
        {
            const PropertyHeader * tagsHeader = prop.getPropertyHeader("mtoa_constant_tags");
            if (IStringGeomParam::matches( *tagsHeader ))
            {
                IStringGeomParam param( prop,  "mtoa_constant_tags" );
                if ( param.valid() )
                {
                    IStringGeomParam::prop_type::sample_ptr_type valueSample =
                                    param.getExpandedValue( ISampleSelector( args->frame / args->fps ) ).getVals();

                    if ( param.getScope() == kConstantScope || param.getScope() == kUnknownScope)
                    {
                        Json::Value jtags;
                        Json::Reader reader;
                        if(reader.parse(valueSample->get()[0], jtags))
                            for( Json::ValueIterator itr = jtags.begin() ; itr != jtags.end() ; itr++ )
                            {

                                if (jtags[itr.key().asUInt()].asString() == "DISPLAY" )
                                    return false;
                            }
                    }
                }
            }
        }
    }

    return true;
}


bool isVisibleForArnold(IObject child, ProcArgs* args)
{
    AtUInt16 minVis = AI_RAY_ALL & ~(AI_RAY_GLOSSY|AI_RAY_DIFFUSE|AI_RAY_REFRACTED|AI_RAY_REFLECTED|AI_RAY_SHADOW|AI_RAY_CAMERA);
    std::string name = args->nameprefix + child.getFullName();

    if(args->linkOverride)
    {
        for(std::vector<std::string>::iterator it=args->overrides.begin(); it!=args->overrides.end(); ++it)
        {
                Json::Value overrides;
                if(it->find("/") != string::npos)
                    if(name.find(*it) != string::npos)
                        overrides = args->overrideRoot[*it];


                if(overrides.size() > 0)
                {
                    for( Json::ValueIterator itr = overrides.begin() ; itr != overrides.end() ; itr++ )
                    {
                        std::string attribute = itr.key().asString();
                        if (attribute == "visibility")
                        {

                            AtUInt16 vis = args->overrideRoot[*it][itr.key().asString()].asInt();
                            if(vis <= minVis)
                            {
                                AiMsgDebug("Object %s is invisible", name.c_str());
                                return false;
                            }

                            break;
                        }

                    }

                }
        }
    }
    return true;

}


void OverrideProperties(Json::Value & jroot, Json::Value jrootOverrides)
{
    for( Json::ValueIterator itr = jrootOverrides.begin() ; itr != jrootOverrides.end() ; itr++ )
    {
        const Json::Value paths = jrootOverrides[itr.key().asString()];
        for( Json::ValueIterator overPath = paths.begin() ; overPath != paths.end() ; overPath++ )
        {
            Json::Value attr = paths[overPath.key().asString()];
            jroot[itr.key().asString()][overPath.key().asString()] = attr;
        }
    }
}

Json::Value OverrideAssignations(Json::Value jroot, Json::Value jrootOverrides)
{
    std::vector<std::string> pathOverrides;

    Json::Value newJroot;
    // concatenate both json string.
    for( Json::ValueIterator itr = jrootOverrides.begin() ; itr != jrootOverrides.end() ; itr++ )
    {
        Json::Value tmp = jroot[itr.key().asString()];
        const Json::Value paths = jrootOverrides[itr.key().asString()];
        for( Json::ValueIterator shaderPath = paths.begin() ; shaderPath != paths.end() ; shaderPath++ )
        {
            Json::Value val = paths[shaderPath.key().asUInt()];
            pathOverrides.push_back(val.asString());
        }
        if(tmp.size() == 0)
        {
            newJroot[itr.key().asString()] = jrootOverrides[itr.key().asString()];
        }
        else
        {
            const Json::Value shaderPaths = jrootOverrides[itr.key().asString()];
            for( Json::ValueIterator itr2 = shaderPaths.begin() ; itr2 != shaderPaths.end() ; itr2++ )
            {
                newJroot[itr.key().asString()].append(jrootOverrides[itr.key().asString()][itr2.key().asUInt()]);
            }
        }
    }
    // Now adding back the original shaders without the ones overriden
    for( Json::ValueIterator itr = jroot.begin() ; itr != jroot.end() ; itr++ )
    {
        const Json::Value pathsShader = jroot[itr.key().asString()];
        const Json::Value curval = newJroot[itr.key().asString()];
        for( Json::ValueIterator shaderPathOrig = pathsShader.begin() ; shaderPathOrig != pathsShader.end() ; shaderPathOrig++ )
        {
            Json::Value val = pathsShader[shaderPathOrig.key().asUInt()];
            bool isPresent = (std::find(pathOverrides.begin(), pathOverrides.end(), val.asCString())  != pathOverrides.end());
            if (!isPresent)
                newJroot[itr.key().asString()].append(val);
        }
    }


    return newJroot;
}

void ParseShaders(Json::Value jroot, std::string ns, std::string nameprefix, ProcArgs* args, AtByte type)
{
    for( Json::ValueIterator itr = jroot.begin() ; itr != jroot.end() ; itr++ )
    {
        AiMsgDebug( "[ABC] Parsing shader %s", itr.key().asCString());
        std::string shaderName = ns + itr.key().asString();
        AtNode* shaderNode = AiNodeLookUpByName(shaderName.c_str());
        if(shaderNode == NULL)
        {
            if(args->useAbcShaders)
            {
                std::string orginalName = itr.key().asString();
                // we must get rid of .message if it's there
                if(pystring::endswith(orginalName, ".message"))
                {
                    orginalName = pystring::replace(orginalName, ".message", "");
                }

                AiMsgDebug( "[ABC] Create shader %s from ABC", orginalName.c_str());

                IObject object = args->materialsObject.getChild(orginalName);
                if (IMaterial::matches(object.getHeader()))
                {
                    shaderNode = AiNode("AbcShader");
                    AiNodeSetStr(shaderNode, "file", args->abcShaderFile);
                    AiNodeSetStr(shaderNode, "shader", orginalName.c_str());
                    AiNodeSetStr(shaderNode, "name", shaderName.c_str());
                }
            }
            if(shaderNode == NULL)
            {
                // search without custom namespace
                shaderName = itr.key().asString();
                shaderNode = AiNodeLookUpByName(shaderName.c_str());
                if(shaderNode == NULL)
                {
                    AiMsgDebug( "[ABC] Searching shader %s deeper underground...", itr.key().asCString());
                    // look for the same namespace for shaders...
                    std::vector<std::string> strs;
                    boost::split(strs,nameprefix,boost::is_any_of(":"));
                    if(strs.size() > 1)
                    {
                        strs.pop_back();
                        strs.push_back(itr.key().asString());

                        shaderNode = AiNodeLookUpByName(boost::algorithm::join(strs, ":").c_str());
                    }
                }
            }
        }
        if(shaderNode != NULL)
        {
            const Json::Value paths = jroot[itr.key().asString()];
            AiMsgDebug("[ABC] Shader exists, checking paths. size = %d", paths.size());
            for( Json::ValueIterator itr2 = paths.begin() ; itr2 != paths.end() ; itr2++ )
            {
                Json::Value val = paths[itr2.key().asUInt()];
                AiMsgDebug("[ABC] Adding path %s", val.asCString());
                if(type == 0)
                    args->displacements[val.asString().c_str()] = shaderNode;
                else if(type == 1)
                    args->shaders[val.asString().c_str()] = shaderNode;

            }
        }
        else
        {
            AiMsgWarning("[ABC] Can't find shader %s", shaderName.c_str());
        }
    }

}