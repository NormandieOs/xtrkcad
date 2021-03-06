
# $Header: /home/dmarkle/xtrkcad-fork-cvs/xtrkcad/app/lib/params/CMakeLists.txt,v 1.9 2008-06-04 18:53:04 m_fischer Exp $
#
# CMakeList for the params directory
#
# Several xtp files are created from text definitions, all xtp files and the
# created xtp files are installed.
#

FILE(GLOB XTP_FILES *.xtp)

SET( outFiles )

#  create param files from car definitions

ADD_EXECUTABLE( mkcarpart mkcarpart.c )

FILE(GLOB inFiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*.cars")

FOREACH(infileName ${inFiles})
    # Generate output file name
    STRING(REGEX REPLACE ".cars\$" ".xtp" outfileName "${infileName}")
    SET(outfile "${CMAKE_CURRENT_BINARY_DIR}/${outfileName}")
    # Generate input file name
    SET(infile "${CMAKE_CURRENT_SOURCE_DIR}/${infileName}")
    # Custom command to do the processing
    ADD_CUSTOM_COMMAND(OUTPUT "${outfile}"
        COMMAND mkcarpart "${infile}" "${outfile}"
        DEPENDS "${infile}" mkcarpart
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    # Finally remember the output file for dependencies
    SET(outFiles ${outFiles} "${outfile}")
ENDFOREACH(infileName)

# create param files from  structure definitions

ADD_EXECUTABLE( mkstruct mkstruct.c )

FILE(GLOB inFiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*.struct")

# define build commands for all struct files
FOREACH(infileName ${inFiles})
    # Generate output file name
    STRING(REGEX REPLACE ".struct\$" ".xtp" outfileName "${infileName}")
    SET(outfile "${CMAKE_CURRENT_BINARY_DIR}/${outfileName}")
    # Generate input file name
    SET(infile "${CMAKE_CURRENT_SOURCE_DIR}/${infileName}")
    # Custom command to do the processing
    ADD_CUSTOM_COMMAND(OUTPUT "${outfile}"
        COMMAND mkstruct "${infile}" "${outfile}"
        DEPENDS "${infile}" mkstruct
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" )
    # Finally remember the output file for dependencies
    SET(outFiles ${outFiles} "${outfile}")
ENDFOREACH(infileName)


ADD_CUSTOM_TARGET(xtpfiles ALL DEPENDS ${outFiles})

INSTALL(
	FILES ${XTP_FILES} t-trak-notes.txt ${outFiles}
	DESTINATION ${XTRKCAD_SHARE_INSTALL_DIR}/params
	)
