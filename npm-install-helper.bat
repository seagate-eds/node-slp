git submodule update --init --recursive --remote
rem For manual building only: cd src/openslp/openslp/win32 && build-vs2017.bat
node-gyp rebuild
