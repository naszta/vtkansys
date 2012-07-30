#ifndef vtkansysExportH
#define vtkansysExportH 1

#ifdef WIN32
# ifdef vtkansys_EXPORTS
#  define vtkansysEXPORT __declspec( dllexport )
# else
#  define vtkansysEXPORT __declspec( dllimport )
# endif
#else
# define vtkansysEXPORT
#endif

#endif
