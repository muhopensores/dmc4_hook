PROJECT(minhook)

add_library(minhook STATIC
            src/buffer.c
            src/buffer.h
            src/hook.c
            src/trampoline.c
            src/trampoline.h
            src/hde/hde32.c
            src/hde/hde32.h
            src/hde/hde64.c
            src/hde/hde64.h
            src/hde/pstdint.h
            src/hde/table32.h
            src/hde/table64.h)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER minhook)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC include)
