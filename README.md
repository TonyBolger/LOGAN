# LOGAN: LOssless Graph-based Analysis of NGS Datasets

Implementation of the Routed Sparse Graph approached described [here](https://www.biorxiv.org/content/early/2017/08/21/175976)

Creates a graph-based representation of short read, long read or pre-assembled sequences. Current implementation is primarily optimized for short reads, and partially also for pre-assembled sequences. Modest long-read datasets can also be used. 

### Status
This is early code release, for evaluation purposes. Building the graph should be feasible on a 32GB RAM desktop for most Illumina datasets up to ~100Gbp (preprocessing with [Trimmomatic](http://www.usadellab.org/cms/?page=trimmomatic) for adapter removal and filtering at Q20 is recommended). Creation of pan-genome graphs from multiple gigabase-scale assembled references should also work, and has been tested with 10 human genomes. 

Basic but inefficient example code for extracting sequences is included. Code for further analysing the resulting graph is under development. 


### Requirements
Any recent version of: gcc, make, jdk & ant

### To Build 

* Change to `LOGAN-Graph` directory, and build Java part using `ant`
* Change to `LOGAN-Graph-Native` directory, build JNI headers using `./scripts/jHeaders.sh`, then build native code using `make`

### To Test

* Change to LOGAN-Graph-Native/bin
* Run using `./LOGAN <indexingThreads> <routingThreads> <file1>...`
* Files can be either reads from Illumina, PacBio and Oxford Nanopore etc. in FASTQ format (.fq extension), or reference / pre-assembled sequences in FASTA format (.fa extension). Support for compressed files is planned.
