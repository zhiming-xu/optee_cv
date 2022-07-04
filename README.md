### Compiling
Since the libm is directly placed under the project directory and specified in the TA linker script, after moved to `optee_examples` directory, the program
can be compiled without external configuration changes.

### Running
After compiling, run the `optee_example_cv` program under the folder that has the test images. The program accepts one argument that specifies how many images to use.
The test files should be named `Cars#_crop_sec` where the # denotes the number of that test image. The [img](./imgs) folder contains the ones used in the benchmarking. 
After mounting and entering this folder to QEMU, execute `run.sh` in the normal console for the benchmark. For each run, the throught will be printed out in the normal
world console.
```
# optee_example_cv 20
optee_example_cv 20
throughtput: 0.96 img/s on 20 samples
```

### Known problems
Won't work on images of some 300KB or larger, or those become larger than this size after processing.