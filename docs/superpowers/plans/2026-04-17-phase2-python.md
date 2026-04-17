# Phase 2: Python Implementation — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an educational Python implementation of kit-vcs — independent format, zero external dependencies, heavily commented for learning.

**Architecture:** `python/` package installable via pip. `core/` for object model, `cli/` for command dispatch via argparse. Own `.kit/` format (same structure, slightly different wire format to highlight design choices).

**Tech Stack:** Python 3.10+, stdlib only (argparse, hashlib, pathlib, json)

**Note:** Python implementation is independent — does NOT share `.kit/` repos with C++ or Rust.

---

## File Map

### Created
```
python/pyproject.toml
python/README.md
python/kit/__init__.py
python/__main__.py          ← python -m kit entry point
python/kit/core/__init__.py
python/kit/core/objects.py  ← Blob, Tree, Commit in one file
python/kit/core/index.py
python/kit/core/refs.py
python/kit/core/repository.py
python/kit/cli/__init__.py
python/kit/cli/commands/__init__.py
python/kit/cli/commands/cmd_init.py
python/kit/cli/commands/cmd_add.py
python/kit/cli/commands/cmd_commit.py
python/kit/cli/commands/cmd_status.py
python/kit/cli/commands/cmd_log.py
python/kit/cli/commands/cmd_branch.py
python/kit/cli/commands/cmd_checkout.py
python/kit/cli/commands/cmd_diff.py
python/kit/cli/commands/cmd_merge.py
python/kit/cli/commands/cmd_reset.py
python/kit/cli/commands/cmd_stash.py
python/kit/cli/dispatch.py
python/tests/__init__.py
python/tests/test_objects.py
python/tests/test_repository.py
python/tests/test_commands.py
```

---

### Task 1: Package setup

**Files:**
- Create: `python/pyproject.toml`
- Create: `python/kit/__init__.py`
- Create: `python/__main__.py`

- [ ] **Step 1: Create pyproject.toml**

```toml
[build-system]
requires = ["setuptools>=68"]
build-backend = "setuptools.backends.legacy:build"

[project]
name = "kit-vcs-py"
version = "2.0.0"
description = "kit-vcs — educational Python implementation"
requires-python = ">=3.10"
dependencies = []

[project.scripts]
kit-py = "kit.cli.dispatch:main"

[tool.setuptools.packages.find]
where = ["."]
```

- [ ] **Step 2: Create kit/__init__.py (empty)**

```python
# kit-vcs Python implementation
# Educational version control system — learn by reading this code.
```

- [ ] **Step 3: Create __main__.py**

```python
from kit.cli.dispatch import main
main()
```

- [ ] **Step 4: Install in editable mode**

```bash
cd python
pip install -e .
kit-py --help
```

Expected: usage message with no commands yet.

- [ ] **Step 5: Commit**

```bash
git add python/
git commit -m "feat(python): init package structure"
```

---

### Task 2: Core objects

**Files:**
- Create: `python/kit/core/objects.py`

- [ ] **Step 1: Write failing tests**

`python/tests/test_objects.py`:

```python
import pytest
from kit.core.objects import Blob, Tree, Commit

class TestBlob:
    def test_serialize_format(self):
        blob = Blob(content=b"hello\n")
        raw = blob.serialize()
        assert raw == b"blob 6\nhello\n"

    def test_hash_is_40_hex(self):
        blob = Blob(content=b"hello\n")
        h = blob.hash()
        assert len(h) == 40
        assert all(c in "0123456789abcdef" for c in h)

    def test_deserialize_roundtrip(self):
        blob = Blob(content=b"hello world\n")
        recovered = Blob.deserialize(blob.serialize())
        assert recovered.content == blob.content

class TestTree:
    def test_serialize_sorted(self):
        tree = Tree(entries=[
            {"mode": "100644", "name": "z.txt", "hash": "b" * 40},
            {"mode": "100644", "name": "a.txt", "hash": "a" * 40},
        ])
        raw = tree.serialize().decode()
        # a.txt should come before z.txt
        assert raw.index("a.txt") < raw.index("z.txt")

class TestCommit:
    def test_root_commit_no_parent(self):
        c = Commit(tree="a"*40, parent=None, author="Alice",
                   timestamp=1713300000, message="Initial commit")
        raw = c.serialize().decode()
        assert "parent" not in raw
        assert "tree " + "a"*40 in raw

    def test_roundtrip(self):
        c = Commit(tree="a"*40, parent="b"*40, author="Bob",
                   timestamp=1713300060, message="Second commit")
        c2 = Commit.deserialize(c.serialize())
        assert c2.author == "Bob"
        assert c2.parent == "b"*40
```

