#!bash
echo "Enter Mod name:"
read name

echo "Adding $name to src/CMakeLists.txt"
sed -i "5i\  mods/$name.hpp\n  mods/$name.cpp" src/dmc4_hook/CMakeLists.txt

echo "Copying modSample and updating mods/$name"
cp src/dmc4_hook/mods/modSample.hpp src/dmc4_hook/mods/$name.hpp
cp src/dmc4_hook/mods/modSample.cpp src/dmc4_hook/mods/$name.cpp
sed -i "/\#if 0/d;/\#endif/d;s/ModSample/${name^}/" src/dmc4_hook/mods/$name.hpp
sed -i "/\#if 0/d;/\#endif/d;s/ModSample/${name^}/;s/modSample/$name/" src/dmc4_hook/mods/$name.cpp

echo "Dont forget to add $name into Mods.cpp"