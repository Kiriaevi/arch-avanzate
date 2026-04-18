# K-Means Project - Group 9

**Course**: Advanced Computer System Architectures  
**Academic Year**: 2025/2026  
**University of Calabria**

## Project Description

This project implements an **optimized version of the K-Means algorithm**, developed to make the most of **vector instructions** and **parallel computing**. Also, this library aims to be avaible as a wrapper for the Python language.

The goal was to create three variants of the program, each with a different level of optimization:

1. **SSE + float version**  
   Implementation based on `float` numbers and **SSE** instructions for vector processing.

2. **AVX + double version**  
   Implementation based on `double` numbers and **AVX** instructions to achieve greater vector width.

3. **AVX + OpenMP version**  
   More advanced version, combining **AVX** and **OpenMP** to leverage both vectorization and multithreaded parallelism.

## Objectives

- Optimize the calculation of distances between points and centroids
- Reduce execution time compared to a sequential version
- Leverage the SIMD instructions available on the CPU
- Integrate thread-level parallelism via OpenMP
- Compare the performance of the different implementations


## Project Structure

The repository is organized into three main folders, one for each version:

- `src/32/` → 32-bit implementation with SSE and `float` support
- `src/64/` → 64-bit implementation with AVX and `double` support
- `src/64omp/` → 64-bit implementation with AVX and OpenMP

There are also assembly files (`.nasm`) and supporting C files for handling the main operations.

## Technologies Used
### Languages
- **C**
- **NASM Assembly**
- **SSE**
- **AVX**
- **OpenMP**
- **Python** 
### Profiling softwares
- **Valgrind**, in particular: massif and callgrind
- **Perf**

## Requirements

To compile and run the project, you need:

- gcc compiler (tested with gcc 11.2.0)
- **NASM**
- **SSE** and/or **AVX** support on the CPU
- **OpenMP** for the parallel version
- Python 3

## Compilation and Execution

The compilation process depends on the version you choose.  
Each folder contains a `run.sh` script that can be used as a reference for compilation and execution.

# Results
### Small dataset  (N = 2000, D = 256)

| Config. | Fit (s) | Predict (s) |
|:-----------| :--- | :--- |
 | No optimizations| 0.031 | 10.800 |
| SSE        | 0.016 | 0.074 |
| AVX        | 0.016 | 0.075 |
| AVX+OPENMP | 0.039 | 0.034 |

### Big dataset (N = 40000, D = 2000)

| Config. | Fit (s) | Predict (s) |
|:-----------|:--------|:------------|
| No optimizations| 14.306  | 186.914            |
| SSE        | 0.793   | 6.213       |
| AVX        | 0.799   | 6.242       |
| AVX+OPENMP | 1.298   | 1.283       |
