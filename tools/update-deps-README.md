Update dependencies (developer helper)

This folder contains a helper PowerShell script to bootstrap vcpkg and install the core runtime dependencies used by SportsRace. It does NOT automatically retarget your Visual Studio projects; it only installs libraries using vcpkg so you can use them locally or in CI.

Usage (Windows):
1. Open PowerShell as a normal user (not required to be admin):
   PowerShell -ExecutionPolicy Bypass -File .\tools\update-deps.ps1

2. To bootstrap vcpkg into tools\vcpkg (default) and install packages used by the repo (x64-windows triplet):
   PowerShell -ExecutionPolicy Bypass -File .\tools\update-deps.ps1 -BootstrapVcpkg

3. After install, either:
   - Run: tools\vcpkg\vcpkg.exe integrate install (one-time global MSBuild integration), or
   - Manually add include/lib paths in project properties to point into tools\vcpkg\installed\x64-windows\include and ...\lib or ...\bin.

Packages installed by default:
- sdl2
- sdl2-image
- sdl2-ttf
- sdl2-mixer
- spdlog

Notes:
- This script assumes git is available and can clone from GitHub.
- CI: prefer adding a GitHub Actions step that bootstraps vcpkg and runs `vcpkg install` before building.
- If you prefer not to use vcpkg, you can continue to build against the vendored external/ directory already in the repo.

If you want, I can also:
- Retarget Visual Studio projects to use vcpkg's include/lib paths automatically (small project edits), or
- Add a GitHub Actions workflow to run this script and build tests on the runner.
