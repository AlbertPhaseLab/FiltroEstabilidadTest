# FiltroEstabilidadTest

A C++17 console application that tests the stability of JUCE low-pass biquad filter coefficients using impulse-response simulations in single and double precision.

## Requirements

- Windows
- Visual Studio 2026 with the MSVC C++ workload
- CMake 3.22 or newer
- Ninja
- Git
- Internet access on the first CMake configuration, because JUCE 7.0.5 is downloaded with `FetchContent`

## Build with CMake and Ninja

Open a PowerShell terminal in the project root, the directory containing `CMakeLists.txt`.

### Release build

```powershell
cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-ninja --parallel
```

### Debug build

```powershell
cmake -S . -B build-ninja-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-ninja-debug --parallel
```

CMake generates the JUCE headers, downloads JUCE 7.0.5 if necessary, and builds the application with the MSVC compiler.

## Run the application

After a Release build, run:

```powershell
.\build-ninja\FiltroEstabilidadTest_artefacts\Release\FiltroEstabilidadTest.exe
```

After a Debug build, run:

```powershell
.\build-ninja-debug\FiltroEstabilidadTest_artefacts\Debug\FiltroEstabilidadTest.exe
```

The application prints the stability results for the configured frequencies in single and double precision. It waits for **Enter** before closing.

## Reconfigure or perform a clean build

If the generator or CMake options change, use a new build directory or remove the old one before configuring again:

```powershell
Remove-Item -Recurse -Force build-ninja
cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-ninja --parallel
```

Do not commit generated build directories or downloaded JUCE sources.

## Open and build with Visual Studio

Visual Studio can open the folder containing `CMakeLists.txt` and use the CMake configuration. Ninja is the recommended generator for this project because it works with the MSVC toolchain in Visual Studio 2026.

Alternatively, if the installed CMake version exposes the `Visual Studio 18 2026` generator, create a separate Visual Studio build directory:

```powershell
cmake -S . -B build-vs -G "Visual Studio 18 2026" -A x64
cmake --build build-vs --config Release --parallel
```

Run the Visual Studio-generator build with:

```powershell
.\build-vs\FiltroEstabilidadTest_artefacts\Release\FiltroEstabilidadTest.exe
```

## Create a Git repository

Run these commands once from the project root:

```powershell
git init
git branch -M main
git add CMakeLists.txt Main.cpp README.md .gitignore
git commit -m "Initial commit"
```

Check the repository status with:

```powershell
git status
```
