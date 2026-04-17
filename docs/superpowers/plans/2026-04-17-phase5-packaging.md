# Phase 5: Packaging + CI — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Cross-platform packaging for `kit` (C++), `kit-rust`, `kit-server`, `kit-daemon` on Linux, macOS, Windows. Python via PyPI. GitHub Actions CI builds and publishes release artifacts on version tag push.

**Architecture:** `packaging/` holds all platform scripts. Each platform packages the same 4 binaries. GitHub Actions matrix builds all platforms, uploads to GitHub Release. Python pushed to PyPI separately.

**Tech Stack:** CMake, Cargo, NSIS (Windows installer), dpkg-deb (Debian), Homebrew, GitHub Actions, PyPI (twine)

**Prerequisite:** All phases 0–4 complete and building successfully on each platform.

---

## File Map

### Created
```
packaging/linux/install.sh
packaging/linux/debian/control
packaging/linux/debian/install
packaging/linux/arch/PKGBUILD
packaging/macos/install.sh
packaging/macos/Formula/kit-vcs.rb
packaging/windows/kit-vcs.nsi
packaging/windows/chocolatey/kit-vcs.nuspec
packaging/windows/chocolatey/tools/chocolateyInstall.ps1
packaging/ci/build-linux.yml
packaging/ci/build-macos.yml
packaging/ci/build-windows.yml
.github/workflows/release.yml
```

---

### Task 1: Linux install script + .deb package

**Files:**
- Create: `packaging/linux/install.sh`
- Create: `packaging/linux/debian/control`
- Create: `packaging/linux/debian/install`

- [ ] **Step 1: Write install.sh**

`packaging/linux/install.sh`:
```bash
#!/usr/bin/env bash
set -euo pipefail

VERSION="2.0.0"
INSTALL_DIR="/usr/local/bin"
REPO="https://github.com/kyuna0312/kit-vcs"

echo "Installing kit-vcs $VERSION..."

# Detect arch
ARCH=$(uname -m)
case $ARCH in
    x86_64)  ARCH_TAG="x86_64" ;;
    aarch64) ARCH_TAG="aarch64" ;;
    *)       echo "Unsupported arch: $ARCH"; exit 1 ;;
esac

RELEASE_URL="$REPO/releases/download/v$VERSION/kit-linux-$ARCH_TAG.tar.gz"
TMP=$(mktemp -d)

echo "Downloading from $RELEASE_URL..."
curl -fsSL "$RELEASE_URL" | tar -xz -C "$TMP"

for bin in kit kit-rust kit-server kit-daemon; do
    if [ -f "$TMP/$bin" ]; then
        sudo install -m 755 "$TMP/$bin" "$INSTALL_DIR/$bin"
        echo "Installed $bin -> $INSTALL_DIR/$bin"
    fi
done

rm -rf "$TMP"
echo "Done. Run 'kit --help' to get started."
```

- [ ] **Step 2: Make executable and test locally**

```bash
chmod +x packaging/linux/install.sh
# dry-run: check script syntax
bash -n packaging/linux/install.sh
echo "Script syntax OK"
```

- [ ] **Step 3: Create .deb control file**

`packaging/linux/debian/control`:
```
Package: kit-vcs
Version: 2.0.0
Section: vcs
Priority: optional
Architecture: amd64
Depends: libssl3
Maintainer: kyuna0312 <khatanzorigb@gmail.com>
Description: kit-vcs — Git-like sandbox version control system
 C++ and Rust implementations of a local VCS with server and web UI.
```

- [ ] **Step 4: Create .deb install manifest**

`packaging/linux/debian/install`:
```
build/kit-vcs          usr/local/bin/kit
rust/target/release/kit-rust    usr/local/bin/kit-rust
build/server/kit-server         usr/local/bin/kit-server
build/server/kit-daemon         usr/local/bin/kit-daemon
```

- [ ] **Step 5: Create build-deb script**

`packaging/linux/build-deb.sh`:
```bash
#!/usr/bin/env bash
set -euo pipefail
VERSION="2.0.0"
PKG="kit-vcs_${VERSION}_amd64"
mkdir -p "$PKG/DEBIAN" "$PKG/usr/local/bin"

cp packaging/linux/debian/control "$PKG/DEBIAN/control"

# Copy binaries (must be built first)
cp build/kit-vcs                  "$PKG/usr/local/bin/kit"
cp rust/target/release/kit-rust   "$PKG/usr/local/bin/kit-rust"
cp build/server/kit-server        "$PKG/usr/local/bin/kit-server"
cp build/server/kit-daemon        "$PKG/usr/local/bin/kit-daemon"

chmod 755 "$PKG/usr/local/bin/"*
dpkg-deb --build "$PKG"
echo "Built $PKG.deb"
```

