mkdir $PWD/gen -p
protoc -I=$PWD/src --cpp_out=$PWD/gen $PWD/src/serverStructs.proto 
