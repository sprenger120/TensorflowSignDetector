Sign Detection for a uni project.   
To build you have to download tensorflow 1.9 and follow the instructions of https://github.com/sprenger120/SignDetecc but when building with bazel you have to use this command:  
```
bazel build --config=monolithic tensorflow:libtensorflow_all.so
```
Additionally you have to use the local installation variant for protobuf and eigen. 
