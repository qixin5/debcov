#!/bin/bash

if [ ! -d ./build/java ]; then
    mkdir -p ./build/java
fi

javac -cp :./lib/java/* -d build/java src/java/main/*.java
