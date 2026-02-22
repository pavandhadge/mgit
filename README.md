# mgit - A Simple Git Clone
![MGit Logo](./docs/mgit.png)
## Description

`mgit` is a simple, lightweight clone of the popular version control system, Git. It is written in C++ and aims to replicate the core functionalities of Git, such as initializing repositories, adding and committing files, creating branches, and merging changes. This project is intended for educational purposes, to understand the inner workings of a distributed version control system.

## Features

*   **Repository Initialization:** Create a new `mgit` repository from scratch.
*   **File Management:** Add files to the staging area and commit them to the repository.
*   **Branching and Merging:** Create, switch, and delete branches. Merge branches with conflict resolution.
*   **Conflict Resolution:** When a merge conflict occurs, `mgit` provides clear instructions on how to resolve it. You can either manually resolve the conflicts and continue the merge or abort the merge altogether.
*   **Status:** View the status of your working directory and staging area.
*   **Object Model:** `mgit` uses a Git-like object model with blobs, trees, and commits.

## Getting Started

### Prerequisites

Before you can build `mgit`, you need to have the following software installed on your system:

*   A C++ compiler that supports C++17 (e.g., GCC, Clang)
*   CMake (version 3.13 or higher)
*   Zlib
*   SQLite3
*   xmake

### Building `mgit`

`mgit` uses xmake for local development builds.
CLI11 is vendored in the repository (`external/CLI/CLI.hpp`).
OpenSSL is not required anymore; SHA-1 is implemented internally.

**1. Clone the repository:**

```bash
git clone https://github.com/your-username/mgit.git
cd mgit
```

**2. Configure + build:**

```bash
export XMAKE_GLOBALDIR=$PWD/.xmake-global
xmake f -m release
xmake
```

For debug builds:

```bash
export XMAKE_GLOBALDIR=$PWD/.xmake-global
xmake f -m debug
xmake
```

**3. Run:**

```bash
xmake run mgit -- --help
```

**4. Test:**

```bash
xmake run test
```

**5. Compile + test:**

```bash
xmake run compile-test
```

Both test commands run in isolated temporary directories under `/tmp` and do not touch this repository's `.git` or `.mgit`.

## Command Reference

Here is a list of the most common `mgit` commands:

| Command | Description |
| --- | --- |
| `mgit init` | Initialize a new `mgit` repository. |
| `mgit add <file(s)>` | Add file(s) to the staging area. |
| `mgit commit -m "<message>"` | Commit the staged changes. |
| `mgit status` | Show the status of the working directory. |
| `mgit branch` | List all branches. |
| `mgit branch <name>` | Create a new branch. |
| `mgit switch <name>` | Switch to a different branch. |
| `mgit merge <branch>` | Merge a branch into the current branch. |
| `mgit merge --continue` | Continue a merge after resolving conflicts. |
| `mgit merge --abort` | Abort a merge in progress. |

## Contributing

Contributions are welcome! If you would like to contribute to `mgit`, please follow these steps:

1.  Fork the repository.
2.  Create a new branch for your feature or bug fix.
3.  Make your changes and commit them with descriptive messages.
4.  Push your changes to your fork.
5.  Create a pull request to the main repository.