- [ ] **Step 2: Run — expect fail**

```bash
cd python
python -m pytest tests/test_objects.py -v
```

Expected: ImportError — module not found.

- [ ] **Step 3: Implement Blob, Tree, Commit**

`python/kit/core/objects.py`:

```python
"""
kit-vcs object model — Blob, Tree, Commit.

Each object is content-addressed: its SHA1 hash is its identity.
This is the same principle Git uses, though our wire format differs slightly.
"""
import hashlib
from dataclasses import dataclass, field
from typing import Optional


def _sha1(data: bytes) -> str:
    """Return lowercase hex SHA1 of data."""
    return hashlib.sha1(data).hexdigest()


@dataclass
class Blob:
    """A blob stores raw file content. Nothing more."""
    content: bytes

    def serialize(self) -> bytes:
        # Header tells us type and size — makes corruption detectable.
        header = f"blob {len(self.content)}\n".encode()
        return header + self.content

    def hash(self) -> str:
        return _sha1(self.serialize())

    @classmethod
    def deserialize(cls, raw: bytes) -> "Blob":
        nl = raw.index(b"\n")
        return cls(content=raw[nl + 1:])

    @classmethod
    def from_file(cls, path: str) -> "Blob":
        with open(path, "rb") as f:
            return cls(content=f.read())


@dataclass
class TreeEntry:
    mode: str
    name: str
    hash: str


@dataclass
class Tree:
    """A tree maps filenames to blob hashes — a directory snapshot."""
    entries: list[TreeEntry] = field(default_factory=list)

    def __init__(self, entries: list[dict] | list[TreeEntry]):
        self.entries = [
            e if isinstance(e, TreeEntry) else TreeEntry(**e)
            for e in entries
        ]

    def serialize(self) -> bytes:
        # Sort for determinism: same files always produce same tree hash.
        sorted_entries = sorted(self.entries, key=lambda e: e.name)
        body = ""
        for e in sorted_entries:
            body += f"{e.mode} {e.name}\x00{e.hash}\n"
        header = f"tree {len(sorted_entries)}\n"
        return (header + body).encode()

    def hash(self) -> str:
        return _sha1(self.serialize())

    @classmethod
    def deserialize(cls, raw: bytes) -> "Tree":
        text = raw.decode()
        lines = text.split("\n")
        # skip header line "tree N"
        entries = []
        for line in lines[1:]:
            if not line:
                continue
            null_pos = line.index("\x00")
            mode_name = line[:null_pos]
            sha = line[null_pos + 1:]
            space = mode_name.index(" ")
            entries.append(TreeEntry(
                mode=mode_name[:space],
                name=mode_name[space + 1:],
                hash=sha,
            ))
        return cls(entries=entries)


@dataclass
class Commit:
    """A commit is a snapshot + metadata. Parent links form the history graph."""
    tree: str
    parent: Optional[str]
    author: str
    timestamp: int
    message: str

    def serialize(self) -> bytes:
        body = f"tree {self.tree}\n"
        if self.parent:
            body += f"parent {self.parent}\n"
        body += f"author {self.author}\n"
        body += f"timestamp {self.timestamp}\n"
        body += f"\n{self.message}\n"
        header = f"commit {len(body)}\n"
        return (header + body).encode()

    def hash(self) -> str:
        return _sha1(self.serialize())

    @classmethod
    def deserialize(cls, raw: bytes) -> "Commit":
        text = raw.decode()
        lines = text.split("\n")
        # skip "commit <size>" header
        i = 1
        tree = parent = author = ""
        timestamp = 0
        while i < len(lines) and lines[i]:
            line = lines[i]
            if line.startswith("tree "):      tree = line[5:]
            elif line.startswith("parent "):  parent = line[7:]
            elif line.startswith("author "):  author = line[7:]
            elif line.startswith("timestamp "): timestamp = int(line[10:])
            i += 1
        # blank line then message
        i += 1
        message = "\n".join(lines[i:]).rstrip("\n")
        return cls(tree=tree, parent=parent or None, author=author,
                   timestamp=timestamp, message=message)
```

