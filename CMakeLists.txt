cmake_minimum_required (VERSION 2.8.11)
project (CGARACER)

include_directories(
	/Users/joeld/Toolkits/tk_raylib/src/
	/usr/local/include
	/Users/joeld/Toolkits/rocket/external/rocket/lib	
	/Users/joeld/Toolkits/soloud/include/
	/usr/local/include
)

link_directories(
	/Users/joeld/Toolkits/tk_raylib/release/osx/
	/usr/local/Cellar/glfw3/3.2.1/lib 	
	/Users/joeld/Toolkits/rocket/t2-output/macosx-clang-debug-default/
	/Users/joeld/Toolkits/soloud/lib/
)

set ( LIBS ${LIBS}
		"-framework OpenGL"
		"-framework OpenAl"
		"-framework Cocoa"
		"-framework AudioToolbox"
		raylib
		glfw3
		sync
		soloud_static
      )

add_executable (cga_racer 
				cgajam_racer.cpp
				carsim.cpp
				track.cpp
				dither.cpp
				engine_noise.cpp				
				)

#target_compile_options( cga_racer PUBLIC -fno-rtti -g -O0)
target_compile_options( cga_racer PUBLIC -fno-rtti)
target_link_libraries( cga_racer ${LIBS} )
set_target_properties( cga_racer PROPERTIES
		       		 RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ )