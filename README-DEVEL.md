# Developing admiraldog

## Requirements

- CMake 3.28+, Ninja, Clang (with C++20 support)
- For the ISA tests: Clang with RISC-V, `ld.lld`, `llvm-objcopy`.
  - On macOS you need to install llvm and lld e.g. via brew: `brew install llvm lld`. The default Apple-provided Clang does not provide the RISC-V backend and without the two components the ISA tests are skipped with a warning.
- The `riscv-tests` are imported via submodule: `git submodule update --init`

doctest is fetched by CMake at configure time.

## Build and test

Everything at once (configure, build, test):

```sh
cmake --workflow --preset dev  # the development preset
cmake --workflow --preset asan # like dev, but it adds the Clang sanitizers to find memory access errors and code relying on undefined behaviour.
```

See the table below for the presets.

Step by step:

```sh
cmake --preset dev           # configure into build/dev (once, and after CMake changes)
cmake --build --preset dev   # build; also rebuilds the RISC-V test programs
ctest --preset dev           # run all tests
```

Remember to run `cmake --build` before `ctest`.
If `ctest` says "No tests were found", you should run the preset at least once and build.

## Presets

| Preset | What |
|---|---|
| `dev`  | Clang, Debug. Daily work. |
| `asan` | `dev` + AddressSanitizer and UndefinedBehaviorSanitizer. |

Each preset has its own build tree in `build/<preset>/`.
`compile_commands.json` is written there for clangd.

## Choosing tests

```sh
ctest --preset dev -L unit                  # everything headless (unit + ISA)
ctest --preset dev -L isa                   # only the riscv-tests programs
ctest --preset dev -R rv32ui-add            # one program
ctest --preset dev -R 'rv32ui-(lui|auipc)'  # a regex
ctest --preset dev -R 'cpu:'                # the Cpu unit tests
ctest --preset dev -N                       # list tests without running them
```

To see the output of a single test directly:

```sh
build/dev/tests/unit_tests -tc='cpu: JALR*'           # doctest filters
build/dev/tests/isa/isa_runner build/dev/tests/isa/rv32ui/add.bin
```

## Cleaning 🧹

```sh
cmake --build --preset dev --target clean   # delete build outputs, keep the configuration
cmake --build --preset dev --clean-first    # clean, then build
cmake --preset dev --fresh                  # configure again from scratch, ignoring the cache
rm -rf build/dev                            # delete the whole build tree of one preset
```

Use `clean` to force a full rebuild. Use `--fresh` after changing the compiler,
the toolchain or a cache option. When in doubt, delete `build/<preset>/`: the next
configure fetches doctest again.

## Troubleshooting

See [docs/troubleshooting/](docs/troubleshooting/) for platform specific issues.
