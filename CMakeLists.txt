cmake_minimum_required (VERSION 2.8.11)
project (CGARACER)

include_directories(
	/Users/joeld/Toolkits/tk_raylib/src/
	/usr/local/include
	/Users/joeld/Toolkits/rocket/external/rocket/lib	
	/Users/joeld/Toolkits/soloud/include/
	/usr/local/include
	${CMAKE_SOURCE_DIR}/imgui
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

set (IMGUI_SRCS
		imgui/gl3w.h
		imgui/glcorearb.h
		imgui/imconfig.h
		imgui/imgui.h
		imgui/imgui_impl_glfw_gl3.h
		imgui/imgui_internal.h
		imgui/stb_rect_pack.h
		imgui/stb_textedit.h
		imgui/stb_truetype.h
		imgui/stb_image.h
		imgui/stb_image_write.h

		imgui/gl3w.c
		imgui/imgui.cpp
		imgui/imgui_draw.cpp
		imgui/imgui_impl_glfw_gl3.cpp
	)

add_executable (cga_racer 
				${IMGUI_SRCS}
				cgajam_racer.cpp
				carsim.cpp
				track.cpp
				dither.cpp
				)

target_link_libraries( cga_racer ${LIBS} )
set_target_properties( cga_racer PROPERTIES
		       		 RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ )