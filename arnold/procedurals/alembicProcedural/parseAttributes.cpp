
#include "parseAttributes.h"
#include "abcshaderutils.h"
#include <pystring.h>
#include <boost/thread.hpp>

boost::mutex gGlobalLock;
#define GLOBAL_LOCK	   boost::mutex::scoped_lock writeLock( gGlobalLock );


namespace
{
    using namespace Alembic::AbcGeom;
    using namespace Alembic::AbcMaterial;
}

void getTags(IObject &iObj, std::vector<std::string> & tags, ProcArgs* args)
{
    const MetaData &md = iObj.getMetaData();

    ICompoundProperty arbGeomParams;

    if ( IXformSchema::matches(md))
    {
        IXform xform( iObj, kWrapExisting );
        IXformSchema ms = xform.getSchema();
        arbGeomParams = ms.getArbGeomParams();
    }
    else if ( IPolyMeshSchema::matches( md ))
    {
        IPolyMesh mesh( iObj, kWrapExisting );
        IPolyMeshSchema ms = mesh.getSchema();
        arbGeomParams = ms.getArbGeomParams();
    }
    else if ( ISubD::matches( md ))
    {
        ISubD mesh( iObj, kWrapExisting );
        ISubDSchema ms = mesh.getSchema();
        arbGeomParams = ms.getArbGeomParams();
    }

    else if ( IPoints::matches( md ) )
    {
        IPoints points( iObj, kWrapExisting );
        IPointsSchema ms = points.getSchema();
        arbGeomParams = ms.getArbGeomParams();

    }
    else if ( ICurves::matches( md ) )
    {
        ICurves curves( iObj, kWrapExisting );
        ICurvesSchema ms = curves.getSchema();
        arbGeomParams = ms.getArbGeomParams();
    }


    if ( arbGeomParams != NULL && arbGeomParams.valid() )
    {
        if (arbGeomParams.getPropertyHeader("mtoa_constant_tags") != NULL)
        {
            const PropertyHeader * tagsHeader = arbGeomParams.getPropertyHeader("mtoa_constant_tags");
            if (IStringGeomParam::matches( *tagsHeader ))
            {
                IStringGeomParam param( arbGeomParams,  "mtoa_constant_tags" );
                if ( param.valid() )
                {
                    IStringGeomParam::prop_type::sample_ptr_type valueSample = param.getExpandedValue().getVals();
                    if ( param.getScope() == kConstantScope || param.getScope() == kUnknownScope)
                    {
                        Json::Value jtags;
                        Json::Reader reader;
                        if(reader.parse(valueSample->get()[0], jtags))
                        {
                            for( Json::ValueIterator itr = jtags.begin() ; itr != jtags.end() ; itr++ )
                            {
                                std::string tag = jtags[itr.key().asUInt()].asString();
                                tags.push_back(tag);
                            }
                        }
                    }

                }
            }
        }
    }
}


void getAllTags(IObject &iObj, std::vector<std::string> & tags, ProcArgs* args)
{
    while ( iObj )
    {
        getTags(iObj, tags, args);
        iObj = iObj.getParent();
    }
}


