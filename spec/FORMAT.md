# kit-vcs Object Format Specification

This document describes the on-disk wire format produced by the C++ implementation of kit-vcs.
All Rust (and any future) implementations must read and write the exact same bytes to be interoperable.

---

## 1. Repository Layout

`kit init` creates the following structure inside the working directory:

```
<root>/
└── .kit/
    ├── HEAD          # current branch pointer or detached commit hash
    ├── index         # staged file list (path → blob hash)
    ├── objects/      # content-addressed object store (flat directory)
    └── refs/
        └── heads/    # one file per branch, named after the branch
```

No subdirectories are created inside `objects/` — all objects are stored as flat files.

---

## 2. HEAD File

**Path:** `.kit/HEAD`

### Format A — symbolic ref (normal branch)

```
ref: refs/heads/<branch-name>\n
```

Example:
```
ref: refs/heads/master
```

The line is always terminated by a single newline (`\n`). When reading, the trailing newline is stripped before use.

### Format B — detached HEAD

```
<40-char-lowercase-hex-sha1>\n
```

Example:
```
a3f1c2d4e5b6789012345678901234567890abcd
```

A detached HEAD is detected by the absence of the `ref: refs/heads/` prefix.

---

## 3. Index File

**Path:** `.kit/index`

The index is a plain-text file. Each line records one staged file:

```
<blob-hash> <path>\n
```

- `<blob-hash>` — 40-character lowercase hex SHA1 of the blob object.
- `<path>` — the file path as staged (workspace-relative). A single space separates the hash from the path.
- Lines are separated by `\n`. Blank lines are ignored on load.
- On `kit init`, the file is created empty.
- Entry order is unspecified (backed by an unordered map); implementations must not rely on order.

Example:
```
da39a3ee5e6b4b0d3255bfef95601890afd80709 README.md
7c4a8d09ca3762af61e59520943dc26494f8941b src/main.cpp
```

---

## 4. Object Storage

**Directory:** `.kit/objects/`

Objects are stored as flat files. The filename is the full 40-character lowercase hex SHA1 of the serialized object content. No compression or binary encoding is applied — objects are stored as raw UTF-8 text exactly as serialized.

```
.kit/objects/<40-char-sha1>
```

The SHA1 is computed with OpenSSL's EVP interface (`EVP_sha1`) over the raw serialized bytes of the object. The hash is formatted as lowercase hexadecimal, two digits per byte, zero-padded.

---

## 5. Blob Object

A blob stores the raw content of a single file.

### Serialization format

```
blob <size>\n<content>
```

- `blob ` — literal ASCII string `blob` followed by a single space.
- `<size>` — decimal byte count of `<content>` (no leading zeros).
- `\n` — a single newline separating the header from the content.
- `<content>` — the raw file bytes; no further encoding.

Example (file containing `hello\n`):
```
blob 6
hello
```

### Hash

`SHA1("blob <size>\n<content>")`

### Deserialization

Find the first `\n`; everything after it is the content. The header before `\n` is `blob <size>`.

---

## 6. Tree Object

A tree is a snapshot of the staged files at commit time. Each line describes one entry.

### Serialization format

```
<mode> <blob-hash> <name>\n
```

One line per entry, in the order the entries were added (iteration order of the C++ `unordered_map` converted to a `Tree` at commit time; **not** guaranteed to be lexicographic).

- `<mode>` — currently always the literal string `blob`.
- `<blob-hash>` — 40-character lowercase hex SHA1 of the blob object for this file.
- `<name>` — the path/filename of the file (workspace-relative, same as stored in the index).
- `\n` — line terminator after each entry. Blank lines are ignored on load.

Example:
```
blob da39a3ee5e6b4b0d3255bfef95601890afd80709 README.md
blob 7c4a8d09ca3762af61e59520943dc26494f8941b src/main.cpp
```

An empty tree (no staged files) serializes to the empty string `""`.

### Hash

`SHA1` of the full serialized string (which may be empty for an empty tree).

---

## 7. Commit Object

### Serialization format — root commit (no parent)

```
tree <tree-hash>\n
author <author-string>\n
timestamp <unix-seconds>\n
\n
<message>
```

### Serialization format — child commit (with parent)

```
tree <tree-hash>\n
parent <parent-hash>\n
author <author-string>\n
timestamp <unix-seconds>\n
\n
<message>
```

Field details:

| Field | Type | Description |
|-------|------|-------------|
| `tree` | 40-char hex SHA1 | Hash of the tree object for this commit's snapshot. |
| `parent` | 40-char hex SHA1 | Hash of the preceding commit. **Absent** for the root commit. |
| `author` | string | Value of the `KIT_AUTHOR` env var; falls back to the output of `whoami` (trailing newline stripped); falls back to `"unknown"`. |
| `timestamp` | int64, decimal | Unix epoch seconds (UTC), written as a plain decimal integer. |
| message | string | Everything after the blank line (`\n\n`). Multi-line messages are supported. No trailing newline is added by the serializer. |

### Hash

`SHA1` of the full serialized string.

### Deserialization

Lines before the first blank line are parsed as `<key> <value>` pairs. The blank line ends the header; all remaining content (including subsequent newlines) becomes the message. The parser strips a single trailing newline from each header value.

---

## 8. Refs

### Branch ref file

**Path:** `.kit/refs/heads/<branch-name>`

```
<40-char-lowercase-hex-sha1>\n
```

One commit hash followed by a newline. When reading, the trailing `\n` is stripped. A branch ref file is created or overwritten atomically (via a single `write_file` call) whenever a branch is created or advanced.

### Branch enumeration

All regular files directly inside `.kit/refs/heads/` are branch names. Subdirectories are not created inside `refs/heads/`.

---

## 9. Invariants

All conforming implementations (C++, Rust, or any other language) must satisfy the following invariants:

1. **SHA1 input is the raw serialized string** — hash is computed over the serialized bytes before any storage operation, never over the file path or any other derived value.

2. **Lowercase hex only** — all SHA1 digests are stored and compared as 40-character lowercase hexadecimal strings. Mixed case is invalid.

3. **No compression** — objects are stored verbatim (no zlib, gzip, or other encoding). This differs from git's object format.

4. **No fanout subdirectory** — objects are stored as `.kit/objects/<full-40-char-hash>`, not split into `objects/ab/cdef…` like git.

5. **Newline termination on ref files** — HEAD and branch ref files always end with `\n`. Implementations must write it and must strip it when reading.

6. **`parent` line is omitted for root commits** — a commit object with no parent must not include a `parent` line at all. An empty `parent` field is not the same as an absent one.

7. **Index order is unspecified** — the index file is an unordered set of entries. Implementations must not assume any ordering when reading.

8. **Object filenames are exact hashes** — the filename stored under `objects/` equals `SHA1(serialized_content)` with no truncation or prefix modification.

9. **Text encoding** — all text (paths, author strings, messages) is treated as raw bytes (effectively UTF-8 in practice). No encoding conversion is performed.

10. **Initial HEAD** — `kit init` always writes `ref: refs/heads/master\n` to HEAD. The default branch name is `master`.
