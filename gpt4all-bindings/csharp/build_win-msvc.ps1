Remove-Item -Force -Recurse .\runtimes\win-x64\msvc -ErrorAction SilentlyContinue
mkdir .\runtimes\win-x64\msvc\build | Out-Null
cmake -G "Visual Studio 17 2022" -A X64 -S ..\..\gpt4all-backend -B .\runtimes\win-x64\msvc\build
cmake --build .\runtimes\win-x64\msvc\build --parallel --config Release
cp .\runtimes\win-x64\msvc\build\bin\Release\*.dll .\runtimes\win-x64
mv .\runtimes\win-x64\llmodel.dll .\runtimes\win-x64\libllmodel.dll