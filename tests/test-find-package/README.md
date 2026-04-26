# test-find-package

Verifies that `find_package(dsfw)` works after installation.

## Usage

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<dsfw-install-prefix>
cmake --build build
```

Build success means `dsfw::core` and `dsfw::ui-core` are correctly exported and consumable.
