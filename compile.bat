rd /s /q build\win
mkdir "build\win" /p
cd build/win
cmake ../../
cmake --build . -- /p:Configuration=Release
cd ../../


rd /s /q build\test
mkdir "build\test" /p
cd build/test
cmake ../../test/
cmake --build . -- /p:Configuration=Release
cd ../../