# Advanced Vector

A custom `Vector<T>` container in C++17, built on top of a `RawMemory<T>` wrapper for raw storage. It mimics the core of `std::vector` and is written to be exception-safe.

## Features

- Manual memory management: raw allocation, placement new, explicit destruction.
- Copy and move construction and assignment (copy-and-swap where the storage must grow).
- `Reserve`, `Resize`, `PushBack`, `EmplaceBack`, `Emplace`, `Insert`, `Erase`, `PopBack`.
- Safe reallocation: elements are moved if their move constructor is `noexcept` (or they cannot be copied), otherwise copied, so the vector stays valid if an exception is thrown.
- Safe insertion of an element that belongs to the same vector (for example `v.PushBack(v[0])`).
- Iterators (`begin`, `end`, `cbegin`, `cend`).

## Build and run

```
cd advanced-vector
g++ -std=c++17 main.cpp -o vector_test
./vector_test
```

Do not build with `-DNDEBUG`: the tests use `assert`.

## Tests

`main.cpp` contains six assert-based test groups. They use a tracking class that counts constructions, copies, moves, assignments and destructions, and they check that no objects leak, that exceptions leave the vector in a valid state, and that the expected number of copies and moves happens. At the end a small benchmark compares the operation counts of `Vector` with `std::vector`.

## Project layout

- `advanced-vector/vector.h`: `RawMemory<T>` and `Vector<T>`.
- `advanced-vector/main.cpp`: tests and benchmark.
