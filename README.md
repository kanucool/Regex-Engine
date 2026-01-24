# Regex-Engine

A high-performance Regex engine written from scratch in C++20. It compiles regular expressions into finite state machines, namely an **NFA** (Thompson's Construction) and a **DFA** (Subset Construction).

## Supports
* **Operators:** `*` (Kleene Star), `+` (Plus), `?` (Optional), `|` (Union).
* **Special Characters:** `.` (Wildcard), `()` (Grouping).

## Optimizations
* **Paged Memory Arena:** Allocates graph nodes in pages to mitigate heap fragmentation and improve cache locality.
* **Bitmasking:** Uses a `std::bitset` for faster set lookups during DFA construction, migrating to `std::vector` keys mid-construction if state count exceeds an adjustable threshold.

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
* Character classes ([a-z], [0-9], etc)
* Comprehensive test suite
* Benchmark against std::regex
* Refactor file structure
* Extend support for Unicode
