#include "ReadInstancer.h"
#include "../../../common/to_string_patch.h"

namespace
{
using namespace Alembic::AbcGeom;


void getSampleTimes(
    IPoints & prim,
    ProcArgs & args,
    SampleTimeSet & sampleTimes
    )
{
    Alembic::AbcGeom::IPointsSchema  &ps = prim.getSchema();
    TimeSamplingPtr ts = ps.getTimeSampling();

    sampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );
}

}

void WalkObjectForInstancer( IObject & parent, const ObjectHeader &i_ohead, ProcArgs &args,
             PathList::const_iterator I, PathList::const_iterator E,
                    MatrixSampleMap * xformSamples)
{
    IObject nextParentObject = parent.getChild(i_ohead.getName());
    std::auto_ptr<MatrixSampleMap> concatenatedXformSamples;

	const ObjectHeader& ohead = parent.isChildInstance(i_ohead.getName()) ? nextParentObject.getHeader() : i_ohead;

    if ( IXform::matches( ohead ) )
    {
        IXform xform( parent, ohead.getName() );
        IXformSchema &xs = xform.getSchema();

        IObject child = IObject( parent, ohead.getName() );

        // also check visibility flags

        {
            if ( xs.getNumOps() > 0 )
            {
                TimeSamplingPtr ts = xs.getTimeSampling();
                size_t numSamples = xs.getNumSamples();

                SampleTimeSet sampleTimes;
                GetRelevantSampleTimes( args, ts, numSamples, sampleTimes,
                        xformSamples);
                MatrixSampleMap localXformSamples;

                MatrixSampleMap * localXformSamplesToFill = 0;

                concatenatedXformSamples.reset(new MatrixSampleMap);

                if ( !xformSamples )
                {
                    // If we don't have parent xform samples, we can fill
                    // in the map directly.
                    localXformSamplesToFill = concatenatedXformSamples.get();
                }
                else
                {
                    //otherwise we need to fill in a temporary map
                    localXformSamplesToFill = &localXformSamples;
                }


                for (SampleTimeSet::iterator I = sampleTimes.begin();
                        I != sampleTimes.end(); ++I)
                {
                    XformSample sample = xform.getSchema().getValue(
                            Abc::ISampleSelector(*I));
                    (*localXformSamplesToFill)[(*I)] = sample.getMatrix();
                }
                if ( xformSamples )
                {
                    ConcatenateXformSamples(args,
                            *xformSamples,
                            localXformSamples,
                            *concatenatedXformSamples.get());
                }


                xformSamples = concatenatedXformSamples.get();
            }

            nextParentObject = xform;
        }
    }
    else if ( IPoints::matches( ohead ) )
    {
        IPoints points( parent, ohead.getName() );

	    SampleTimeSet sampleTimes;
	    getSampleTimes(points, args, sampleTimes);

		Alembic::AbcGeom::IPointsSchema  &ps = points.getSchema();
		TimeSamplingPtr ts = ps.getTimeSampling();

		SampleTimeSet singleSampleTimes;
		singleSampleTimes.insert( ts->getFloorIndex(args.frame / args.fps, ps.getNumSamples()).second );

		ICompoundProperty arbGeomParams = ps.getArbGeomParams();
		ISampleSelector frameSelector( *singleSampleTimes.begin() );

        Alembic::AbcGeom::IPointsSchema::Sample sample = ps.getValue( frameSelector );

		size_t pSize = sample.getPositions()->size();
		Alembic::Abc::P3fArraySamplePtr v3ptr = sample.getPositions();

		ICompoundProperty arbPointsParams = ps.getArbGeomParams();
		for ( size_t i = 0; i < arbPointsParams.getNumProperties(); ++i )
		{
			const PropertyHeader &propHeader = arbPointsParams.getPropertyHeader( i );
			const std::string &propName = propHeader.getName();
		}

		AtNode* proc = args.proceduralNode;
		for ( size_t pId = 0; pId < pSize; ++pId )
		{
			Alembic::Abc::V3f pos = (*v3ptr)[pId];
            AtVector apos;
            apos.x = pos.x;
            apos.y = pos.y;
            apos.z = pos.z;

			AtMatrix outMtx = AiM4Translation(apos);

			AtNode* instance = AiNode("procedural");
			std::string newName(AiNodeGetName(proc));
			newName = newName + "_" + to_string(pId);
			AiNodeSetStr(instance, "name", newName.c_str());
			AiNodeSetStr(instance, "dso", AiNodeGetStr(proc, "dso"));
			std::string dataField(AiNodeGetStr(proc, "data"));
			std::smatch what;
			if(std::regex_match(dataField, what, e))
				dataField = what[1].str() + newName + what[3].str();

			AiNodeSetStr(instance, "data", dataField.c_str());
			AiNodeSetMatrix(instance, "matrix", outMtx);

			AiNodeSetBool(instance, "load_at_init", true);

			if (AiNodeLookUpUserParameter(proc, "skipJsonFile") != NULL )
			{
				AiNodeDeclare(instance, "skipJsonFile", "constant BOOL");
				AiNodeSetBool(instance, "skipJsonFile", AiNodeGetBool(proc, "skipJsonFile"));			
			}
			if (AiNodeLookUpUserParameter(proc, "skipShaders") != NULL )
			{
				AiNodeDeclare(instance, "skipShaders", "constant BOOL");
				AiNodeSetBool(instance, "skipShaders", AiNodeGetBool(proc, "skipShaders"));		
			}
			if (AiNodeLookUpUserParameter(proc, "skipAttributes") != NULL )
			{
				AiNodeDeclare(instance, "skipAttributes", "constant BOOL");
				AiNodeSetBool(instance, "skipAttributes", AiNodeGetBool(proc, "skipAttributes"));		
			}
			if (AiNodeLookUpUserParameter(proc, "skipDisplacements") != NULL )
			{
				AiNodeDeclare(instance, "skipDisplacements", "constant BOOL");
				AiNodeSetBool(instance, "skipDisplacements", AiNodeGetBool(proc, "skipDisplacements"));		
			}
			if (AiNodeLookUpUserParameter(proc, "skipAttributes") != NULL )
			{
				AiNodeDeclare(instance, "skipAttributes", "constant BOOL");
				AiNodeSetBool(instance, "skipAttributes", AiNodeGetBool(proc, "skipAttributes"));		
			}
			if (AiNodeLookUpUserParameter(proc, "skipLayers") != NULL )
			{
				AiNodeDeclare(instance, "skipLayers", "constant BOOL");
				AiNodeSetBool(instance, "skipLayers", AiNodeGetBool(proc, "skipLayers"));		
			}

			

			if (AiNodeLookUpUserParameter(proc, "abcShaders") !=NULL )
			{
				AiNodeDeclare(instance, "abcShaders", "constant STRING");
				AiNodeSetStr(instance, "abcShaders", AiNodeGetStr(proc, "abcShaders"));		
			}
			if (AiNodeLookUpUserParameter(proc, "uvsArchive") !=NULL )
			{
				AiNodeDeclare(instance, "uvsArchive", "constant STRING");
				AiNodeSetStr(instance, "uvsArchive", AiNodeGetStr(proc, "uvsArchive"));			
			}
			if (AiNodeLookUpUserParameter(proc, "jsonFile") !=NULL )
			{
				AiNodeDeclare(instance, "jsonFile", "constant STRING");
				AiNodeSetStr(instance, "jsonFile", AiNodeGetStr(proc, "jsonFile"));
			}
			if (AiNodeLookUpUserParameter(proc, "secondaryJsonFile") !=NULL )
			{
				AiNodeDeclare(instance, "secondaryJsonFile", "constant STRING");
				AiNodeSetStr(instance, "secondaryJsonFile", AiNodeGetStr(proc, "secondaryJsonFile"));
			}
			if (AiNodeLookUpUserParameter(proc, "shadersAssignation") !=NULL )
			{
				AiNodeDeclare(instance, "shadersAssignation", "constant STRING");		
				AiNodeSetStr(instance, "shadersAssignation", AiNodeGetStr(proc, "shadersAssignation"));
			}
			if (AiNodeLookUpUserParameter(proc, "displacementsAssignation") !=NULL )
			{
				AiNodeDeclare(instance, "displacementsAssignation", "constant STRING");
				AiNodeSetStr(instance, "displacementsAssignation", AiNodeGetStr(proc, "displacementsAssignation"));
			}
			if (AiNodeLookUpUserParameter(proc, "attributes") !=NULL )
			{
				AiNodeDeclare(instance, "attributes", "constant STRING");
				AiNodeSetStr(instance, "attributes", AiNodeGetStr(proc, "attributes"));
			}

			if (AiNodeLookUpUserParameter(proc, "layersOverride") !=NULL )
			{
				AiNodeDeclare(instance, "layersOverride", "constant STRING");
				AiNodeSetStr(instance, "layersOverride", AiNodeGetStr(proc, "layersOverride"));
			}
			if (AiNodeLookUpUserParameter(proc, "shadersNamespace") !=NULL )
			{
				AiNodeDeclare(instance, "shadersNamespace", "constant STRING");
				AiNodeSetStr(instance, "shadersNamespace", AiNodeGetStr(proc, "shadersNamespace"));
			}

			args.createdNodes->addNode(instance);

		}

        nextParentObject = points;
    }
      else
    {
        nextParentObject = parent.getChild(ohead.getName());
    }

    if ( nextParentObject.valid() )
    {
        //std::cerr << nextParentObject.getFullName() << std::endl;

        if ( I == E )
        {
            for ( size_t i = 0; i < nextParentObject.getNumChildren() ; ++i )
            {
                
                WalkObjectForInstancer( nextParentObject,
                            nextParentObject.getChildHeader( i ),
                            args, I, E, xformSamples);
            }
        }
        else
        {
            const ObjectHeader *nextChildHeader =
                nextParentObject.getChildHeader( *I );
            
            if ( nextChildHeader != NULL )
            {
                WalkObjectForInstancer( nextParentObject, *nextChildHeader, args, I+1, E,
                    xformSamples);
            }
        }
    }



}
