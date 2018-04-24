# LOGAN: LOssless Graph-based Analysis of NGS Datasets

Implementation of the Routed Sparse Graph approach described [here](https://www.biorxiv.org/content/early/2017/08/21/175976)

Creates a graph-based representation of short read, long read or pre-assembled sequences. Current implementation is primarily optimized for short reads, and partially also for pre-assembled sequences. Modest long-read datasets can also be used. 

Graph building is divided into two phases, indexing, which determines a suitable set of nodes, and building, which adds edges and routes to the chosen set of nodes. 

### Status
This is early code release, for evaluation purposes. 

Building the graph should be feasible on a 32GB RAM desktop for most Illumina datasets up to ~100Gbp (preprocessing with [Trimmomatic](http://www.usadellab.org/cms/?page=trimmomatic) for adapter removal and filtering at Q20 is recommended). Creation of pan-genome graphs from multiple gigabase-scale assembled references should also work, and has been tested with Human vs Chimp and 10 human genomes. 

Basic but inefficient example code for extracting sequences is included. Code for further analysing the resulting graph is under development. 

Reading/Writing of the graph in binary formats is now available. 

### Requirements
Any recent version of: gcc, make, gperftools, jdk & ant

### To Build 

* Change to `LOGAN-Graph` directory, and build Java part using `ant`
* Change to `LOGAN-Graph-Native` directory, build JNI headers using `./scripts/jHeaders.sh`, then build native code using `make`. You may need to modify JNI_INC in `makefile` to correctly locate the JNI headers (jni.h etc).

### To Test

* Change to LOGAN-Graph-Native/bin
* Run using `./LOGAN TestIndexAndRoute <indexingThreads> <routingThreads> <file1>...`
* Files can be either reads from Illumina, PacBio and Oxford Nanopore etc. in FASTQ format (.fq extension), or reference / pre-assembled sequences in FASTA format (.fa extension). Support for compressed files is planned.

### Command Line Options

General format is: `LOGAN <command> <args>`
  
* `LOGAN Index <indexingThreads> <graph> <files...>`: Performs the indexing phase of graph building for the provided FASTQ/A files and outputs the resulting nodes in `graph.nodes`.
* `LOGAN Route <routingThreads> <graph> <files...>`: Performs the routing phase of graph building for the provided FASTQ/A files, based on the existing `graph.nodes` file, and outputs the resulting edges/routes in `graph.edges` and `graph.routes`.
* `LOGAN IndexAndRoute <indexingThreads> <routingThreads> <graph> <files...>`: Performs the both phases of graph building for the provided FASTQ/A files and outputs the resulting graph elements in `graph.nodes`, `graph.edges` and `graph.routes`.
* `LOGAN TestIndex <indexingThreads> <files...>`: Performs the indexing phase of graph building for the provided FASTQ/A files, but does not save the result.
* `LOGAN TestRoute <routingThreads> <files...>`: Performs the routing phase of graph building for the provided FASTQ/A files, based on the existing `graph.nodes` file, but does not save the result.
* `LOGAN TestIndexAndRoute <indexingThreads> <routingThreads> <files...>`: Performs the both phases of graph building for the provided FASTQ/A files, but does not save the result.
* `LOGAN ReadGraph`: Loads the graph previously stored in `graph.nodes`, `graph.edges` and `graph.routes`. 
