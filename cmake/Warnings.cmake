# emu_set_warnings(<target>)
#
# Applies the project warning set to one of our own targets. Never call it on
# fetched dependencies or generated code: their warnings would bury ours.
function(emu_set_warnings target)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic
      -Wshadow -Wconversion -Wsign-conversion)
    if(EMU_WERROR)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()
