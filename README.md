# Regex-Engine

A high-performance Regex engine written from scratch in C++20. It compiles regular expressions into finite state machines, namely an **NFA** (Thompson's Construction) and a **DFA** (Subset Construction).

## Supports
* **Operators:** `*` (Kleene Star), `+` (Plus), `?` (Optional), `|` (Union).
* **Special Characters:** `.` (Wildcard), `()` (Grouping), `\` (Escaping), `^` and `$` (Anchors).
* **Character Classes:** `[a-z]`, `[0-9]`, `[0-9a-gxyz], [🚀-🚢]`, etc.
* **Encoding:** Full UTF-8 Support.

## Features

### UTF-8 std::string Parsing Iterator
Uses a custom `UTF8View` iterator to parse raw bytes into UTF-8 during iteration to reduce memory usage and avoid copying the underlying std::string. You can easily switch between UTF-8 parsing and ASCII (std::string) by modifying the alias in ``nfa.hpp``:
```cpp
// using string = std::string;
using string = UTF8View;
```

### Lazy DFA Construction (Optional)
Prevents exponential state explosion during construction. States are initialized as needed during matching.

ex: (a|b)*a(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)

The above adversarial regular expression takes 2.45s and 1million+ states to construct a DFA for. DFA construction time/memory increases exponentially with the \# of "(a|b)"s.

**Construction Speed:** Reduces compilation time for the above adversarial input by **99.99%**.

Measured on the above regex, with a 16.5k-character long candidate string used for matching.
| Metric | Eager DFA | Lazy DFA | Impact |
| :--- | :--- | :--- | :--- |
| **Construction Time** | 2.45s | **100µs** | **24,500x Faster Startup** |
| **Matching (Cold)** | 42 µs | 142 µs | State initialization overhead |
| **Matching (Hot)** | 38 µs | **32 µs** | Performant after warm-up |

**Runtime Trade-off:** 256x theoretical matching slowdown by a factor of |Σ| (256 for ASCII) in state initialization, though a one-time cost and extremely rare in practice.

Due to the tradeoff between construction and matching times, this feature is optional, though highly recommended for memory-critical applications. Choose eager construction if DFA construction time is irrelevant to your use-case, though be aware that even small regular expressions could take until the heat death of the universe to eagerly construct.

## Build & Run

**Requires:** `g++` (C++20 support) and `make`.

```bash
# Build (Release mode with -O3)
make

# Run the CLI
./main
```
## Resources
* [Regular Expression Matching Can Be Simple And Fast](https://swtch.com/~rsc/regexp/regexp1.html)
* [Shunting Yard Algorithm](https://en.wikipedia.org/wiki/Shunting_yard_algorithm)

## Roadmap
In descending order of priority:
* Comprehensive test suite
* Refactor file structure
* Benchmark against std::regex
* Support Unicode character class escapes
