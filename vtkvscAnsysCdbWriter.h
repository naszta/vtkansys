#ifndef vtkvscAnsysCdbWriterH
#define vtkvscAnsysCdbWriterH

#include "vtkWriter.h"
#include "vtkansysExport.h"

class vtkCell;
class vtkFieldData;
class vtkIdTypeArray;
class vtkDoubleArray;

class vtkvscAnsysCdbWriterPrivate;

#define VTKANSYS_MATERIALS_NAME "Materials"
#define VTKANSYS_REALS_NAME     "Reals"

class vtkansysEXPORT vtkvscAnsysCdbWriter : public vtkWriter
{
public:
	static vtkvscAnsysCdbWriter* New();
	vtkTypeRevisionMacro(vtkvscAnsysCdbWriter,vtkWriter);
	void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetMacro(DefaultElementType,int);
  vtkGetMacro(DefaultElementType,int);
	vtkSetStringMacro( FileName );
	vtkGetStringMacro( FileName );

  vtkGetObjectMacro(NodeSurfaces,vtkFieldData);
  virtual void SetNodeSurfaces( vtkFieldData * _arg );
  vtkGetObjectMacro(ElemSurfaces,vtkFieldData);
  virtual void SetElemSurfaces( vtkFieldData * _arg );

protected:
	vtkvscAnsysCdbWriter();
	~vtkvscAnsysCdbWriter();

	char * FileName;
  int DefaultElementType;

  vtkFieldData * NodeSurfaces;
  vtkFieldData * ElemSurfaces;

	void WriteData( void );
	int FillInputPortInformation( int port, vtkInformation * info );

	void WriteNode( ostream &target, vtkIdType index, double * XYZ );
	void WriteElem( ostream &target, vtkIdType index, vtkCell * cell, int Real = 0, int Material = 0, int Type = 0, int Secnum = 0 );
	void WriteBlocks( ostream &target, vtkFieldData * blocks, const char * name );
	void WriteBlock( ostream &target, vtkIdTypeArray * block, const char * name, int NumberOfTags = 8, int LengthOfNumber = 8 );
	void WriteThicknesses( ostream &target, vtkIdTypeArray * ids, vtkDoubleArray * thinkness );
	void WriteThickness( ostream &target, vtkIdType id, double * thinkness, int size );

private:
	vtkvscAnsysCdbWriter(const vtkvscAnsysCdbWriter&);  // Not implemented.
	void operator=(const vtkvscAnsysCdbWriter&);  // Not implemented.

  vtkvscAnsysCdbWriterPrivate * store;
};

#endif
