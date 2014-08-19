/*
 * ABCGetTags.h
 *
 *  Created on: Jan 18, 2012
 *      Author: nozon
 */
#include <maya/MPxCommand.h>
#include <maya/MString.h>

#include <Alembic/AbcCoreFactory/IFactory.h>
#include <Alembic/AbcGeom/Visibility.h>
#include "Alembic/AbcGeom/All.h"
#include "Alembic/Abc/All.h"

#ifndef ABCGETTAGS_H_
#define ABCGETTAGS_H_

using namespace Alembic::AbcGeom;

class ABCGetTags : public MPxCommand
{
public:
//	ABCHierarchy();
//	virtual ~ABCHierarchy();
	static void		cleanup();
	MStatus doIt( const MArgList& args );
	bool isUndoable() const {return false;};
	static void* creator();

	void visitObject( IObject iObj, MStringArray* results );

	const char * 	filename () const {return m_filename.asChar();}
	const char * 	path () const {return m_path.asChar();}

	static const MString commandName;

private:
	MString m_filename;
	MString m_path;

};


#endif /* ABCGETTAGS_H_ */
