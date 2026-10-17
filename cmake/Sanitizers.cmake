# emu_enable_sanitizers(<target>)
#
# Adds -fsanitize=<list> to compile and link options when EMU_SANITIZE is set
# (for example "address;undefined"). Does nothing otherwise.
function(emu_enable_sanitizers target)
  if(NOT EMU_SANITIZE)
    return()
  endif()
  list(JOIN EMU_SANITIZE "," sanitizers)
  target_compile_options(${target} PRIVATE
    -fsanitize=${sanitizers} -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_options(${target} PRIVATE -fsanitize=${sanitizers})
endfunction()
