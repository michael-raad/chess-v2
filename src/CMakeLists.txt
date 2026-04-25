add_library(chess_engine STATIC
  attacks.cpp
  choppedfish_engine.cpp
  experimental_engine.cpp
  game.cpp
  gui.cpp
  movegen.cpp
  position.cpp
  random_engine.cpp
  search.cpp
  uci_client.cpp
  zobrist.cpp
)

target_include_directories(chess_engine PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/include
)

# Position independent for flexibility
set_target_properties(chess_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(chess_engine PRIVATE sfml-graphics sfml-window sfml-system)

# Compiler features and warnings (modern CMake + generator expressions)
target_compile_features(chess_engine PUBLIC cxx_std_17)
target_compile_options(chess_engine PRIVATE
  $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>>:/W4>
  $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-Wall;-Wextra;-Wpedantic>
)

# Executable - GUI version
add_executable(chess main.cpp)
target_link_libraries(chess PRIVATE chess_engine)

# Executable - UCI White (ExperimentalEngine)
add_executable(chess_uci_white main_uci.cpp uci.cpp)
target_link_libraries(chess_uci_white PRIVATE chess_engine)
target_compile_definitions(chess_uci_white PRIVATE
  UCI_ENGINE_TYPE=1
)

# Executable - UCI Black (ChoppedfishEngine)
add_executable(chess_uci_black main_uci.cpp uci.cpp)
target_link_libraries(chess_uci_black PRIVATE chess_engine)
target_compile_definitions(chess_uci_black PRIVATE
  UCI_ENGINE_TYPE=2
)
