/*
 * ABCHierarchy.h
 *
 *  Created on: Jan 18, 2012
 *      Author: nozon
 */
#include <maya/MPxCommand.h>
#include <maya/MString.h>

#ifndef ABCHIERARCHY_H_
#define ABCHIERARCHY_H_

class ABCHierarchy : public MPxCommand
{
public:
//	ABCHierarchy();
//	virtual ~ABCHierarchy();
	static void		cleanup();
	MStatus doIt( const MArgList& args );
	bool isUndoable() const {return false;};
	static void* creator();

	const char * 	filename () const {return m_filename.asChar();}
	const char * 	path () const {return m_path.asChar();}

	static const MString commandName;

private:
	MString m_filename;
	MString m_path;

};


#endif /* ABCHIERARCHY_H_ */
