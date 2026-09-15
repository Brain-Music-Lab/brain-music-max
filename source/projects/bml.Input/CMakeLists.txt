# Copyright 2018 The Min-DevKit Authors. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)


#############################################################
# MAX EXTERNAL
#############################################################


include_directories( 
	"${C74_INCLUDES}"
)

set( SOURCE_FILES
	${PROJECT_NAME}.cpp
)

add_library( 
	${PROJECT_NAME}
	MODULE
	${SOURCE_FILES}
)

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)


#############################################################
# UNIT TEST
#############################################################
# set( SOURCE_FILES_test
# 	${PROJECT_NAME}_test.cpp
# )

# add_library( 
# 	${PROJECT_NAME}_test
# 	MODULE
# 	${SOURCE_FILES_test}
# )

include(${C74_MIN_API_DIR}/test/min-object-unittest.cmake)
