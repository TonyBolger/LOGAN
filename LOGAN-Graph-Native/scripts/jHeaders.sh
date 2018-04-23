#!/bin/bash

javah -classpath ../LOGAN-Graph/bin:../LOGAN-Graph/dist/build -d jniHeaders -jni logan.graph.Graph logan.graph.GraphReader logan.graph.GraphWriter 
