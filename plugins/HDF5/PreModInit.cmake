#_______________________Pmake___________________________________________________
#
#	Makefile : 	Seis Pre-ModInit
# 	Feb 2018	Bert
#_______________________________________________________________________________


list ( APPEND OD_MODULE_INCLUDESYSPATH ${HDF5_INCLUDE_DIRS} )

set( OD_MODULE_EXTERNAL_LIBS ${HDF5_LIBRARIES} )

set( OD_EXEC_DEP_LIBS ${OD_EXEC_DEP_LIBS} ${OD_MODULE_EXTERNAL_LIBS} )
set( OD_RUNTIMELIBS ${OD_RUNTIMELIBS} ${OD_MODULE_EXTERNAL_LIBS} )
