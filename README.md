# clingo modulo simplex

A simplistic simplex solver for checking satisfiability of a set of equations.

## Installation

To compile the package, [cmake], [gmp], [clingo], and a C++ compiler supporting C++17 have to be installed.
All these requirements can be installed with [anaconda].

```bash
conda create -n simplex -c potassco/label/dev cmake ninja gmp clingo gxx_linux-64
conda activate simplex
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

[cmake]: https://cmake.org
[gmp]: https://gmplib.org
[clingo]: https://github.com/potassco/clingo
[anaconda]: https://anaconda.org

## Input format

The system supports `&sum` constraints over rationals with relations among `<=`, `>=`, and `=`.
The elements of the sum constraints must have form `x`, `-x`, `n*x`, `-n*x`, or `-n/d*x`
where `x` is a function symbol or tuple, and `n` and `d` are numbers.
A number has to be either a non negative integer or decimal number in quotes.


For example, the following program is accepted:
```
{ x }.
&sum { x; -y; 2*x; -3*y; 2/3*x; -3/4*y, "0.75"*z } >= 100 :- x.
```

Furthermore, `&dom` constraints are supported, which are shortcuts for `&sum` constraints.
The program
```
{ x }.
&dom { 1..2 } = x.
```
is equivalent to
```
{ x }.
&sum { x } >= 1.
&sum { x } <= 2.
```

When option `--strict` is passed to the solver, then also strict constraints are supperted:
```
{ x }.
&sum { x } > 1.
&sum { x } < 2.
```
The assignment will then contain an epsilon compontent for each variable.
For example, with the above program, `x>=1+e` will appear in the output.
This feature could also be used to support constraints in rule body and the `!=` relation;
neither is implemented at the moment.

## Profiling

Profiling with the [gperftools] can be enabled via cmake.

```bash
conda create -n profile -c conda-forge -c potassco/label/dev cmake ninja gmp clingo gxx_linux-64 gperftools
conda activate profile
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCLINGOLP_PROFILE=ON
cmake --build build
```

The resulting binary can then be profiled using the following calls:

```bash
CPUPROFILE_FREQUENCY=1000 ./build/clingo-lpx examples/encoding-lp.lp examples/tai4_4_1.lp --stats -c n=132 -q 0
google-pprof --gv ./build/clingo-lpx profile.out
```

[gperftools]: https://gperftools.github.io/gperftools/cpuprofile.html

## Literature

- "Integrating Simplex with `DPLL(T)`" by Bruno Dutertre and Leonardo de Moura
