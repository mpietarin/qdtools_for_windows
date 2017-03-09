#!/usr/bin/env bash

cd ../../.. && mkdir -p qdtools_msvc && cd qdtools_msvc && cmake -G "Visual Studio 14 2015 Win64" ../qdtools_for_windows

read -p "Press [Enter] key to continue..."
