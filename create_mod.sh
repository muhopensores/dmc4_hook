#!bash
echo "Enter Mod name:"
read name

echo "Adding $name to src/CMakeLists.txt"
sed -i "5i\  mods/$name.hpp\n  mods/$name.cpp" src/CMakeLists.txt

echo "Copying modSample and updating mods/$name"
cp src/mods/ModSample.hpp src/mods/$name.hpp
cp src/mods/ModSample.cpp src/mods/$name.cpp
sed -i "/\#if 0/d;/\#endif/d;s/ModSample/${name^}/" src/mods/$name.hpp
sed -i "/\#if 0/d;/\#endif/d;s/ModSample/${name^}/;s/modSample/$name/" src/mods/$name.cpp

echo "Dont forget to add $name into Mods.cpp"