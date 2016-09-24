mkdir $PWD/gen -p
mkdir $PWD/bin -p
mkdir $PWD/bin/backups -p
mkdir $PWD/bin/users -p
protoc -I=$PWD/src --cpp_out=$PWD/gen $PWD/src/serverStructs.proto 
