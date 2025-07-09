# 🧠 `mgit` — A Minimal Git Clone with Better Readability

**`mgit`** is a personal Git clone written in **C++**, built to demystify how Git works internally and to explore new ways of improving its usability and readability for users.

This project reimplements Git’s core concepts — blobs, trees, commits, tags, and object storage — using a clean CLI interface and simple architecture. It's built as a learning tool **and** as a possible base for more human-readable version control.

---

## 🚀 Features

* ✅ Git-style object storage with `.git/objects`
* ✅ SHA-1 hashing of file data (`mgit hash-object`)
* ✅ Tree and directory snapshotting (`mgit write-tree`)
* ✅ Commit creation with metadata (`mgit commit-tree` and `mgit commit`)
* ✅ Annotated tagging (`mgit tag-object`)
* ✅ Raw and pretty object reading (`mgit cat-file`, `mgit ls-tree`, `mgit read-object`, `mgit ls-read`)
* ✅ Staging/index support (`mgit add`)
* ✅ Status reporting (`mgit status`)
* ✅ Branch management (`mgit branch`, `mgit switch`, `mgit checkout`)
* ✅ Merge operations and conflict resolution (`mgit merge`, `mgit merge-continue`, `mgit merge-abort`, `mgit resolve-conflict`)
* ✅ Remote operations (`mgit push`, `mgit pull`, `mgit remote add/remove/list`)
* ✅ Configuration management (`mgit config`)
* ✅ Activity logging and analytics (`mgit activity` with subcommands: summary, performance, errors, analysis, timeline, health, workflow, slow, usage, stats, recent, export, raw, errors-raw, performance-raw)
* ✅ CLI11-based CLI for clear UX
* 🧑‍💻 Educational source code ideal for learning Git internals
* 📝 **Comprehensive activity logging and AI-ready analysis**

---

## ⚠️ Important Note: CLI11 Dependency

For the build to work, you must clone the CLI11 library into the `external/` directory:

```sh
git clone https://github.com/CLIUtils/CLI11.git external/CLI11
```

This is required for the CLI parsing functionality.

---

## 🛠️ Commands

See [docs/API_REFERENCE.md](docs/API_REFERENCE.md) for a full list of commands and their options.

---

## 📦 Build Instructions

### Dependencies (managed by vcpkg)

* C++17+
* `zlib` (for compression)
* `openssl` (for hashing)
* `sqlite3` (for activity logging)
* `cli11` (for CLI parsing)

### Build with vcpkg and CMake

1. **Install vcpkg** (if not already):
   ```sh
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.sh
   ```
2. **Install dependencies:**
   ```sh
   ./vcpkg/vcpkg install
   ```
3. **Build the project:**
   ```sh
   ./mgit.sh
   ```
   This script will configure and build your project using vcpkg, then run your program.

---

## 📁 Project Structure

```
mgit/
├── src/
│   ├── main.cpp
│   ├── GitRepository.cpp
│   ├── GitActivityLogger.cpp
│   └── ...
├── headers/
│   └── *.hpp
├── .git/objects/  ← Created at runtime
├── vcpkg.json
├── mgit.sh
├── external/
│   └── CLI11/   ← Required for CLI parsing
└── docs/
    ├── ARCHITECTURE.md
    └── API_REFERENCE.md
```

---

## 📚 Documentation

- **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md):** High-level design, component interactions, and data flow.
- **[docs/API_REFERENCE.md](docs/API_REFERENCE.md):** Detailed class, method, and command documentation.

---

## 📈 Activity Logging & AI Analysis

`mgit` automatically logs all command activity, performance, and errors to `.mgit/activity.log` and related files. See [docs/API_REFERENCE.md](docs/API_REFERENCE.md#activity-logging) for details.

---

## 🧑‍💻 Why I Built This

I made `mgit` to:

* Learn Git internals by re-implementing them
* Understand Git objects, trees, commits, and tags deeply
* Build a version control interface that's easier for humans to reason about
* **Explore AI-driven developer tooling and analytics**

It's a personal project, educational journey, and exploration of simplicity in tooling.

---

## License

**No license is currently set for this project. If you intend to use or contribute, please contact the author or open an issue to discuss licensing.**
