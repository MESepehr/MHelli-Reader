CXXFLAGS = -DQT_GUI_LIB -DQT_CORE_LIB -std=gnu++11
INCPATH = -I ~/Qt/5.7/clang_64/lib/QtGui.framework/Headers -I ~/Qt/5.7/clang_64/lib/QtCore.framework/Headers -I ~/Qt/5.7/clang_64/mkspecs/macx-clang -F/Users/sassanharadji/Qt/5.7/clang_64/lib
LIBS = -F/Users/sassanharadji/Qt/5.7/clang_64/lib -framework QtGui -framework QtCore

all:
	clang++ $(CXXFLAGS) $(INCPATH) -c -fPIC object.cpp -o object.o
	clang++ $(CXXFLAGS) $(INCPATH) -c -fPIC reader.cpp -o reader.o
	clang++ $(LIBS) -shared -Wl,-install_name,libreader.so -o libreader.so reader.o object.o