- [ ] **Step 4: Run — expect pass**

```bash
python -m pytest tests/test_objects.py -v
```

Expected: all PASS.

- [ ] **Step 5: Commit**

```bash
git add python/kit/core/objects.py python/tests/test_objects.py
git commit -m "feat(python): implement Blob, Tree, Commit objects"
```

---

### Task 3: Repository, Refs, Index

**Files:**
- Create: `python/kit/core/refs.py`
- Create: `python/kit/core/index.py`
- Create: `python/kit/core/repository.py`

- [ ] **Step 1: Write failing repository test**

`python/tests/test_repository.py`:

```python
import pytest
import tempfile, os
from pathlib import Path
from kit.core.repository import Repository

def test_init_creates_kit_dir():
    with tempfile.TemporaryDirectory() as tmp:
        repo = Repository.init(tmp)
        assert (Path(tmp) / ".kit" / "HEAD").exists()
        assert (Path(tmp) / ".kit" / "objects").is_dir()
        assert (Path(tmp) / ".kit" / "refs" / "heads").is_dir()

def test_open_nonexistent_raises():
    with tempfile.TemporaryDirectory() as tmp:
        with pytest.raises(Exception, match="not a kit repository"):
            Repository.open(tmp)

def test_write_and_read_object():
    with tempfile.TemporaryDirectory() as tmp:
        repo = Repository.init(tmp)
        repo.write_object("abc123", b"data")
        assert repo.read_object("abc123") == b"data"
```

- [ ] **Step 2: Run — expect fail**

```bash
python -m pytest tests/test_repository.py -v
```

- [ ] **Step 3: Implement Refs**

`python/kit/core/refs.py`:

```python
from pathlib import Path
from typing import Optional


class Refs:
    """Manages HEAD and branch ref files under .kit/refs/heads/."""

    def __init__(self, kit_dir: Path):
        self.kit_dir = kit_dir

    def read_head(self) -> str:
        return (self.kit_dir / "HEAD").read_text().strip()

    def write_head(self, content: str) -> None:
        (self.kit_dir / "HEAD").write_text(content + "\n")

    def current_branch(self) -> Optional[str]:
        head = self.read_head()
        if head.startswith("ref: refs/heads/"):
            return head[len("ref: refs/heads/"):]
        return None  # detached HEAD

    def read_branch(self, branch: str) -> Optional[str]:
        path = self.kit_dir / "refs" / "heads" / branch
        if not path.exists():
            return None
        return path.read_text().strip()

    def write_branch(self, branch: str, sha: str) -> None:
        path = self.kit_dir / "refs" / "heads" / branch
        path.write_text(sha + "\n")

    def head_commit(self) -> Optional[str]:
        head = self.read_head()
        if head.startswith("ref: refs/heads/"):
            return self.read_branch(head[len("ref: refs/heads/"):])
        if len(head) == 40:
            return head
        return None

    def list_branches(self) -> list[str]:
        heads = self.kit_dir / "refs" / "heads"
        return sorted(p.name for p in heads.iterdir() if p.is_file())
```

