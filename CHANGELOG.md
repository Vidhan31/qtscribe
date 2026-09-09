# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed
- **Breaking rename:** application renamed from QTranscribe (`qtranscribe`) to QtScribe (`qtscribe`).
  New binary `qtscribe`, app ID `io.github.qtscribe`, D-Bus service `io.github.qtscribe.SpeechService`.
  No migration: settings, keychain entries, and models stored under the old `qtranscribe` names are orphaned;
  reconfigure API keys and re-download models after upgrading.

## [1.0.0] - 2026-09-10

### Added
- Initial public release of QtScribe.
- Pre-built distribution packages for Debian/Ubuntu (`.deb`), Fedora (`.rpm`) and Arch Linux (`.pkg.tar.zst`).

[Unreleased]: https://github.com/Vidhan31/qtscribe/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/Vidhan31/qtscribe/releases/tag/v1.0.0
