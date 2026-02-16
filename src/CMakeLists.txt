add_library(chess_engine STATIC
  attacks.cpp
  eval.cpp
  movegen.cpp
  position.cpp
  search.cpp
  uci.cpp
  zobrist.cpp
)

target_include_directories(chess_engine PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)

# Position independent for flexibility
set_target_properties(chess_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Compiler features and warnings (modern CMake + generator expressions)
target_compile_features(chess_engine PUBLIC cxx_std_17)
target_compile_options(chess_engine PRIVATE
  $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>>:/W4>
  $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-Wall;-Wextra;-Wpedantic>
)

# Executable
add_executable(chess main.cpp)
target_link_libraries(chess PRIVATE chess_engine)
