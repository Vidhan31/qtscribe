## Code quality and formatting
- Omit step-by-step, obvious, and decorative banner comments. Document non-obvious logic and workarounds.
- Before committing changes, run `pre-commit run --all-files` to format and lint all C++ and QML files.

## Build configurations and presets
- Presets:
  - Debug (`build/`): `cmake --preset linux-qt6-debug` and `cmake --build --preset build-debug`
  - Release (`build-release/`): `cmake --preset linux-qt6-release` and `cmake --build --preset build-release`
  - RelWithDebInfo (`build-relwithdebinfo/`): `cmake --preset linux-qt6-relwithdebinfo` and `cmake --build --preset build-relwithdebinfo`
- Pass `--target <name>` (such as `qtscribe` or `keyinjectord`) to rebuild only the changed module.
- Keep `-DENABLE_VULKAN=OFF` (the default) for faster local builds.

## Testing
- Run test suites with `ctest --preset test-debug` (or the matching preset).
- Tests must run headless (`QT_QPA_PLATFORM=offscreen`, `QT_MEDIA_BACKEND=null`) and remain executable in container environments.

## Platform and environment
- Application is strictly Wayland only. No fallback to X11.
- Ubuntu 24.04 and GNOME 46 are the baseline.
