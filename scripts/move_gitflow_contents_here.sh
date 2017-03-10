#!/usr/bin/env bash

mkdir -p git
mkdir -p cmake
cp -r ../modules/gitflow/* ./git/
cp ../modules/utility/cmake/* ./cmake/
rm  ./git/LICENSE
rm ./git/README.md