- [ ] **Step 4: Implement Index**

`python/kit/core/index.py`:

```python
from pathlib import Path


class Index:
    """Tracks staged file paths. One path per line in .kit/index."""

    def __init__(self, path: Path):
        self.path = path

    def read(self) -> list[str]:
        if not self.path.exists():
            return []
        return [l for l in self.path.read_text().splitlines() if l]

    def write(self, entries: list[str]) -> None:
        self.path.write_text("\n".join(sorted(entries)) + ("\n" if entries else ""))

    def add(self, filepath: str) -> None:
        entries = self.read()
        if filepath not in entries:
            entries.append(filepath)
        self.write(entries)
```

- [ ] **Step 5: Implement Repository**

`python/kit/core/repository.py`:

```python
from pathlib import Path
from kit.core.refs import Refs
from kit.core.index import Index


class KitError(Exception):
    pass


class Repository:
    """Central access point for a kit repository on disk."""

    def __init__(self, path: Path):
        self.path = Path(path)
        self.kit_dir = self.path / ".kit"
        self.refs = Refs(self.kit_dir)

    @classmethod
    def init(cls, path: str | Path) -> "Repository":
        root = Path(path)
        kit = root / ".kit"
        (kit / "objects").mkdir(parents=True, exist_ok=True)
        (kit / "refs" / "heads").mkdir(parents=True, exist_ok=True)
        (kit / "HEAD").write_text("ref: refs/heads/master\n")
        (kit / "index").write_text("")
        print(f"Initialized empty kit repository in {kit}")
        return cls(root)

    @classmethod
    def open(cls, path: str | Path = ".") -> "Repository":
        root = Path(path)
        if not (root / ".kit").exists():
            raise KitError("not a kit repository")
        return cls(root)

    def index(self) -> Index:
        return Index(self.kit_dir / "index")

    def objects_dir(self) -> Path:
        return self.kit_dir / "objects"

    def write_object(self, sha: str, data: bytes) -> None:
        (self.objects_dir() / sha).write_bytes(data)

    def read_object(self, sha: str) -> bytes:
        path = self.objects_dir() / sha
        if not path.exists():
            raise KitError(f"object not found: {sha}")
        return path.read_bytes()

    def object_exists(self, sha: str) -> bool:
        return (self.objects_dir() / sha).exists()
```

- [ ] **Step 6: Run tests**

```bash
python -m pytest tests/test_repository.py -v
```

Expected: all PASS.

- [ ] **Step 7: Commit**

```bash
git add python/kit/core/
git commit -m "feat(python): implement Repository, Refs, Index"
```

---

### Task 4: CLI dispatch + commands

**Files:**
- Create: `python/kit/cli/dispatch.py`
- Create: `python/kit/cli/commands/cmd_init.py` through `cmd_stash.py`

- [ ] **Step 1: Create CLI dispatch**

`python/kit/cli/dispatch.py`:

