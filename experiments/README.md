# Experiments, Tests & Temporary Artifacts Archive

This directory houses tests, test executables, generated build files, and experimental demo binaries to keep the main compiler codebase clean and focused.

## Directory Structure

- **`tests/`**: Language test suites (`test_main.cand`, `test_math.cand`) and test binaries (`test_main.exe`, `test_runtime.exe`, `test_runtime.obj`).
- **`binaries/`**: Compiled standalone executables from demos and experimental runs (e.g. calculators, GUI apps, ecosystem demos, tensor demo, tiny GPT).
- **`temp/`**: Intermediate compiler codegen files (`*.exe_gen.c`, `*.obj`), temporary run scripts, and model weight files (`weights.candmodel`).
- **`backups/`**: Legacy codebase archives (`c_and_backup.zip`).
