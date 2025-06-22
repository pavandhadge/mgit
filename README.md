
# 🧠 `mgit` — A Minimal Git Clone with Better Readability

**`mgit`** is a personal Git clone written in **C++**, built to demystify how Git works internally and to explore new ways of improving its usability and readability for users.

This project reimplements Git’s core concepts — blobs, trees, commits, tags, and object storage — using a clean CLI interface and simple architecture.
It's built as a learning tool **and** as a possible base for more human-readable version control.

---

## 🚀 Features

* ✅ Git-style object storage with `.git/objects`
* ✅ SHA-1 hashing of file data (`mgit hash-object`)
* ✅ Tree and directory snapshotting (`mgit write-tree`)
* ✅ Commit creation with metadata (`mgit commit-tree`)
* ✅ Annotated tagging (`mgit tag-object`)
* ✅ Raw and pretty object reading (`mgit cat-file`, `mgit ls-tree`)
* ✅ CLI11-based CLI for clear UX
* 🧪 Educational source code ideal for learning Git internals

---

## 🛠 Commands

### 🔧 Initialization

```bash
mgit init [path]
```

Creates a `.git`-like object store in the specified directory.

---

### 📄 Hash and Store Files

```bash
mgit hash-object --file <file> [-w]
```

* Hashes a file like Git's `hash-object`
* Optionally writes it to `.git/objects` if `-w` is used

---

### 🌳 Tree Object Creation

```bash
mgit write-tree --folder <path>
```

Recursively creates a tree object representing the directory.

---

### 📝 Create Commits

```bash
mgit commit-tree --tree <tree_hash> --parent <parent_hash> --message <msg> --author "<name> <email>"
```

Builds a commit object pointing to a tree and optional parent.

---

### 🏷 Create Tags

```bash
mgit tag-object --object <hash> --type <type> --tag <name> --message <msg> --tagger "<name> <email>"
```

Creates an annotated tag for a blob, tree, or commit.

---

### 🧪 Object Inspection

#### Raw Dump

```bash
mgit read-object --hash <sha>
```

Prints full raw content (header + body) from object store.

#### Git-style Inspection

```bash
mgit cat-file <sha> [-p] [-t] [-s]
```

* `-p` = Print content
* `-t` = Print type
* `-s` = Print size

---

### 📂 Tree Reading

```bash
mgit ls-tree --hash <tree_sha>
```

Pretty-prints a tree object, showing filenames, modes, and hashes — like `git ls-tree`.

---

### 🧠 Auto-type Smart Read

```bash
mgit ls-read <sha>
```

Auto-detects the type and displays content in human-readable format.

---

## 📚 Why I Built This

I made `mgit` to:

* Learn Git internals by re-implementing them
* Understand Git objects, trees, commits, and tags deeply
* Build a version control interface that's easier for humans to reason about

It’s a personal project, educational journey, and exploration of simplicity in tooling.

---

## ⚙️ Build Instructions

### Dependencies

* C++17+
* `zlib` (for compression)
* [CLI11](https://github.com/CLIUtils/CLI11)

### Compile

```bash
g++ -std=c++17 -lz -o mgit src/*.cpp
```

---

## 📁 Project Structure

```
mgit/
├── src/
│   ├── main.cpp
│   ├── GitRepository.cpp
│   ├── BlobObject.cpp
│   └── ...
├── headers/
│   └── *.hpp
└── .git/objects/  ← Created at runtime
```

---

## 📈 Roadmap

* [ ] Add staging/index support (`mgit add`)
* [ ] Implement basic branches
* [ ] `mgit log` with pretty output
* [ ] Remote pushing/pulling for learning
* [ ] Explore more user-friendly logs/commits

---

## 🧑‍💻 Author

**Sharad Etthar** — [@yourname](#)
Learning Git internals, systems programming, and building devtools for developers.

---

Would you like a logo or example `.gif` of it in action for the README too?
