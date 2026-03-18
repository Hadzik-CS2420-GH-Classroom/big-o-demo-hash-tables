# Big O Demo: Hash Tables

**Not graded** — this is a runnable demonstration, not an assignment.

## What This Shows

Benchmarks hash table operations at increasing input sizes using two collision strategies:
chaining (array of linked lists) and linear probing (open addressing). Demonstrates how
load factor affects performance and why hash tables achieve O(1) average-case lookups.

| Operation | Chaining | Linear Probing | Why |
|-----------|----------|----------------|-----|
| `insert` | O(1) avg / O(n) worst | O(1) avg / O(n) worst | Hash + append vs probe |
| `search` | O(1) avg / O(n) worst | O(1) avg / O(n) worst | Hash + scan chain vs probe |
| `remove` | O(1) avg / O(n) worst | O(1) avg / O(n) worst | Same pattern |
| `resize` | O(n) | O(n) | Must rehash everything |

The demo also shows how performance degrades as load factor increases (0.5 vs 0.9 vs 1.5
for chaining), and the O(n) cost of resizing.

## How to Run

```bash
cmake -B build
cmake --build build
./build/big-o-demo-hash-tables
```

The output shows timing tables so you can see how O(1) average-case operations stay flat
across input sizes, while high load factors and resize operations reveal the underlying costs.