```python
import argparse
import sys


def main():
    parser = argparse.ArgumentParser(prog="kit-py", description="kit-vcs Python implementation")
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("init", help="Initialize repository")

    add_p = sub.add_parser("add", help="Stage files")
    add_p.add_argument("files", nargs="+")

    commit_p = sub.add_parser("commit", help="Create commit")
    commit_p.add_argument("-m", "--message", required=True)
    commit_p.add_argument("--author", default="user")

    sub.add_parser("status", help="Show working tree status")
    sub.add_parser("log", help="Show commit log")

    branch_p = sub.add_parser("branch", help="List or create branches")
    branch_p.add_argument("name", nargs="?")

    checkout_p = sub.add_parser("checkout", help="Switch branch")
    checkout_p.add_argument("branch")

    diff_p = sub.add_parser("diff", help="Show diff")
    diff_p.add_argument("file", nargs="?")

    merge_p = sub.add_parser("merge", help="Merge branch")
    merge_p.add_argument("branch")

    reset_p = sub.add_parser("reset", help="Reset HEAD")
    reset_p.add_argument("--hard", action="store_true")
    reset_p.add_argument("commit", nargs="?")

    stash_p = sub.add_parser("stash", help="Stash changes")
    stash_p.add_argument("action", nargs="?", choices=["pop", "list"])

    args = parser.parse_args()

    from kit.cli.commands import (
        cmd_init, cmd_add, cmd_commit, cmd_status, cmd_log,
        cmd_branch, cmd_checkout, cmd_diff, cmd_merge, cmd_reset, cmd_stash
    )

    match args.command:
        case "init":     cmd_init.run()
        case "add":      cmd_add.run(args.files)
        case "commit":   cmd_commit.run(args.message, args.author)
        case "status":   cmd_status.run()
        case "log":      cmd_log.run()
        case "branch":   cmd_branch.run(args.name)
        case "checkout": cmd_checkout.run(args.branch)
        case "diff":     cmd_diff.run(getattr(args, "file", None))
        case "merge":    cmd_merge.run(args.branch)
        case "reset":    cmd_reset.run(args.hard, args.commit)
        case "stash":    cmd_stash.run(args.action)
```

- [ ] **Step 2: Implement cmd_init, cmd_add, cmd_commit**

`python/kit/cli/commands/cmd_init.py`:
```python
from kit.core.repository import Repository
def run():
    Repository.init(".")
```

`python/kit/cli/commands/cmd_add.py`:
```python
from kit.core.repository import Repository
from kit.core.objects import Blob

def run(files: list[str]):
    repo = Repository.open()
    for f in files:
        blob = Blob.from_file(f)
        repo.write_object(blob.hash(), blob.serialize())
        repo.index().add(f)
        print(f"staged: {f}")
```

`python/kit/cli/commands/cmd_commit.py`:
```python
import time
from kit.core.repository import Repository, KitError
from kit.core.objects import Blob, Tree, TreeEntry, Commit
from pathlib import Path

def run(message: str, author: str):
    repo = Repository.open()
    staged = repo.index().read()
    if not staged:
        raise KitError("nothing to commit")

    entries = []
    for f in staged:
        blob = Blob.from_file(f)
        repo.write_object(blob.hash(), blob.serialize())
        entries.append(TreeEntry(mode="100644", name=f, hash=blob.hash()))

    tree = Tree(entries=entries)
    repo.write_object(tree.hash(), tree.serialize())

    parent = repo.refs.head_commit()
    commit = Commit(
        tree=tree.hash(), parent=parent, author=author,
        timestamp=int(time.time()), message=message
    )
    repo.write_object(commit.hash(), commit.serialize())

    branch = repo.refs.current_branch() or "master"
    repo.refs.write_branch(branch, commit.hash())
    print(f"[{commit.hash()[:7]}] {message}")
```

- [ ] **Step 3: Stub remaining commands**

For each of `cmd_status`, `cmd_log`, `cmd_branch`, `cmd_checkout`, `cmd_diff`, `cmd_merge`, `cmd_reset`, `cmd_stash`:

```python
# python/kit/cli/commands/cmd_status.py
def run():
    print("status: not yet implemented")
```

- [ ] **Step 4: Create commands __init__.py (empty)**

```python
# python/kit/cli/commands/__init__.py
```

- [ ] **Step 5: Smoke test**

```bash
cd /tmp && rm -rf py-test && mkdir py-test && cd py-test
kit-py init
echo "# hello" > README.md
kit-py add README.md
kit-py commit -m "Initial commit"
```

Expected: commit hash printed.

- [ ] **Step 6: Run all tests**

```bash
cd /home/kyuna/Desktop/kit-vcs/python
python -m pytest tests/ -v
```

Expected: all PASS.

- [ ] **Step 7: Commit**

```bash
git add python/
git commit -m "feat(python): implement CLI dispatch and core commands"
```
