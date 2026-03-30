// ============================================================================
// Big O Demo: Hash Tables
// ============================================================================
// This demo times hash table operations at increasing input sizes so you can
// SEE average-case O(1) performance and how load factor degrades it.
//
// Two collision strategies are compared:
//   - Chaining: each bucket is a linked list
//   - Linear probing: on collision, scan forward to the next open slot
//
// Not graded -- run it, read the output, and observe the patterns.
// ============================================================================

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <list>
#include <utility>
#include <functional>
#include <fstream>
#include <cstdlib>

// ── Simple Hash Function ────────────────────────────────────────────────────
// Sum of character values mod capacity. Intentionally simple so students
// can see how it works -- not a production-quality hash.

size_t simple_hash(const std::string& key, size_t capacity) {
    size_t sum = 0;
    for (char c : key) {
        sum += static_cast<size_t>(c);
    }
    return sum % capacity;
}

// ── Generate Keys ───────────────────────────────────────────────────────────
// Produces deterministic string keys: "key_0", "key_1", ...

std::vector<std::string> generate_keys(int count) {
    std::vector<std::string> keys;
    keys.reserve(count);
    for (int i = 0; i < count; i++) {
        keys.push_back("key_" + std::to_string(i));
    }
    return keys;
}

// ── Chaining Hash Table ─────────────────────────────────────────────────────
// Each bucket is a std::list of (key, value) pairs.

class ChainingHashTable {
public:
    explicit ChainingHashTable(size_t capacity)
        : buckets_(capacity), size_{0}, capacity_{capacity} {}

    void insert(const std::string& key, int value) {
        size_t index = simple_hash(key, capacity_);
        // Check if key already exists -- update if so
        for (auto& [k, v] : buckets_[index]) {
            if (k == key) {
                v = value;
                return;
            }
        }
        buckets_[index].emplace_back(key, value);
        size_++;
    }

    bool search(const std::string& key) const {
        size_t index = simple_hash(key, capacity_);
        for (const auto& [k, v] : buckets_[index]) {
            if (k == key) return true;
        }
        return false;
    }

    bool remove(const std::string& key) {
        size_t index = simple_hash(key, capacity_);
        auto& chain = buckets_[index];
        for (auto it = chain.begin(); it != chain.end(); ++it) {
            if (it->first == key) {
                chain.erase(it);
                size_--;
                return true;
            }
        }
        return false;
    }

    // Resize: allocate a new bucket array and rehash everything -- O(n)
    void resize(size_t new_capacity) {
        std::vector<std::list<std::pair<std::string, int>>> new_buckets(new_capacity);
        for (auto& chain : buckets_) {
            for (auto& [k, v] : chain) {
                size_t index = simple_hash(k, new_capacity);
                new_buckets[index].emplace_back(std::move(k), v);
            }
        }
        buckets_ = std::move(new_buckets);
        capacity_ = new_capacity;
    }

    size_t get_size() const { return size_; }
    size_t get_capacity() const { return capacity_; }
    double load_factor() const { return static_cast<double>(size_) / capacity_; }

private:
    std::vector<std::list<std::pair<std::string, int>>> buckets_;
    size_t size_;
    size_t capacity_;
};

// ── Linear Probing Hash Table ───────────────────────────────────────────────
// On collision, scan forward (wrapping around) until an empty slot is found.
// Uses a sentinel state to mark deleted slots for correct search behavior.

class LinearProbingHashTable {
public:
    explicit LinearProbingHashTable(size_t capacity)
        : capacity_{capacity}, size_{0} {
        keys_.resize(capacity);
        values_.resize(capacity, 0);
        state_.resize(capacity, SlotState::EMPTY);
    }

    void insert(const std::string& key, int value) {
        // Don't allow the table to fill completely -- leave at least one EMPTY
        // slot so probing always terminates
        if (size_ + 1 >= capacity_) return;

        size_t index = simple_hash(key, capacity_);
        size_t start = index;
        do {
            if (state_[index] == SlotState::EMPTY ||
                state_[index] == SlotState::DELETED) {
                keys_[index] = key;
                values_[index] = value;
                state_[index] = SlotState::OCCUPIED;
                size_++;
                return;
            }
            if (state_[index] == SlotState::OCCUPIED && keys_[index] == key) {
                values_[index] = value;  // update existing
                return;
            }
            index = (index + 1) % capacity_;
        } while (index != start);
    }

    bool search(const std::string& key) const {
        size_t index = simple_hash(key, capacity_);
        size_t start = index;
        do {
            if (state_[index] == SlotState::EMPTY) return false;
            if (state_[index] == SlotState::OCCUPIED && keys_[index] == key) {
                return true;
            }
            index = (index + 1) % capacity_;
        } while (index != start);
        return false;
    }