- [ ] **Step 6: Commit**

```bash
git add packaging/linux/
git commit -m "feat(packaging): add Linux install script and .deb packaging"
```

---

### Task 2: Arch Linux PKGBUILD

**Files:**
- Create: `packaging/linux/arch/PKGBUILD`

- [ ] **Step 1: Write PKGBUILD**

`packaging/linux/arch/PKGBUILD`:
```bash
# Maintainer: kyuna0312 <khatanzorigb@gmail.com>
pkgname=kit-vcs
pkgver=2.0.0
pkgrel=1
pkgdesc="Git-like sandbox version control system"
arch=('x86_64' 'aarch64')
url="https://github.com/kyuna0312/kit-vcs"
license=('MIT')
depends=('openssl')
makedepends=('cmake' 'rust' 'ninja')
source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$srcdir/$pkgname-$pkgver"

    # Build C++ + server
    cmake -B build -S cpp -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    cmake -B build-server -S server -DCMAKE_BUILD_TYPE=Release
    cmake --build build-server --parallel

    # Build Rust
    cd rust && cargo build --release && cd ..
}

package() {
    cd "$srcdir/$pkgname-$pkgver"
    install -Dm755 build/kit-vcs                 "$pkgdir/usr/bin/kit"
    install -Dm755 rust/target/release/kit-rust  "$pkgdir/usr/bin/kit-rust"
    install -Dm755 build-server/kit-server       "$pkgdir/usr/bin/kit-server"
    install -Dm755 build-server/kit-daemon       "$pkgdir/usr/bin/kit-daemon"
    install -Dm644 LICENSE                       "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
```

- [ ] **Step 2: Commit**

```bash
git add packaging/linux/arch/
git commit -m "feat(packaging): add Arch Linux PKGBUILD for AUR"
```

---

### Task 3: macOS packaging

**Files:**
- Create: `packaging/macos/install.sh`
- Create: `packaging/macos/Formula/kit-vcs.rb`

- [ ] **Step 1: Write macOS install script**

`packaging/macos/install.sh`:
```bash
#!/usr/bin/env bash
set -euo pipefail

VERSION="2.0.0"
INSTALL_DIR="/usr/local/bin"
REPO="https://github.com/kyuna0312/kit-vcs"

ARCH=$(uname -m)
case $ARCH in
    x86_64)  ARCH_TAG="x86_64" ;;
    arm64)   ARCH_TAG="arm64" ;;
    *)       echo "Unsupported arch: $ARCH"; exit 1 ;;
esac

RELEASE_URL="$REPO/releases/download/v$VERSION/kit-macos-$ARCH_TAG.tar.gz"
TMP=$(mktemp -d)

echo "Downloading kit-vcs $VERSION for macOS $ARCH..."
curl -fsSL "$RELEASE_URL" | tar -xz -C "$TMP"

for bin in kit kit-rust kit-server kit-daemon; do
    [ -f "$TMP/$bin" ] && install -m 755 "$TMP/$bin" "$INSTALL_DIR/$bin" && echo "Installed $bin"
done

rm -rf "$TMP"
echo "Done."
```

- [ ] **Step 2: Write Homebrew formula**

`packaging/macos/Formula/kit-vcs.rb`:
```ruby
class KitVcs < Formula
  desc "Git-like sandbox version control system"
  homepage "https://github.com/kyuna0312/kit-vcs"
  version "2.0.0"
  license "MIT"

  on_macos do
    on_arm do
      url "https://github.com/kyuna0312/kit-vcs/releases/download/v#{version}/kit-macos-arm64.tar.gz"
      sha256 "PLACEHOLDER_ARM64_SHA256"
    end
    on_intel do
      url "https://github.com/kyuna0312/kit-vcs/releases/download/v#{version}/kit-macos-x86_64.tar.gz"
      sha256 "PLACEHOLDER_X86_64_SHA256"
    end
  end

  def install
    bin.install "kit"
    bin.install "kit-rust"
    bin.install "kit-server"
    bin.install "kit-daemon"
  end

  test do
    system "#{bin}/kit", "--version"
    system "#{bin}/kit-rust", "--version"
  end
end
```

- [ ] **Step 3: Commit**

```bash
git add packaging/macos/
git commit -m "feat(packaging): add macOS install script and Homebrew formula"
```

---

### Task 4: Windows packaging

