# craftworld_cpp

A modified C++ implementation of the Craftworld environment from [Modular Multitask Reinforcement Learning with Policy Sketches](https://arxiv.org/pdf/1611.01796.pdf).
Andreas, J., Klein, D. &amp; Levine, S.. (2017). Modular Multitask Reinforcement Learning with Policy Sketches. *Proceedings of the 34th International Conference on Machine Learning*, in *Proceedings of Machine Learning Research*.

## Include to Your Project: CMake

### FetchContent
```shell
include(FetchContent)
# ...
FetchContent_Declare(craftworld
    GIT_REPOSITORY https://github.com/tuero/craftworld_cpp_v2.git
    GIT_TAG master
)

# make available
FetchContent_MakeAvailable(craftworld)
link_libraries(craftworld)
```

### Git Submodules
```shell
# assumes project is cloned into external/craftworld_cpp
add_subdirectory(external/craftworld_cpp)
link_libraries(craftworld)
```

## Installing Python Bindings
```shell
git clone https://github.com/tuero/craftworld_cpp_v2.git
pip install ./craftworld_cpp_v2
```

If you get a `GLIBCXX_3.4.XX not found` error at runtime, 
then you most likely have an older `libstdc++` in your virtual environment `lib/` 
which is taking presidence over your system version.
Either correct your `$PATH`, or update your virtual environment `libstdc++`.

For example, if using anaconda
```shell
conda install conda-forge::libstdcxx-ng
```


## Generate Levels
The levelset generator will generate a curriculum of levels to gather the gem ring:
make a bronze pick, make an iron pick, and collect the gem ring.
```shell
cd scripts
python generate_levelset.py --export_path=EXPORT_PATH --map_size=14 --num_train=50000 --num_test=1000 --num_grass=2
```
