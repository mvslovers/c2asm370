# c2asm370 Test Suite

## Structure

```
tests/
  smoke/             Compilation smoke tests (must compile without error)
  regression/        Bug-specific regression tests (added with each fix)
  expected/          Reference .s output for regression comparison
    smoke/           (generated from Linux reference build)
    regression/
  run_tests.sh       Test runner script
```

## Running Tests

```bash
make                          # build the compiler first
./tests/run_tests.sh          # run all tests
./tests/run_tests.sh ./c2asm370   # specify compiler path
```

## Test Phases

Each .c file goes through:

1. **Compilation** - must succeed without errors
2. **Sanity check** - output must contain PDPPRLG/PDPEPIL macros
3. **Regression check** - if a matching expected/*.s file exists, compare output

## Adding Tests

- **Smoke test:** add a .c file to `tests/smoke/`
- **Regression test:** add a .c file to `tests/regression/`
- **Expected output:** generate on Linux reference build, place in `tests/expected/`
