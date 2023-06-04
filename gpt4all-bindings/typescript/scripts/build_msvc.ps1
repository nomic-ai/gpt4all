# thanks csharp maintainer
# slightly modified script because
# Running this scripts assumes cwd is typescript
#

Remove-Item -Force -Recurse runtimes\win-x64\msvc -ErrorAction SilentlyContinue
mkdir runtimes\win-x64\msvc\build | Out-Null
cmake -A x64 -S ..\..\gpt4all-backend -B runtimes\win-x64\msvc\build
cmake --build .\runtimes\win-x64\msvc\build 
cp .\runtimes\win-x64\msvc\build\bin\Release\*.dll .\runtimes\win-x64
