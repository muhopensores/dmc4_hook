#!/usr/bin/env bash
echo "Enter Mod name:"
read name
Name="$(echo "$name" | sed 's/^./\U&/')"

echo "Adding $Name to src/CMakeLists.txt"
sed -i "5i\  mods/$Name.hpp" src/CMakeLists.txt
sed -i "6i\  mods/$Name.cpp" src/CMakeLists.txt

echo "Copying modSample and updating mods/$Name"
cp src/mods/ModSample.hpp "src/mods/$Name.hpp"
cp src/mods/ModSample.cpp "src/mods/$Name.cpp"
sed -i "/\#if 0/d;/\#endif/d;s/ModSample/${Name}/" "src/mods/$Name.hpp"
sed -i "/\#if 0/d;/\#endif/d;s/ModSample/${Name}/;s/modSample/$name/" "src/mods/$Name.cpp"

echo "Adding mods/$Name.hpp to src/ModsHeaders.hpp"
if grep -q "mods/${Name}\.hpp" src/ModsHeaders.hpp; then
    echo "  include already present, skipping"
else
    sed -i '0,/^#ifndef PUBLIC_BUILD/s|^#ifndef PUBLIC_BUILD|#include "mods/'"$Name"'.hpp"\n&|' src/ModsHeaders.hpp
fi

echo "Dont forget to add ADD_MOD(${Name}); into Mods.cpp Mods::Mods() - order matters"
echo "Private/dev mods: move the new include inside the #ifndef PUBLIC_BUILD guard in ModsHeaders.hpp"
