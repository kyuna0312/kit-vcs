# 🦊 kit-vcs

**`kit`** is a minimal, educational version control system written in modern C++. Inspired by Git, it is designed to help developers learn and explore the internals of version control systems: content-addressed object storage, trees, refs, and merges — built from scratch.

![kit-vcs logo](assets/logo_pixel.png)
![kit banner](https://img.shields.io/badge/version-2.0.0-blue?style=flat-square)

---

## ✨ Features

All core commands are implemented:

| Command | Description |
|---------|-------------|
| `kit init` | Initialize a new `.kit/` repository |
| `kit add <file>` | Stage file(s) into the index |
| `kit commit -m <msg>` | Commit staged files |
| `kit status` | Show staged / modified / untracked files |
| `kit log` | Show commit history |
| `kit diff` | Line-level diff between working directory and commits |
| `kit branch` | Create, list, and delete branches |
| `kit checkout <branch>` | Switch branches or commits |
| `kit merge <branch>` | Merge a branch into the current branch |
| `kit reset <commit>` | Reset to a specific commit |
| `kit stash` | Temporarily save and restore changes |

---

## 🛠 Getting Started

### Requirements

- CMake ≥ 3.14
- A C++17 compiler
- OpenSSL (for SHA-1 hashing)

### Build

```bash
git clone https://github.com/kyuna0312/kit-vcs.git
cd kit-vcs
mkdir cpp/build && cd cpp/build
cmake ..
make
```

Binary output: `cpp/build/kit-vcs`

> On macOS the `CMakeLists.txt` uses Homebrew's OpenSSL path. On Linux, override it if needed:
> `cmake .. -DOPENSSL_ROOT_DIR=/usr`

### Run Tests

```bash
cd cpp/build
ctest                # run all tests
./test_unit          # unit tests, verbose
./test_integration   # integration tests
```

Tests use Google Test, fetched automatically via CMake FetchContent.

---

## 🗂 Usage Example

```bash
kit init
kit add main.cpp
kit commit -m "Initial commit"
kit branch feature
kit checkout feature
# ...edit files...
kit add main.cpp
kit commit -m "Add feature"
kit checkout master
kit merge feature
kit log
```

---

## 📦 Object Model

`kit` mirrors Git's three-layer object model — **commit → tree → blobs** — stored as flat, SHA-1-addressed files:

```
.kit/
├── HEAD          # "ref: refs/heads/<branch>" or a bare commit hash (detached)
├── index         # staged file list (path → blob hash)
├── objects/      # blobs, trees, commits — flat files named by SHA-1
├── refs/
│   └── heads/    # one file per branch, containing a commit hash
└── stash/        # stashed snapshots
```

The on-disk wire format is fully specified in [`spec/FORMAT.md`](spec/FORMAT.md), with byte-exact fixtures under [`spec/fixtures/`](spec/fixtures/) so alternative implementations (e.g. Rust) can verify interoperability.

---

## 🏗 Code Layout

```
cpp/src/
├── main.cpp              # entry point → kit::cli::run()
├── cli/
│   ├── cli.cpp           # command dispatch
│   └── commands/         # cmd_add, cmd_commit, cmd_merge, ... one pair per command
├── core/
│   ├── repository.cpp    # opens/inits .kit/, reads/writes objects
│   ├── refs.cpp          # HEAD and branch pointers
│   ├── index.cpp         # staging area
│   └── objects/          # Blob, Tree, Commit
└── utils/                # SHA-1 (OpenSSL), diff, filesystem, logging, Result<T>
```

Errors are handled explicitly with a `Result<T>` type — no exceptions on the happy path.

---

## 👀 Goals

- Learn Git internals by rebuilding them.
- Keep the implementation small enough to read in one sitting.
- Play with low-level file I/O, hashing, and commit DAGs.
- Serve as a reference implementation: the format spec enables ports to other languages.

---

## 📜 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

## 🙌 Contributing

Contributions are welcome! Feel free to open issues or submit pull requests to improve `kit-vcs`.
