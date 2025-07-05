# Contrees
## Requirement
- An x86-64(AMD64) platform supporting SIMD and AES-NI
- C++ compiler supporting C++20
- Mimalloc v1.8.4(v2.1.4) or above

## Building
To build the main project:
```sh
make
```

## Export Dataset
```bash
make gen
```

This command will retrieve all files in the `ycsbc/workloads` directory, generate the corresponding datasets based on these configuration files, and store them in `ycsbc/export`.

## Usage
Run the main benchmark:
```sh
./main <dataset> <scheduler> <structure> [options]
```
- `<dataset>`: Path to a binary workload file
- `<scheduler>`: `inplace`, `seqcow` or `concow`
- `<structure>`: `betree`, `btree`, `aert`, `art`
- Options:
  - `-c <num_clients>`: Number of client threads (default: 1)
  - `-p <num_pipes>`: Number of pipes (default: 1)
  - `-w <num_workers>`: Number of concurrent workers (default: 1)

## Remark
The number of pipes does not include the last one, so it should be one less than the number reported in the paper.

## References
- [YCSB-C](https://github.com/brianfrankcooper/YCSB/wiki): Standardized cloud serving benchmark.
- [Mimalloc](https://github.com/microsoft/mimalloc): Used for efficient concurrent memory allocation.