    bool remove(const std::string& key) {
        size_t index = simple_hash(key, capacity_);
        size_t start = index;
        do {
            if (state_[index] == SlotState::EMPTY) return false;
            if (state_[index] == SlotState::OCCUPIED && keys_[index] == key) {
                state_[index] = SlotState::DELETED;
                size_--;
                return true;
            }
            index = (index + 1) % capacity_;
        } while (index != start);
        return false;
    }

    // Resize: allocate new arrays and rehash everything -- O(n)
    void resize(size_t new_capacity) {
        std::vector<std::string> old_keys = std::move(keys_);
        std::vector<int> old_values = std::move(values_);
        std::vector<SlotState> old_state = std::move(state_);
        size_t old_capacity = capacity_;

        capacity_ = new_capacity;
        size_ = 0;
        keys_.resize(new_capacity);
        values_.resize(new_capacity, 0);
        state_.resize(new_capacity, SlotState::EMPTY);

        for (size_t i = 0; i < old_capacity; i++) {
            if (old_state[i] == SlotState::OCCUPIED) {
                insert(old_keys[i], old_values[i]);
            }
        }
    }

    size_t get_size() const { return size_; }
    size_t get_capacity() const { return capacity_; }
    double load_factor() const { return static_cast<double>(size_) / capacity_; }

private:
    enum class SlotState { EMPTY, OCCUPIED, DELETED };
    std::vector<std::string> keys_;
    std::vector<int> values_;
    std::vector<SlotState> state_;
    size_t capacity_;
    size_t size_;
};

// ── Benchmark Utilities ─────────────────────────────────────────────────────

using Clock = std::chrono::high_resolution_clock;

// Returns elapsed time in microseconds
template <typename Func>
double time_us(Func&& func) {
    auto start = Clock::now();
    func();
    auto end = Clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count();
}

void print_header(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
    std::cout << std::setw(12) << "n"
              << std::setw(15) << "time (us)"
              << std::setw(15) << "growth" << "\n";
    std::cout << std::string(42, '-') << "\n";
}

void print_row(int n, double time_us, double prev_time_us) {
    std::cout << std::setw(12) << n
              << std::setw(15) << std::fixed << std::setprecision(1) << time_us;
    if (prev_time_us > 0) {
        std::cout << std::setw(12) << std::setprecision(1) << (time_us / prev_time_us) << "x";
    }
    std::cout << "\n";
}

// ── Benchmark Result (for CSV export) ────────────────────────────────────────

struct BenchResult {
    std::string operation;
    std::string structure;
    std::string complexity;
    int n;
    double time_us;
};

