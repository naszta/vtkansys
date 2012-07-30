#include "vtkvscAnsysCdbWriter.h"

#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPoints.h"
#include "vtkCellTypes.h"
#include "vtkCellType.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkFieldData.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"

#include "vtkansysVersion.h"

#include <vtksys/stl/map>
#include <vtksys/stl/utility>

#include <vtksys/ios/fstream>
#include <vtksys/ios/sstream>

#define ANSYS_NODE_BLOCK "NODE"
#define ANSYS_ELEM_BLOCK "ELEMENT"

namespace
{
  template <class T> inline void IntConverter( vtksys_ios::ostream &target, T number, std::streamsize Size = 8 )
  {
    target.width( Size );
    target.fill( ' ' );
    target << number;
  }

  template <class T> inline void DoubleConverter( vtksys_ios::ostream &target, T number, std::streamsize Prec = 8, std::streamsize Size = 16 )
  {
    target.width( Size );
    target.fill(' ');
    target << vtksys_ios::setprecision( Prec ) << vtksys_ios::scientific << number;
  }
}

class vtkvscAnsysCdbWriterPrivate
{
public:
  enum { SURFACE = 2,	YES = 1, NO = 0, NOT_KNOWN = -1	};
  enum { NON_LIN_VTK_TRIANGLE = 66665, NON_LIN_VTK_QUAD = 66666, ANSYS_SURFACE = 66667 };
  enum { AN_ET_1 = 1, AN_ET_2 = 2, AN_ET_3 = 3, AN_ET_4 = 4  };
  typedef vtksys_stl::map<vtkIdType,int> StorageType;

  vtkvscAnsysCdbWriterPrivate( void )
  {
    types.insert( vtksys_stl::make_pair<vtkIdType,int>(VTK_TETRA,AN_ET_1) );
    types.insert( vtksys_stl::make_pair<vtkIdType,int>(VTK_VOXEL,AN_ET_1) );
    types.insert( vtksys_stl::make_pair<vtkIdType,int>(VTK_HEXAHEDRON,AN_ET_1) );
    types.insert( vtksys_stl::make_pair<vtkIdType,int>(VTK_WEDGE,AN_ET_1) );
    types.insert( vtksys_stl::make_pair<vtkIdType,int>(VTK_TRIANGLE,AN_ET_2) );
    types.insert( vtksys_stl::make_pair<vtkIdType,int>(VTK_QUAD,AN_ET_2) );
    types.insert( vtksys_stl::make_pair<vtkIdType,int>(NON_LIN_VTK_TRIANGLE,AN_ET_3) );
    types.insert( vtksys_stl::make_pair<vtkIdType,int>(NON_LIN_VTK_QUAD,AN_ET_3) );
    types.insert( vtksys_stl::make_pair<vtkIdType,int>(ANSYS_SURFACE,AN_ET_4) );
    materials.insert( vtksys_stl::make_pair<vtkIdType,int>(ANSYS_SURFACE,SURFACE) );
  };
  void AddMaterial( vtkIdType mat, int non_linear )
  {
    StorageType::iterator lb = this->materials.lower_bound(mat);

    if ( ( lb != this->materials.end() ) && !( this->materials.key_comp()( mat, lb->first ) ) )
    {
      lb->second = non_linear;
      return;
    }
    this->materials.insert( lb, vtksys_stl::make_pair<vtkIdType,int>(mat,non_linear) );
  };
  int GetElementType( int cell_type, vtkIdType mat_type, int default_type )
  {
    StorageType::iterator iter = materials.find( mat_type );
    int non_lin = NOT_KNOWN;
    if ( iter != materials.end() )
      non_lin = iter->second;

    switch ( cell_type )
    {
    case VTK_TRIANGLE :
      switch ( non_lin )
      {
      case SURFACE :
        iter = this->types.find(ANSYS_SURFACE);
        break;
      case YES :
        iter = this->types.find(NON_LIN_VTK_TRIANGLE);
        break;
      default :
        iter = this->types.find(VTK_TRIANGLE);
      };
      break;
    case VTK_QUAD :
      switch ( non_lin )
      {
      case SURFACE :
        iter = this->types.find(ANSYS_SURFACE);
        break;
      case YES :
        iter = this->types.find(NON_LIN_VTK_QUAD);
        break;
      default :
        iter = this->types.find(VTK_QUAD);
      };
      break;
    default :
      iter = types.find( cell_type );
    };
    if ( iter == types.end() )
      return default_type;
    return iter->second;
  };
  void WriteTypes( ostream &os )
  {
    os << "ET," << AN_ET_1 << ",185\n";
    os << "ET," << AN_ET_2 << ",63\n";
    os << "ET," << AN_ET_3 << ",181\n";
    os << "ET," << AN_ET_4 << ",154\n";
  };
  void clear( void )
  {
    this->materials.clear();
    this->types.clear();
  };
private:
  StorageType materials;
  StorageType types;
};

