mkdir gen -p
protoc -I=./src --cpp_out=./gen ./src/struct_serialization.proto 
