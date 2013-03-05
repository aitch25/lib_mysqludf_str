string_utils_test: string_utils_test.o x_strlcpy.o
	$(CXX) -o $@ string_utils_test.o x_strlcpy.o -lboost_unit_test_framework-mt -lstdc++

string_utils_test.o: ../../string_utils.h ../../x_strlcpy.c
	$(CXX) -c -o $@ -I ../.. string_utils_test.cpp

x_strlcpy.o: ../../string_utils.h ../../x_strlcpy.c
	$(CXX) -c -o $@ -I ../.. ../../x_strlcpy.c