vtkCxxRevisionMacro(vtkvscAnsysCdbWriter, vtkansys_VERSION_VTK );
vtkStandardNewMacro(vtkvscAnsysCdbWriter);

vtkvscAnsysCdbWriter::vtkvscAnsysCdbWriter( void )
: FileName(0), NodeSurfaces(0), ElemSurfaces(0), store(new vtkvscAnsysCdbWriterPrivate), DefaultElementType(0)
{
  this->SetNumberOfInputPorts(1);
	this->SetNumberOfOutputPorts(0);
}

vtkvscAnsysCdbWriter::~vtkvscAnsysCdbWriter( void )
{
	this->SetFileName(0);
  this->SetNodeSurfaces(0);
  this->SetElemSurfaces(0);
  if ( this->store != 0 )
  {
    delete this->store;
    this->store = 0;
  }
}

void vtkvscAnsysCdbWriter::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os,indent);

	os << indent << "File Name : ";
	if( this->FileName == 0 )
		os << "(none)" << endl;
	else
		os << this->FileName << endl;
}

vtkCxxSetObjectMacro(vtkvscAnsysCdbWriter,NodeSurfaces,vtkFieldData);
vtkCxxSetObjectMacro(vtkvscAnsysCdbWriter,ElemSurfaces,vtkFieldData);

void vtkvscAnsysCdbWriter::WriteData( void )
{
	vtkUnstructuredGrid * input = vtkUnstructuredGrid::SafeDownCast( this->GetInput() );

	// Description:
	// Check and prepare input
	if( input == 0 )
	{
		vtkErrorMacro( "Ack!! no input!!" );
		return;
	}

  this->store->clear();

	vtkIdType i = 0;
	input->BuildLinks();

	// Description:
	// Make and check file
	vtksys_ios::ofstream FileOut( this->FileName, vtksys_ios::ios_base::out | vtksys_ios::ios_base::trunc );
	if ( ! FileOut.is_open() )
	{
		vtkErrorMacro( "File cannot be created" );
		return;
	}

  // Description:
	// What will we write out
	vtkSmartPointer<vtkIdTypeArray> Material = vtkIdTypeArray::SafeDownCast( input->GetCellData()->GetArray( VTKANSYS_MATERIALS_NAME ) );
	if ( Material == 0 )
	{
		Material = vtkSmartPointer<vtkIdTypeArray>::New();
		Material->SetNumberOfValues( input->GetNumberOfCells() );
		memset( Material->GetPointer(0), 0, input->GetNumberOfCells() * sizeof(vtkIdType) );
	}
	vtkSmartPointer<vtkIdTypeArray> Real = vtkIdTypeArray::SafeDownCast( input->GetCellData()->GetArray( VTKANSYS_REALS_NAME ) );
	if ( Real == 0 )
	{
		Real = vtkSmartPointer<vtkIdTypeArray>::New();
		Real->SetNumberOfValues( input->GetNumberOfCells() );
		memset( Real->GetPointer(0), 0, input->GetNumberOfCells() * sizeof(vtkIdType) );
	}

	FileOut << "/PREP7\n";

	// Description:
	// Format the output stream
	FileOut << vtkstd::setprecision(8) << vtkstd::scientific;

	// Description:
	// Ansys and built in type association
	this->store->WriteTypes( FileOut );

	// Description:
	// Write out points
	vtkPoints * points = input->GetPoints();
	double point[3];

	if ( points->GetNumberOfPoints() > 0 )
	{
		FileOut << "NBLOCK,6,SOLID\n";
		FileOut << "(3i8,6e16.9)\n";
		for ( i = 0; i < points->GetNumberOfPoints(); ++i )
		{
			points->GetPoint( i, point );
			this->WriteNode( FileOut, i, point );
			FileOut << "\n";
		}
		FileOut << "N,R5.3,LOC,     -1,\n";
	}

	// Description:
	// Write out cells
	int elem_type = 0;
	vtkCell * cell = 0;
	if ( input->GetNumberOfCells() > 0 )
	{
		FileOut << "EBLOCK,19,SOLID\n";
		FileOut << "(19i8)\n";
		for ( i = 0; i < input->GetNumberOfCells(); ++i )
		{
      cell = input->GetCell(i);
      elem_type = this->store->GetElementType( cell->GetCellType(), Material->GetValue(i), this->DefaultElementType );
      this->WriteElem( FileOut, i, cell, Real->GetValue(i), Material->GetValue(i), elem_type );
      FileOut << "\n";
		}
		FileOut << "-1\n";
		FileOut << "EN,R5.5,ATTR,     -1\n";
	}

  // Description:
  // Write out user blocks
  if ( this->NodeSurfaces != 0 )
    this->WriteBlocks( FileOut, this->NodeSurfaces, ANSYS_NODE_BLOCK );
  if ( this->ElemSurfaces != 0 )
    this->WriteBlocks( FileOut, this->ElemSurfaces, ANSYS_ELEM_BLOCK );

	FileOut << "FINISH\n";

	FileOut.flush();
	FileOut.close();

	return;
}

