include(FetchContent)

FetchContent_Declare(
	Catch2
	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
	GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(perft_tests perft_tests.cpp)
target_link_libraries(perft_tests PRIVATE chess_engine Catch2::Catch2WithMain)

add_test(NAME perft_tests COMMAND perft_tests)