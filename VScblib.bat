rem SET ILMBASE_ROOT=%cd%/../../../../../../Source/ThirdParty/openexr/Deploy/OpenEXR-2.3.0/OpenEXR
rem SET HDF5_ROOT=%cd%/AlembicDeploy/VS2015/x64/
mkdir build
cd build
mkdir VS2019
cd VS2019
rmdir /s /q cblib
mkdir cblib
cd cblib
rem cmake -G "Visual Studio 16 2019" -A "Win64" -DPHAM_SHARED_LIBS=OFF -DUSE_TESTS=OFF -DUSE_BINARIES=OFF -DCMAKE_INSTALL_PREFIX=%cd%/../../../PhamDeploy/VS2019/x64/ ../../../.
cmake ../../../.
cd ../../../