int vtkvscAnsysCdbWriter::FillInputPortInformation( int port, vtkInformation * info )
{
	info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid" );
	return 1;
}

void vtkvscAnsysCdbWriter::WriteNode( vtksys_ios::ostream &target, vtkIdType index, double * XYZ )
{
	IntConverter( target, index + 1 );
	IntConverter( target, 0 );
	IntConverter( target, 0 );
	DoubleConverter( target, XYZ[0] );
	DoubleConverter( target, XYZ[1] );
	DoubleConverter( target, XYZ[2] );
}

void vtkvscAnsysCdbWriter::WriteElem( ostream &target, vtkIdType index, vtkCell * cell, int Real, int Material, int Type, int Secnum )
{
	switch ( cell->GetCellType() )
	{
	case VTK_TETRA :
		IntConverter( target, Material );
		IntConverter( target, Type );
		IntConverter( target, Real );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 8 );
		IntConverter( target, 0 );
		IntConverter( target, index + 1 );
		IntConverter( target, cell->GetPointId(0) + 1 );
		IntConverter( target, cell->GetPointId(1) + 1 );
		IntConverter( target, cell->GetPointId(2) + 1 );
		IntConverter( target, cell->GetPointId(2) + 1 );
		IntConverter( target, cell->GetPointId(3) + 1 );
		IntConverter( target, cell->GetPointId(3) + 1 );
		IntConverter( target, cell->GetPointId(3) + 1 );
		IntConverter( target, cell->GetPointId(3) + 1 );
		break;
	case VTK_VOXEL :
		IntConverter( target, Material );
		IntConverter( target, Type );
		IntConverter( target, Real );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 8 );
		IntConverter( target, 0 );
		IntConverter( target, index + 1 );
		IntConverter( target, cell->GetPointId(0) + 1 );
		IntConverter( target, cell->GetPointId(1) + 1 );
		IntConverter( target, cell->GetPointId(5) + 1 );
		IntConverter( target, cell->GetPointId(4) + 1 );
		IntConverter( target, cell->GetPointId(2) + 1 );
		IntConverter( target, cell->GetPointId(3) + 1 );
		IntConverter( target, cell->GetPointId(7) + 1 );
		IntConverter( target, cell->GetPointId(6) + 1 );
		break;
	case VTK_HEXAHEDRON :
		IntConverter( target, Material );
		IntConverter( target, Type );
		IntConverter( target, Real );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 8 );
		IntConverter( target, 0 );
		IntConverter( target, index + 1 );
		IntConverter( target, cell->GetPointId(0) + 1 );
		IntConverter( target, cell->GetPointId(1) + 1 );
		IntConverter( target, cell->GetPointId(2) + 1 );
		IntConverter( target, cell->GetPointId(3) + 1 );
		IntConverter( target, cell->GetPointId(4) + 1 );
		IntConverter( target, cell->GetPointId(5) + 1 );
		IntConverter( target, cell->GetPointId(6) + 1 );
		IntConverter( target, cell->GetPointId(7) + 1 );
		break;
	case VTK_WEDGE :
		IntConverter( target, Material );
		IntConverter( target, Type );
		IntConverter( target, Real );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 8 );
		IntConverter( target, 0 );
		IntConverter( target, index + 1 );
		IntConverter( target, cell->GetPointId(0) + 1 );
		IntConverter( target, cell->GetPointId(1) + 1 );
		IntConverter( target, cell->GetPointId(2) + 1 );
		IntConverter( target, cell->GetPointId(2) + 1 );
		IntConverter( target, cell->GetPointId(3) + 1 );
		IntConverter( target, cell->GetPointId(4) + 1 );
		IntConverter( target, cell->GetPointId(5) + 1 );
		IntConverter( target, cell->GetPointId(5) + 1 );
		break;
	case VTK_TRIANGLE :
		IntConverter( target, Material );
		IntConverter( target, Type );
		IntConverter( target, Real );
		IntConverter( target, Secnum );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 4 );
		IntConverter( target, 0 );
		IntConverter( target, index + 1 );
		IntConverter( target, cell->GetPointId(0) + 1 );
		IntConverter( target, cell->GetPointId(1) + 1 );
		IntConverter( target, cell->GetPointId(2) + 1 );
		IntConverter( target, cell->GetPointId(2) + 1 );
		break;
	case VTK_QUAD :
		IntConverter( target, Material );
		IntConverter( target, Type );
		IntConverter( target, Real );
		IntConverter( target, Secnum );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 0 );
		IntConverter( target, 4 );
		IntConverter( target, 0 );
		IntConverter( target, index + 1 );
		IntConverter( target, cell->GetPointId(0) + 1 );
		IntConverter( target, cell->GetPointId(1) + 1 );
		IntConverter( target, cell->GetPointId(2) + 1 );
		IntConverter( target, cell->GetPointId(3) + 1 );
		break;
	default:
		vtkDebugMacro( << "Only VTK_TETRA, VTK_WEDGE, VTK_VOXEL, VTK_HEXAHEDRON, VTK_TRIANGLE and VTK_QUAD are supported! Mesh might be corrupt!" );
	};
}