**Files:**
- Create: `packaging/windows/kit-vcs.nsi`
- Create: `packaging/windows/chocolatey/kit-vcs.nuspec`
- Create: `packaging/windows/chocolatey/tools/chocolateyInstall.ps1`

- [ ] **Step 1: Write NSIS installer script**

`packaging/windows/kit-vcs.nsi`:
```nsis
!define APP_NAME "kit-vcs"
!define APP_VERSION "2.0.0"
!define INSTALL_DIR "$PROGRAMFILES64\kit-vcs"

Name "${APP_NAME} ${APP_VERSION}"
OutFile "kit-vcs-${APP_VERSION}-setup.exe"
InstallDir "${INSTALL_DIR}"
RequestExecutionLevel admin

Section "Install"
    SetOutPath "${INSTALL_DIR}"
    File "dist\kit.exe"
    File "dist\kit-rust.exe"
    File "dist\kit-server.exe"
    File "dist\kit-daemon.exe"

    ; Add to PATH
    EnVar::AddValue "PATH" "${INSTALL_DIR}"

    WriteUninstaller "${INSTALL_DIR}\uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\kit-vcs" \
        "DisplayName" "kit-vcs ${APP_VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\kit-vcs" \
        "UninstallString" "${INSTALL_DIR}\uninstall.exe"
SectionEnd

Section "Uninstall"
    Delete "${INSTALL_DIR}\kit.exe"
    Delete "${INSTALL_DIR}\kit-rust.exe"
    Delete "${INSTALL_DIR}\kit-server.exe"
    Delete "${INSTALL_DIR}\kit-daemon.exe"
    Delete "${INSTALL_DIR}\uninstall.exe"
    RMDir "${INSTALL_DIR}"
    EnVar::DeleteValue "PATH" "${INSTALL_DIR}"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\kit-vcs"
SectionEnd
```

- [ ] **Step 2: Write Chocolatey nuspec**

`packaging/windows/chocolatey/kit-vcs.nuspec`:
```xml
<?xml version="1.0" encoding="utf-8"?>
<package xmlns="http://schemas.microsoft.com/packaging/2015/06/nuspec.xsd">
  <metadata>
    <id>kit-vcs</id>
    <version>2.0.0</version>
    <title>kit-vcs</title>
    <authors>kyuna0312</authors>
    <projectUrl>https://github.com/kyuna0312/kit-vcs</projectUrl>
    <licenseUrl>https://github.com/kyuna0312/kit-vcs/blob/main/LICENSE</licenseUrl>
    <description>Git-like sandbox VCS — C++, Rust, Python implementations with server and web UI.</description>
    <tags>vcs git version-control</tags>
  </metadata>
  <files>
    <file src="tools\**" target="tools" />
  </files>
</package>
```

- [ ] **Step 3: Write Chocolatey install script**

`packaging/windows/chocolatey/tools/chocolateyInstall.ps1`:
```powershell
$ErrorActionPreference = 'Stop'
$version = '2.0.0'
$url = "https://github.com/kyuna0312/kit-vcs/releases/download/v$version/kit-windows-x86_64.zip"

$packageArgs = @{
    packageName   = 'kit-vcs'
    unzipLocation = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
    url           = $url
    checksum      = 'PLACEHOLDER_SHA256'
    checksumType  = 'sha256'
}

Install-ChocolateyZipPackage @packageArgs

$installDir = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
Install-ChocolateyPath $installDir 'Machine'
```

- [ ] **Step 4: Commit**

```bash
git add packaging/windows/
git commit -m "feat(packaging): add Windows NSIS installer and Chocolatey package"
```

---

### Task 5: GitHub Actions CI — release pipeline

**Files:**
- Create: `.github/workflows/release.yml`

- [ ] **Step 1: Create release workflow**