// ── Main ────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "============================================================\n";
    std::cout << "  Big O Demo: Hash Tables\n";
    std::cout << "============================================================\n";
    std::cout << "\nThis demo times hash table operations at increasing sizes.\n";
    std::cout << "Watch the 'growth' column:\n";
    std::cout << "  - O(1) avg: growth stays near 1x (constant per operation)\n";
    std::cout << "  - O(n) resize: total time grows linearly with n\n";

    std::vector<int> sizes = {1000, 2500, 5000, 10000};
    std::vector<BenchResult> results;

    // ── Section 1: Chaining -- Insert, Search, Remove (low load factor) ──
    // Capacity = 2 * n  =>  load factor ~ 0.5  =>  short chains  =>  O(1)

    print_header("Chaining insert -- O(1) avg  [load factor ~0.5]");
    double prev = 0;
    for (int n : sizes) {
        auto keys = generate_keys(n);
        ChainingHashTable table(n * 2);  // load factor ~ 0.5
        double t = time_us([&]() {
            for (int i = 0; i < n; i++) table.insert(keys[i], i);
        });
        double per_op = t / n;
        print_row(n, per_op, prev);
        results.push_back({"insert", "Chaining", "O(1)", n, per_op});
        prev = per_op;
    }

    print_header("Chaining search -- O(1) avg  [load factor ~0.5]");
    prev = 0;
    for (int n : sizes) {
        auto keys = generate_keys(n);
        ChainingHashTable table(n * 2);
        for (int i = 0; i < n; i++) table.insert(keys[i], i);
        double t = time_us([&]() {
            for (int i = 0; i < n; i++) table.search(keys[i]);
        });
        double per_op = t / n;
        print_row(n, per_op, prev);
        results.push_back({"search", "Chaining", "O(1)", n, per_op});
        prev = per_op;
    }

    print_header("Chaining remove -- O(1) avg  [load factor ~0.5]");
    prev = 0;
    for (int n : sizes) {
        auto keys = generate_keys(n);
        ChainingHashTable table(n * 2);
        for (int i = 0; i < n; i++) table.insert(keys[i], i);
        double t = time_us([&]() {
            for (int i = 0; i < n; i++) table.remove(keys[i]);
        });
        double per_op = t / n;
        print_row(n, per_op, prev);
        results.push_back({"remove", "Chaining", "O(1)", n, per_op});
        prev = per_op;
    }

    // ── Section 2: Linear Probing -- Insert, Search, Remove (low load factor)
    // Capacity = 2 * n  =>  load factor ~ 0.5  =>  few probes  =>  O(1)

    print_header("Linear probing insert -- O(1) avg  [load factor ~0.5]");
    prev = 0;
    for (int n : sizes) {
        auto keys = generate_keys(n);
        LinearProbingHashTable table(n * 2);
        double t = time_us([&]() {
            for (int i = 0; i < n; i++) table.insert(keys[i], i);
        });
        double per_op = t / n;
        print_row(n, per_op, prev);
        results.push_back({"insert", "Probing", "O(1)", n, per_op});
        prev = per_op;
    }

    print_header("Linear probing search -- O(1) avg  [load factor ~0.5]");
    prev = 0;
    for (int n : sizes) {
        auto keys = generate_keys(n);
        LinearProbingHashTable table(n * 2);
        for (int i = 0; i < n; i++) table.insert(keys[i], i);
        double t = time_us([&]() {
            for (int i = 0; i < n; i++) table.search(keys[i]);
        });
        double per_op = t / n;
        print_row(n, per_op, prev);
        results.push_back({"search", "Probing", "O(1)", n, per_op});
        prev = per_op;
    }

    print_header("Linear probing remove -- O(1) avg  [load factor ~0.5]");
    prev = 0;
    for (int n : sizes) {
        auto keys = generate_keys(n);
        LinearProbingHashTable table(n * 2);
        for (int i = 0; i < n; i++) table.insert(keys[i], i);
        double t = time_us([&]() {
            for (int i = 0; i < n; i++) table.remove(keys[i]);
        });
        double per_op = t / n;
        print_row(n, per_op, prev);
        results.push_back({"remove", "Probing", "O(1)", n, per_op});
        prev = per_op;
    }

    // ── Section 3: Load Factor Impact (Chaining) ────────────────────────
    // Same n, different capacities => different load factors
    // Higher load factor = longer chains = slower operations

    std::cout << "\n============================================================\n";
    std::cout << "  Load Factor Impact on Chaining Search (n = 10000)\n";
    std::cout << "============================================================\n";
    std::cout << "\n  Higher load factor = longer chains = more comparisons\n";

    int n_load = 10000;
    auto keys_load = generate_keys(n_load);

    std::cout << "\n";
    std::cout << std::setw(18) << "load factor"
              << std::setw(15) << "capacity"
              << std::setw(18) << "search (us/op)" << "\n";
    std::cout << std::string(51, '-') << "\n";

    // - Load factor 0.5: capacity = 2n, chains avg length ~0.5
    // - Load factor 0.9: capacity = n/0.9, chains avg length ~0.9
    // - Load factor 1.5: capacity = n/1.5, chains avg length ~1.5
    // - Load factor 5.0: capacity = n/5, chains avg length ~5
    std::vector<double> load_factors = {0.5, 0.9, 1.5, 5.0};

    for (double lf : load_factors) {
        size_t capacity = static_cast<size_t>(n_load / lf);
        if (capacity < 1) capacity = 1;

        ChainingHashTable table(capacity);
        for (int i = 0; i < n_load; i++) table.insert(keys_load[i], i);

        double t = time_us([&]() {
            for (int i = 0; i < n_load; i++) table.search(keys_load[i]);
        });
        double per_op = t / n_load;

        std::cout << std::setw(18) << std::fixed << std::setprecision(1) << lf
                  << std::setw(15) << capacity
                  << std::setw(15) << std::setprecision(3) << per_op << "\n";
    }

    std::cout << "\n  Notice: as load factor increases, search time grows.\n";
    std::cout << "  At load factor 5.0, each chain averages 5 elements to scan.\n";

    // ── Section 4: Load Factor Impact (Linear Probing) ──────────────────
    // Linear probing degrades much faster than chaining at high load factors
    // because of clustering. Can't exceed load factor 1.0.

    std::cout << "\n============================================================\n";
    std::cout << "  Load Factor Impact on Linear Probing Search (n = 10000)\n";
    std::cout << "============================================================\n";
    std::cout << "\n  Linear probing clusters badly near load factor 1.0.\n";
    std::cout << "  Cannot exceed 1.0 (table is full).\n";

    std::cout << "\n";
    std::cout << std::setw(18) << "load factor"
              << std::setw(15) << "capacity"
              << std::setw(18) << "search (us/op)" << "\n";
    std::cout << std::string(51, '-') << "\n";

    std::vector<double> probe_load_factors = {0.3, 0.5, 0.7, 0.9};

    for (double lf : probe_load_factors) {
        size_t capacity = static_cast<size_t>(n_load / lf);
        if (capacity < static_cast<size_t>(n_load + 1))
            capacity = n_load + 1;

        LinearProbingHashTable table(capacity);
        for (int i = 0; i < n_load; i++) table.insert(keys_load[i], i);

        double t = time_us([&]() {
            for (int i = 0; i < n_load; i++) table.search(keys_load[i]);
        });
        double per_op = t / n_load;

        std::cout << std::setw(18) << std::fixed << std::setprecision(1) << lf
                  << std::setw(15) << capacity
                  << std::setw(15) << std::setprecision(3) << per_op << "\n";
    }

    std::cout << "\n  Notice: linear probing degrades faster than chaining.\n";
    std::cout << "  Clustering causes long probe sequences near capacity.\n";

    // ── Section 5: Resize Benchmark -- O(n) ──────────────────────────────
    // Resizing requires rehashing every element into a new array.

    print_header("Chaining resize (double capacity) -- O(n)");
    prev = 0;
    for (int n : sizes) {
        auto keys = generate_keys(n);
        ChainingHashTable table(n);
        for (int i = 0; i < n; i++) table.insert(keys[i], i);
        double t = time_us([&]() {
            table.resize(n * 2);
        });
        print_row(n, t, prev);
        results.push_back({"resize", "Chaining", "O(n)", n, t});
        prev = t;
    }

    print_header("Linear probing resize (double capacity) -- O(n)");
    prev = 0;
    for (int n : sizes) {
        auto keys = generate_keys(n);
        LinearProbingHashTable table(n * 2);  // start with room
        for (int i = 0; i < n; i++) table.insert(keys[i], i);
        double t = time_us([&]() {
            table.resize(n * 4);
        });
        print_row(n, t, prev);
        results.push_back({"resize", "Probing", "O(n)", n, t});
        prev = t;
    }

    // ── Summary ─────────────────────────────────────────────────────────

    std::cout << "\n============================================================\n";
    std::cout << "  Summary: Hash Table Big O\n";
    std::cout << "============================================================\n";
    std::cout << "\n";
    std::cout << "  Operation | Chaining         | Linear Probing   | Why\n";
    std::cout << "  ----------|------------------|------------------|------------------------\n";
    std::cout << "  insert    | O(1) avg         | O(1) avg         | Hash + append/probe\n";
    std::cout << "  search    | O(1) avg         | O(1) avg         | Hash + scan chain/probe\n";
    std::cout << "  remove    | O(1) avg         | O(1) avg         | Same pattern\n";
    std::cout << "  resize    | O(n)             | O(n)             | Must rehash everything\n";
    std::cout << "\n";
    std::cout << "  Key takeaway: Hash tables give O(1) AVERAGE case for\n";
    std::cout << "  insert/search/remove -- but only with a good load factor.\n";
    std::cout << "\n";
    std::cout << "  Load factor = n / capacity\n";
    std::cout << "    - Low  (0.5): fast -- few collisions\n";
    std::cout << "    - High (0.9): slow -- many collisions, long chains/probes\n";
    std::cout << "    - Linear probing degrades faster than chaining\n";
    std::cout << "    - Resize is O(n) -- the cost you pay to keep load factor low\n";

    // ── Write CSV and generate charts ──────────────────────────────────
    std::string repo_dir = REPO_DIR;
    std::string csv_path = repo_dir + "/results.csv";
    std::ofstream csv(csv_path);
    csv << "operation,structure,complexity,n,time_us\n";
    for (const auto& r : results) {
        csv << r.operation << "," << r.structure << "," << r.complexity
            << "," << r.n << "," << std::fixed << std::setprecision(4)
            << r.time_us << "\n";
    }
    csv.close();
    std::cout << "\n  Results written to CSV -- generating charts...\n";

    std::string cmd = "py -3 \"" + repo_dir + "/graph.py\" --graph-only";
    std::system(cmd.c_str());

    return 0;
}