bool isVisible(IObject child, IXformSchema xs, ProcArgs* args)
{
    if(GetVisibility( child, ISampleSelector( args->frame / args->fps ) ) == kVisibilityHidden)
    {
        // check if the object is not forced to be visible
        std::string name = args->nameprefix + child.getFullName();

        if(args->linkAttributes)
        {
            for(std::vector<std::string>::iterator it=args->attributes.begin(); it!=args->attributes.end(); ++it)
            {
                    Json::Value attributes;
                    if(it->find("/") != string::npos)
                        if(name.find(*it) != string::npos)
                            attributes = args->attributesRoot[*it];


                    if(attributes.size() > 0)
                    {
                        for( Json::ValueIterator itr = attributes.begin() ; itr != attributes.end() ; itr++ )
                        {
                            std::string attribute = itr.key().asString();
                            if (attribute == "forceVisible")
                            {

                                bool vis = args->attributesRoot[*it][itr.key().asString()].asBool();
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

    if(args->linkAttributes)
    {
        for(std::vector<std::string>::iterator it=args->attributes.begin(); it!=args->attributes.end(); ++it)
        {
                Json::Value attributes;
                if(it->find("/") != string::npos)
                    if(name.find(*it) != string::npos)
                        attributes = args->attributesRoot[*it];


                if(attributes.size() > 0)
                {
                    for( Json::ValueIterator itr = attributes.begin() ; itr != attributes.end() ; itr++ )
                    {
                        std::string attribute = itr.key().asString();
                        if (attribute == "visibility")
                        {

                            AtUInt16 vis = args->attributesRoot[*it][itr.key().asString()].asInt();
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


void OverrideProperties(Json::Value & jroot, Json::Value jrootAttributes)
{
    for( Json::ValueIterator itr = jrootAttributes.begin() ; itr != jrootAttributes.end() ; itr++ )
    {
        const Json::Value paths = jrootAttributes[itr.key().asString()];
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

AtNode* createNetwork(IObject object, std::string prefix, ProcArgs & args)
{
    std::map<std::string,AtNode*> aShaders;
    Mat::IMaterial matObj(object, kWrapExisting);
    for (size_t i = 0, e = matObj.getSchema().getNumNetworkNodes(); i < e; ++i)
    {
        Mat::IMaterialSchema::NetworkNode abcnode = matObj.getSchema().getNetworkNode(i);
        std::string target = "<undefined>";
        abcnode.getTarget(target);
        if(target == "arnold")
        {
            std::string nodeType = "<undefined>";
            abcnode.getNodeType(nodeType);
            AiMsgDebug("Creating %s node named %s", nodeType.c_str(), abcnode.getName().c_str());
            AtNode* aShader = AiNode (nodeType.c_str());

            std::string name = prefix + "_" + abcnode.getName();

            AiNodeSetStr (aShader, "name", name.c_str());
            aShaders[abcnode.getName()] = aShader;

            args.createdNodes.push_back(aShader);

            // We set the default attributes
            ICompoundProperty parameters = abcnode.getParameters();
            if (parameters.valid())
            {
                for (size_t i = 0, e = parameters.getNumProperties(); i < e; ++i)
                {
                    const PropertyHeader & header = parameters.getPropertyHeader(i);

                    if (header.getName() == "name")
                        continue;

                    if (header.isArray())
                        setArrayParameter(parameters, header, aShader);

                    else
                        setParameter(parameters, header, aShader);
                }

            }

        }

    }

    // once every node is created, we can set the connections...
    for (size_t i = 0, e = matObj.getSchema().getNumNetworkNodes(); i < e; ++i)
    {
        Mat::IMaterialSchema::NetworkNode abcnode = matObj.getSchema().getNetworkNode(i);

        std::string target = "<undefined>";
        abcnode.getTarget(target);
        if(target == "arnold")
        {
            size_t numConnections = abcnode.getNumConnections();
            if(numConnections)
            {
                AiMsgDebug("linking nodes");
                std::string inputName, connectedNodeName, connectedOutputName;
                for (size_t j = 0; j < numConnections; ++j)
                {
                    if (abcnode.getConnection(j, inputName, connectedNodeName, connectedOutputName))
                    {
                        AiMsgDebug("Linking %s.%s to %s.%s", connectedNodeName.c_str(), connectedOutputName.c_str(), abcnode.getName().c_str(), inputName.c_str());
                        AiNodeLinkOutput(aShaders[connectedNodeName.c_str()], connectedOutputName.c_str(), aShaders[abcnode.getName().c_str()], inputName.c_str());
                    }
                }

            }
        }
    }

    // Getting the root node now ...
    std::string connectedNodeName = "<undefined>";
    std::string connectedOutputName = "<undefined>";
    if (matObj.getSchema().getNetworkTerminal(
                "arnold", "surface", connectedNodeName, connectedOutputName))
    {
        AtNode *root = aShaders[connectedNodeName.c_str()];
        if(root)
            AiNodeSetStr(root, "name", prefix.c_str());

        return root;
    }

    return NULL;

}

void ParseShaders(Json::Value jroot, std::string ns, std::string nameprefix, ProcArgs* args, AtByte type)
{
    // We have to lock here as we need to be sure that another thread is not checking the root while we are creating it here.
    GLOBAL_LOCK;
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
                    shaderNode = createNetwork(object, shaderName, *args);

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