void vtkvscAnsysCdbWriter::WriteBlocks( ostream &target, vtkFieldData * blocks, const char * name )
{
	vtkIdTypeArray * block = 0;
	for ( vtkIdType i = 0; i < blocks->GetNumberOfArrays(); ++i )
	{
		block = vtkIdTypeArray::SafeDownCast( blocks->GetArray(i) );
		if ( block == 0 )
			continue;
		this->WriteBlock( target, block, name );
	}
}

void vtkvscAnsysCdbWriter::WriteBlock( ostream &target, vtkIdTypeArray * block, const char * name, int NumberOfTags, int LengthOfNumber )
{
	target << "CMBLOCK," << block->GetName() << "," << name << "," << block->GetNumberOfTuples() << "\n";
	target.width(0);
	target << "(" << NumberOfTags << "i" << LengthOfNumber << ")\n";

	int Line = 0;
	for ( vtkIdType i = 0; i <= block->GetMaxId(); ++i )
	{
		IntConverter( target, block->GetValue(i) + 1, LengthOfNumber );

		if ( ++Line == NumberOfTags )
		{
			target << "\n";
			Line = 0;
		}
	}
	if ( Line != 0 )
		target << "\n";
}


void vtkvscAnsysCdbWriter::WriteThicknesses( ostream &target, vtkIdTypeArray * ids, vtkDoubleArray * thinkness )
{
	vtkIdType NumberOfThicks = vtksys_stl::min( thinkness->GetNumberOfTuples(), ids->GetNumberOfTuples() );
	if ( NumberOfThicks <= 0 )
		return;

	int size = thinkness->GetNumberOfComponents();
	vtksys_stl::vector<double> temp_array(size);

	for ( vtkIdType i = 0; i < NumberOfThicks; ++i )
	{
		thinkness->GetTupleValue( i, &temp_array[0] );
		this->WriteThickness( target, ids->GetValue(i), &temp_array[0], size );
	}
}

void vtkvscAnsysCdbWriter::WriteThickness( ostream &target, vtkIdType id, double * thinkness, int size )
{
	target << "SECTYPE," << id << ",SHELL,,\n";
	target << "SECOFFSET,MID\n";
	target << "SECBLOCK,1\n";
	target << thinkness[0] << ",1,0.0,3\n";
	target << "SECCONTROLS,0.0,0.0,0.0,0.0,1.0,1.0,1.0,,,,,0.0\n";
}
