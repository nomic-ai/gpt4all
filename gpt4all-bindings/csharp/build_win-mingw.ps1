$ROOT_DIR = '.\runtimes\win-x64'
$BUILD_DIR = '.\runtimes\win-x64\build\mingw'
$LIBS_DIR = '.\runtimes\win-x64\native'

# cleanup env
Remove-Item -Force -Recurse $ROOT_DIR -ErrorAction SilentlyContinue | Out-Null
mkdir $BUILD_DIR | Out-Null
mkdir $LIBS_DIR  | Out-Null

# build
cmake -G "MinGW Makefiles" -S ..\..\gpt4all-backend -B $BUILD_DIR
cmake --build $BUILD_DIR --parallel --config Release

# copy native dlls
cp "C:\ProgramData\mingw64\mingw64\bin\*dll" $LIBS_DIR
cp "$BUILD_DIR\bin\*.dll" $LIBS_DIR
