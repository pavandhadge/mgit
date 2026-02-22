#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

static std::string shellQuote(const std::string &s) {
  std::string out = "'";
  for (char c : s) {
    if (c == '\'')
      out += "'\"'\"'";
    else
      out += c;
  }
  out += "'";
  return out;
}

static fs::path makeTempDir(const std::string &prefix) {
  std::string pattern = (fs::temp_directory_path() / (prefix + "-XXXXXX")).string();
  std::vector<char> buf(pattern.begin(), pattern.end());
  buf.push_back('\0');
  char *res = mkdtemp(buf.data());
  if (!res) {
    throw std::runtime_error("mkdtemp failed for pattern: " + pattern);
  }
  return fs::path(res);
}

struct CmdResult {
  int rc;
  std::string output;
};

static CmdResult runCmd(const fs::path &cwd, const std::string &command) {
  std::string pattern =
      (fs::temp_directory_path() / "mgit-cmd-output-XXXXXX").string();
  std::vector<char> outBuf(pattern.begin(), pattern.end());
  outBuf.push_back('\0');
  int fd = mkstemp(outBuf.data());
  if (fd < 0) {
    throw std::runtime_error("mkstemp failed for command output");
  }
  close(fd);
  fs::path outFile(outBuf.data());
  std::string wrapped =
      "cd " + shellQuote(cwd.string()) + " && " + command + " > " +
      shellQuote(outFile.string()) + " 2>&1";
  int raw = std::system(wrapped.c_str());
  int rc = -1;
  if (raw != -1) {
    if (WIFEXITED(raw))
      rc = WEXITSTATUS(raw);
    else
      rc = 128;
  }
  std::ifstream in(outFile);
  std::string output((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
  std::error_code ec;
  fs::remove(outFile, ec);
  return {rc, output};
}

static bool contains(const std::string &haystack, const std::string &needle) {
  return haystack.find(needle) != std::string::npos;
}

int main(int argc, char **argv) {
  try {
    fs::path mgitPath =
        (argc > 1) ? fs::path(argv[1]) : fs::path("build/linux/x86_64/debug/mgit");
    mgitPath = fs::absolute(mgitPath);
    std::string mgit = mgitPath.string();
    if (!fs::exists(mgitPath)) {
      std::cerr << "mgit binary not found: " << mgitPath << "\n";
      return 2;
    }

    const fs::path repo = makeTempDir("mgit-test-repo");
    const fs::path remote = makeTempDir("mgit-test-remote");
    std::vector<std::string> failures;

    auto expectZero = [&](const std::string &name, const std::string &cmd) {
      CmdResult r = runCmd(repo, cmd);
      if (r.rc != 0) {
        failures.push_back(name + " expected rc=0 got " + std::to_string(r.rc) +
                           "\n" + r.output);
      }
    };
    auto expectZeroContains = [&](const std::string &name, const std::string &cmd,
                                  const std::string &mustContain) {
      CmdResult r = runCmd(repo, cmd);
      if (r.rc != 0) {
        failures.push_back(name + " expected rc=0 got " + std::to_string(r.rc) +
                           "\n" + r.output);
        return;
      }
      if (!contains(r.output, mustContain)) {
        failures.push_back(name + " missing message: " + mustContain + "\n" +
                           r.output);
      }
    };
    auto expectNonZero = [&](const std::string &name, const std::string &cmd,
                             const std::string &mustContain = "") {
      CmdResult r = runCmd(repo, cmd);
      if (r.rc == 0) {
        failures.push_back(name + " expected non-zero rc, got 0\n" + r.output);
      } else if (!mustContain.empty() && !contains(r.output, mustContain)) {
        failures.push_back(name + " missing message: " + mustContain + "\n" +
                           r.output);
      }
    };

    // Init and commit baseline
    expectZero("init", "printf 'Tester\\ntester@example.com\\n' | " +
                           shellQuote(mgit) + " init .git");
    {
      std::ofstream(repo / "a.txt") << "hello\n";
      std::ofstream(repo / "b.txt") << "world\n";
    }
    CmdResult hashRes = runCmd(repo, shellQuote(mgit) + " hash-object -w a.txt");
    if (hashRes.rc != 0 || !std::regex_search(hashRes.output, std::regex("[0-9a-f]{40}"))) {
      failures.push_back("hash-object expected valid sha1 output\n" + hashRes.output);
    }
    expectZero("add", shellQuote(mgit) + " add a.txt b.txt");
    expectZeroContains("commit", shellQuote(mgit) + " commit -m 'initial'",
                       "Commit created successfully.");
    expectZeroContains("status clean", shellQuote(mgit) + " status",
                       "working tree clean");

    CmdResult writeTreeRes = runCmd(repo, shellQuote(mgit) + " write-tree");
    std::smatch m;
    std::string treeHash;
    if (writeTreeRes.rc != 0 ||
        !std::regex_search(writeTreeRes.output, m,
                           std::regex("Tree object written: ([0-9a-f]{40})"))) {
      failures.push_back("write-tree expected tree hash output\n" + writeTreeRes.output);
    } else {
      treeHash = m[1];
    }
    CmdResult headRes = runCmd(repo, shellQuote(mgit) + " cat-file -t HEAD");
    if (headRes.rc != 0 || !contains(headRes.output, "commit")) {
      failures.push_back("cat-file -t HEAD expected commit\n" + headRes.output);
    }
    expectZeroContains("ls-tree-r", shellQuote(mgit) + " ls-tree-r " + treeHash, "a.txt");

    // Command behavior checks
    expectZero("branch create", shellQuote(mgit) + " branch feature");
    expectZeroContains("branch list", shellQuote(mgit) + " branch -l", "feature");
    expectZero("switch feature", shellQuote(mgit) + " switch feature");
    {
      std::ofstream(repo / "feature.txt") << "feature branch\n";
    }
    expectZero("add feature", shellQuote(mgit) + " add feature.txt");
    expectZero("commit feature", shellQuote(mgit) + " commit -m 'feature'");
    expectZero("checkout main", shellQuote(mgit) + " checkout main");
    expectZero("merge feature", shellQuote(mgit) + " merge feature");
    expectZero("branch delete merged", shellQuote(mgit) + " branch -d feature");

    // These were previously broken: they printed errors but returned rc=0.
    expectNonZero("merge --continue no state", shellQuote(mgit) + " merge --continue",
                  "Cannot complete merge");
    expectNonZero("resolve-conflict invalid",
                  shellQuote(mgit) + " resolve-conflict missing.txt deadbeef",
                  "Failed to resolve conflict");
    expectNonZero("log unimplemented", shellQuote(mgit) + " log",
                  "Not implemented");

    // Remote flow
    {
      fs::create_directories(remote / ".git");
      std::string initRemote =
          "cd " + shellQuote(remote.string()) +
          " && printf 'Remote\\nremote@example.com\\n' | " + shellQuote(mgit) +
          " init .git > /dev/null 2>&1";
      std::system(initRemote.c_str());
    }
    expectZero("remote add", shellQuote(mgit) + " remote add origin " +
                                 shellQuote((remote / ".git").string()));
    expectZeroContains("remote list", shellQuote(mgit) + " remote list", "origin");
    expectZero("push", shellQuote(mgit) + " push origin");
    expectZero("pull", shellQuote(mgit) + " pull origin");
    expectZero("remote remove", shellQuote(mgit) + " remote remove origin");

    // Stress: many files in a single commit path
    const int stressFiles = 300;
    for (int i = 0; i < stressFiles; ++i) {
      std::ofstream(repo / ("stress_" + std::to_string(i) + ".txt"))
          << "line " << i << "\n";
    }
    expectZero("stress add", shellQuote(mgit) + " add .");
    expectZero("stress commit", shellQuote(mgit) + " commit -m 'stress'");
    expectZeroContains("activity recent", shellQuote(mgit) + " activity recent -l 5",
                       "Recent Activity");

    if (!failures.empty()) {
      std::cerr << "Integration test failures: " << failures.size() << "\n";
      for (const auto &f : failures) {
        std::cerr << "----\n" << f << "\n";
      }
      return 1;
    }

    std::cout << "All integration checks passed in isolated temp dirs.\n";
    std::cout << "repo: " << repo << "\n";
    std::cout << "remote: " << remote << "\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "fatal: " << e.what() << "\n";
    return 2;
  }
}
