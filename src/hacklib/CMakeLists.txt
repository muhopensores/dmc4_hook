PROJECT(hacklib)

SET(FILES_CPP
    src/Main.cpp
    src/Hooker.cpp
    src/PatternScanner.cpp
    src/Patch.cpp
    src/GfxOverlay.cpp
    src/WindowOverlay.cpp
    src/Logging.cpp
    src/ExeFile.cpp
    src/IDrawer.cpp
    src/DrawerOpenGL.cpp
    src/CrashHandler.cpp
    )
SET(FILES_H
    include/hacklib/MessageBox.h
    include/hacklib/Main.h
    include/hacklib/Injector.h
    include/hacklib/Hooker.h
    include/hacklib/ForeignClass.h
    include/hacklib/PatternScanner.h
    include/hacklib/ImplementMember.h
    include/hacklib/Rng.h
    include/hacklib/Timer.h
    include/hacklib/PageAllocator.h
    include/hacklib/Patch.h
    include/hacklib/ExeFile.h
    include/hacklib/Handles.h
    include/hacklib/Logging.h
    include/hacklib/CrashHandler.h
    include/hacklib/Memory.h
    include/hacklib/IDrawer.h
    include/hacklib/DrawerOpenGL.h
    include/hacklib/Math.h
    )

IF(WIN32)
    SET(FILES_CPP ${FILES_CPP}
        src/MessageBox_WIN32.cpp
        src/D3DDeviceFetcher.cpp
        src/ConsoleEx.cpp
        src/Hooker_VEH.cpp
        src/Injector_WIN32.cpp
        src/Input.cpp
        src/ExeFile_WIN32.cpp
        src/GfxOverlay_WIN32.cpp
        src/WindowOverlay_WIN32.cpp
        src/Main_WIN32.cpp
        src/CrashHandler_WIN32.cpp
        src/Memory_WIN32.cpp
        src/DrawerD3D.cpp
        )
    SET(FILES_H ${FILES_H}
        include/hacklib/D3DDeviceFetcher.h
        include/hacklib/GfxOverlay.h
        include/hacklib/WindowOverlay.h
        include/hacklib/ConsoleEx.h
        include/hacklib/Input.h
        include/hacklib/DrawerD3D.h
        )
ELSEIF(UNIX)
    SET(FILES_CPP ${FILES_CPP}
        src/GfxOverlay_UNIX.cpp
        src/WindowOverlay_UNIX.cpp
        src/Injector_UNIX.cpp
        src/ExeFile_UNIX.cpp
        src/MessageBox_UNIX.cpp
        src/Main_UNIX.cpp
        src/CrashHandler_UNIX.cpp
        src/Memory_UNIX.cpp
        )
    SET(FILES_H ${FILES_H}
        include/hacklib/GfxOverlay_UNIX.h
        )
ENDIF()

ADD_LIBRARY(${PROJECT_NAME} STATIC
    ${FILES_CPP}
    ${FILES_H}
)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER hacklib)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC include)

IF(WIN32)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} d3d9.lib d3dx9.lib d3d11.lib dwmapi.lib psapi.lib OpenGL32.lib)
ELSEIF(UNIX)
    FIND_PACKAGE(Threads REQUIRED)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} X11 Xrender Xfixes GL GLEW dl ${CMAKE_THREAD_LIBS_INIT})
    TARGET_COMPILE_OPTIONS(${PROJECT_NAME} PUBLIC "-pthread")
ENDIF()