`.github/workflows/release.yml`:
```yaml
name: Release

on:
  push:
    tags: ['v*']

permissions:
  contents: write

jobs:
  build-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install dependencies
        run: sudo apt-get install -y libssl-dev cmake ninja-build

      - name: Install Rust
        uses: dtolnay/rust-toolchain@stable

      - name: Build C++ (kit + server)
        run: |
          cmake -B cpp/build -S cpp -DCMAKE_BUILD_TYPE=Release
          cmake --build cpp/build --parallel
          cmake -B server/build -S server -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_PREFIX_PATH=$PWD/cpp/build
          cmake --build server/build --parallel

      - name: Build Rust
        run: cd rust && cargo build --release

      - name: Package
        run: |
          mkdir -p dist
          cp cpp/build/kit-vcs           dist/kit
          cp rust/target/release/kit-rust dist/kit-rust
          cp server/build/kit-server      dist/kit-server
          cp server/build/kit-daemon      dist/kit-daemon
          tar -czf kit-linux-x86_64.tar.gz -C dist .

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: kit-linux-x86_64
          path: kit-linux-x86_64.tar.gz

  build-macos:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install dependencies
        run: brew install openssl cmake

      - name: Install Rust
        uses: dtolnay/rust-toolchain@stable

      - name: Build C++ (kit + server)
        run: |
          cmake -B cpp/build -S cpp -DCMAKE_BUILD_TYPE=Release \
            -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
          cmake --build cpp/build --parallel
          cmake -B server/build -S server -DCMAKE_BUILD_TYPE=Release \
            -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
          cmake --build server/build --parallel

      - name: Build Rust
        run: cd rust && cargo build --release

      - name: Package (universal if possible)
        run: |
          mkdir -p dist
          cp cpp/build/kit-vcs            dist/kit
          cp rust/target/release/kit-rust  dist/kit-rust
          cp server/build/kit-server       dist/kit-server
          cp server/build/kit-daemon       dist/kit-daemon
          ARCH=$(uname -m)
          tar -czf kit-macos-${ARCH}.tar.gz -C dist .

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: kit-macos
          path: kit-macos-*.tar.gz

  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install OpenSSL
        run: choco install openssl -y

      - name: Install Rust
        uses: dtolnay/rust-toolchain@stable

      - name: Build C++
        run: |
          cmake -B cpp/build -S cpp -DCMAKE_BUILD_TYPE=Release `
            -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"
          cmake --build cpp/build --config Release --parallel
          cmake -B server/build -S server -DCMAKE_BUILD_TYPE=Release `
            -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"
          cmake --build server/build --config Release --parallel

      - name: Build Rust
        run: cd rust && cargo build --release

      - name: Package
        run: |
          mkdir dist
          copy cpp\build\Release\kit-vcs.exe           dist\kit.exe
          copy rust\target\release\kit-rust.exe         dist\kit-rust.exe
          copy server\build\Release\kit-server.exe      dist\kit-server.exe
          copy server\build\Release\kit-daemon.exe      dist\kit-daemon.exe
          Compress-Archive -Path dist\* -DestinationPath kit-windows-x86_64.zip

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: kit-windows-x86_64
          path: kit-windows-x86_64.zip

  publish-python:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with: { python-version: '3.11' }
      - run: pip install build twine
      - run: cd python && python -m build
      - run: twine upload python/dist/* --skip-existing
        env:
          TWINE_USERNAME: __token__
          TWINE_PASSWORD: ${{ secrets.PYPI_TOKEN }}

  create-release:
    needs: [build-linux, build-macos, build-windows]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/download-artifact@v4
        with: { path: artifacts }

      - name: Create GitHub Release
        uses: softprops/action-gh-release@v2
        with:
          files: |
            artifacts/kit-linux-x86_64/kit-linux-x86_64.tar.gz
            artifacts/kit-macos/kit-macos-*.tar.gz
            artifacts/kit-windows-x86_64/kit-windows-x86_64.zip
          generate_release_notes: true
```

- [ ] **Step 2: Add PYPI_TOKEN to GitHub Secrets**

In GitHub repo → Settings → Secrets → Actions:
- Add `PYPI_TOKEN` with value from PyPI API token

- [ ] **Step 3: Test workflow on a pre-release tag**

```bash
git tag v2.0.0-rc1
git push origin v2.0.0-rc1
# Check Actions tab on GitHub
```

Expected: all 4 jobs run, artifacts uploaded to release.

- [ ] **Step 4: Commit workflow**

```bash
git add .github/workflows/release.yml
git commit -m "feat(ci): add GitHub Actions release pipeline for all platforms"
```

---

### Task 6: Build verification checklist

- [ ] **Linux build passes**

```bash
# On Linux:
cmake -B cpp/build -S cpp && cmake --build cpp/build
cd rust && cargo build --release
cpp/build/kit-vcs --version
rust/target/release/kit-rust --version
```

- [ ] **macOS build passes**

```bash
# On macOS:
cmake -B cpp/build -S cpp -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
cmake --build cpp/build
cpp/build/kit-vcs --version
```

- [ ] **Windows build passes (CI)**

Push to `v*` tag and verify GitHub Actions Windows job succeeds.

- [ ] **Python package installable**

```bash
cd python && pip install -e . && kit-py --help
```

- [ ] **Final commit**

```bash
git add packaging/ .github/
git commit -m "feat(packaging): complete cross-platform packaging and CI pipeline"
```
