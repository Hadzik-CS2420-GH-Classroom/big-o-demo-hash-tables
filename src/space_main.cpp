// ============================================================================
// Big O Demo: Hash Tables -- Space Complexity
// ============================================================================
// This demo measures the MEMORY used by hash table operations so you can
// SEE O(n) space growth and how collision strategy and load factor affect it.
//
// Two collision strategies are compared:
//   - Chaining: each bucket is a linked list (extra pointer overhead per node)
//   - Linear probing: flat parallel arrays (no per-element overhead)
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
// Same hash used in the time-complexity demo for consistency.

size_t simple_hash(const std::string& key, size_t capacity) {
    size_t sum = 0;
    for (char c : key) {
        sum += static_cast<size_t>(c);
    }
    return sum % capacity;
}

// ── Generate Keys ───────────────────────────────────────────────────────────

std::vector<std::string> generate_keys(int count) {
    std::vector<std::string> keys;
    keys.reserve(count);
    for (int i = 0; i < count; i++) {
        keys.push_back("key_" + std::to_string(i));
    }
    return keys;
}

// ── Chaining Hash Table (with memory tracking) ─────────────────────────────

class ChainingHashTable {
public:
    explicit ChainingHashTable(size_t capacity)
        : buckets_(capacity), size_{0}, capacity_{capacity} {}

    void insert(const std::string& key, int value) {
        size_t index = simple_hash(key, capacity_);
        for (auto& [k, v] : buckets_[index]) {
            if (k == key) {
                v = value;
                return;
            }
        }
        buckets_[index].emplace_back(key, value);
        size_++;
    }

    // Estimated memory in bytes:
    //   - Bucket array: capacity * sizeof(std::list)
    //   - Each element: sizeof(list node) + key storage
    //   A std::list node has prev/next pointers + the stored pair
    size_t estimate_memory_bytes() const {
        // Base: vector of lists
        size_t mem = capacity_ * sizeof(std::list<std::pair<std::string, int>>);

        // Each inserted element: list node overhead + key heap storage
        // list node ~ 2 pointers + pair(string, int)
        // string has ~32 bytes base (SSO buffer) on MSVC + heap if > 15 chars
        size_t node_overhead = 2 * sizeof(void*);  // prev + next pointers
        size_t pair_size = sizeof(std::pair<std::string, int>);

        for (const auto& chain : buckets_) {
            for (const auto& [k, v] : chain) {
                mem += node_overhead + pair_size;
                // If key exceeds SSO (typically 15 chars), add heap allocation
                if (k.size() > 15) {
                    mem += k.size() + 1;
                }
            }
        }
        return mem;
    }

    size_t get_size() const { return size_; }
    size_t get_capacity() const { return capacity_; }

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

private:
    std::vector<std::list<std::pair<std::string, int>>> buckets_;
    size_t size_;
    size_t capacity_;
};

// ── Linear Probing Hash Table (with memory tracking) ────────────────────────

class LinearProbingHashTable {
public:
    explicit LinearProbingHashTable(size_t capacity)
        : capacity_{capacity}, size_{0} {
        keys_.resize(capacity);
        values_.resize(capacity, 0);
        state_.resize(capacity, SlotState::EMPTY);
    }

    void insert(const std::string& key, int value) {
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
                values_[index] = value;
                return;
            }
            index = (index + 1) % capacity_;
        } while (index != start);
    }

    // Estimated memory in bytes:
    //   - Three parallel arrays: keys (strings), values (ints), state (enum)
    //   - Every slot is allocated whether occupied or not
    size_t estimate_memory_bytes() const {
        size_t mem = 0;

        // keys_ vector: capacity * sizeof(string) -- every slot has a string object
        mem += capacity_ * sizeof(std::string);

        // values_ vector: capacity * sizeof(int)
        mem += capacity_ * sizeof(int);

        // state_ vector: capacity * sizeof(SlotState)
        mem += capacity_ * sizeof(SlotState);

        // Heap storage for keys that exceed SSO
        for (size_t i = 0; i < capacity_; i++) {
            if (state_[i] == SlotState::OCCUPIED && keys_[i].size() > 15) {
                mem += keys_[i].size() + 1;
            }
        }

        return mem;
    }

    size_t get_size() const { return size_; }
    size_t get_capacity() const { return capacity_; }

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

private:
    enum class SlotState : uint8_t { EMPTY, OCCUPIED, DELETED };
    std::vector<std::string> keys_;
    std::vector<int> values_;
    std::vector<SlotState> state_;
    size_t capacity_;
    size_t size_;
};

// ── Benchmark Utilities ─────────────────────────────────────────────────────

void print_header(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
    std::cout << std::setw(12) << "n"
              << std::setw(18) << "memory (KB)"
              << std::setw(15) << "growth" << "\n";
    std::cout << std::string(45, '-') << "\n";
}

void print_row(int n, double mem_kb, double prev_kb) {
    std::cout << std::setw(12) << n
              << std::setw(18) << std::fixed << std::setprecision(1) << mem_kb;
    if (prev_kb > 0) {
        std::cout << std::setw(12) << std::setprecision(1) << (mem_kb / prev_kb) << "x";
    }
    std::cout << "\n";
}

// ── Benchmark Result (for CSV export) ────────────────────────────────────────

struct BenchResult {
    std::string measurement;
    std::string structure;
    std::string complexity;
    int n;
    double memory_kb;
};

// ── Main ────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "============================================================\n";
    std::cout << "  Big O Demo: Hash Tables -- Space Complexity\n";
    std::cout << "============================================================\n";
    std::cout << "\nThis demo measures memory usage at increasing input sizes.\n";
    std::cout << "Watch the 'growth' column:\n";
    std::cout << "  - O(n): memory grows linearly (doubling n doubles memory)\n";
    std::cout << "  - Growth ratio near 2x when n doubles confirms O(n)\n";

    std::vector<int> sizes = {1000, 2000, 5000, 10000};
    std::vector<BenchResult> results;

    // ── Section 1: Memory Growth -- O(n) ────────────────────────────────
    // Both structures use O(n) space. Insert n elements, measure memory.

    print_header("Chaining memory usage -- O(n)  [load factor ~0.5]");
    double prev = 0;
    for (int n : sizes) {
        auto keys = generate_keys(n);
        ChainingHashTable table(n * 2);
        for (int i = 0; i < n; i++) table.insert(keys[i], i);
        double mem_kb = table.estimate_memory_bytes() / 1024.0;
        print_row(n, mem_kb, prev);
        results.push_back({"memory", "Chaining", "O(n)", n, mem_kb});
        prev = mem_kb;
    }

    print_header("Linear probing memory usage -- O(n)  [load factor ~0.5]");
    prev = 0;
    for (int n : sizes) {
        auto keys = generate_keys(n);
        LinearProbingHashTable table(n * 2);
        for (int i = 0; i < n; i++) table.insert(keys[i], i);
        double mem_kb = table.estimate_memory_bytes() / 1024.0;
        print_row(n, mem_kb, prev);
        results.push_back({"memory", "Probing", "O(n)", n, mem_kb});
        prev = mem_kb;
    }

    // ── Section 2: Chaining vs Probing Overhead ─────────────────────────
    // Same n, same load factor -- compare how much memory each uses.

    std::cout << "\n============================================================\n";
    std::cout << "  Memory Comparison: Chaining vs Linear Probing (n = 10000)\n";
    std::cout << "============================================================\n";
    std::cout << "\n  Same data, different overhead per element.\n";

    int n_cmp = 10000;
    auto keys_cmp = generate_keys(n_cmp);

    ChainingHashTable chain_cmp(n_cmp * 2);
    LinearProbingHashTable probe_cmp(n_cmp * 2);
    for (int i = 0; i < n_cmp; i++) {
        chain_cmp.insert(keys_cmp[i], i);
        probe_cmp.insert(keys_cmp[i], i);
    }

    double chain_kb = chain_cmp.estimate_memory_bytes() / 1024.0;
    double probe_kb = probe_cmp.estimate_memory_bytes() / 1024.0;

    std::cout << "\n";
    std::cout << std::setw(20) << "structure"
              << std::setw(18) << "memory (KB)"
              << std::setw(18) << "bytes/element" << "\n";
    std::cout << std::string(56, '-') << "\n";
    std::cout << std::setw(20) << "Chaining"
              << std::setw(18) << std::fixed << std::setprecision(1) << chain_kb
              << std::setw(18) << std::setprecision(1)
              << (chain_cmp.estimate_memory_bytes() / static_cast<double>(n_cmp)) << "\n";
    std::cout << std::setw(20) << "Linear Probing"
              << std::setw(18) << std::fixed << std::setprecision(1) << probe_kb
              << std::setw(18) << std::setprecision(1)
              << (probe_cmp.estimate_memory_bytes() / static_cast<double>(n_cmp)) << "\n";

    std::cout << "\n  Chaining: extra prev/next pointers per list node\n";
    std::cout << "  Probing:  flat arrays, but every slot is allocated\n";

    // ── Section 3: Load Factor Impact on Memory ─────────────────────────
    // Lower load factor = more capacity = more wasted space

    std::cout << "\n============================================================\n";
    std::cout << "  Load Factor vs Memory Usage (n = 10000)\n";
    std::cout << "============================================================\n";
    std::cout << "\n  Lower load factor = more empty buckets = more wasted memory.\n";
    std::cout << "  This is the space-time tradeoff.\n";

    int n_load = 10000;
    auto keys_load = generate_keys(n_load);

    std::cout << "\n";
    std::cout << std::setw(18) << "load factor"
              << std::setw(15) << "capacity"
              << std::setw(18) << "memory (KB)"
              << std::setw(18) << "bytes/element" << "\n";
    std::cout << std::string(69, '-') << "\n";

    std::vector<double> load_factors = {0.3, 0.5, 0.7, 0.9};

    for (double lf : load_factors) {
        size_t capacity = static_cast<size_t>(n_load / lf);
        if (capacity < 1) capacity = 1;

        ChainingHashTable table(capacity);
        for (int i = 0; i < n_load; i++) table.insert(keys_load[i], i);

        double mem_kb = table.estimate_memory_bytes() / 1024.0;
        double bytes_per = table.estimate_memory_bytes() / static_cast<double>(n_load);

        std::cout << std::setw(18) << std::fixed << std::setprecision(1) << lf
                  << std::setw(15) << capacity
                  << std::setw(18) << std::setprecision(1) << mem_kb
                  << std::setw(18) << std::setprecision(1) << bytes_per << "\n";

        results.push_back({"load_factor_memory", "Chaining",
                           "LF=" + std::to_string(lf).substr(0, 3), n_load, mem_kb});
    }

    std::cout << "\n  Notice: lower load factor uses MORE memory for the same data.\n";
    std::cout << "  You trade memory for faster lookups.\n";

    // ── Section 4: Resize Memory Spike ──────────────────────────────────
    // During resize, both old and new arrays exist briefly -- 2x memory.

    std::cout << "\n============================================================\n";
    std::cout << "  Resize Memory Spike\n";
    std::cout << "============================================================\n";
    std::cout << "\n  During resize, the old and new arrays coexist briefly.\n";
    std::cout << "  This causes a temporary memory spike.\n\n";

    int n_resize = 10000;
    auto keys_resize = generate_keys(n_resize);

    ChainingHashTable resize_table(n_resize);
    for (int i = 0; i < n_resize; i++) resize_table.insert(keys_resize[i], i);

    double before_kb = resize_table.estimate_memory_bytes() / 1024.0;
    // The spike is: old capacity + new capacity worth of bucket array
    double spike_kb = before_kb +
        (n_resize * 2) * sizeof(std::list<std::pair<std::string, int>>) / 1024.0;

    resize_table.resize(n_resize * 2);
    double after_kb = resize_table.estimate_memory_bytes() / 1024.0;

    std::cout << std::setw(25) << "before resize:"
              << std::setw(12) << std::fixed << std::setprecision(1)
              << before_kb << " KB\n";
    std::cout << std::setw(25) << "during resize (est):"
              << std::setw(12) << std::fixed << std::setprecision(1)
              << spike_kb << " KB\n";
    std::cout << std::setw(25) << "after resize:"
              << std::setw(12) << std::fixed << std::setprecision(1)
              << after_kb << " KB\n";
    std::cout << "\n  The spike is ~"
              << std::setprecision(1) << (spike_kb / before_kb)
              << "x the pre-resize memory.\n";

    // ── Summary ─────────────────────────────────────────────────────────

    std::cout << "\n============================================================\n";
    std::cout << "  Summary: Hash Table Space Complexity\n";
    std::cout << "============================================================\n";
    std::cout << "\n";
    std::cout << "  Both chaining and linear probing use O(n) space.\n";
    std::cout << "\n";
    std::cout << "  Structure      | Space      | Why\n";
    std::cout << "  ---------------|------------|--------------------------------------\n";
    std::cout << "  Chaining       | O(n + m)   | n elements + m buckets + node ptrs\n";
    std::cout << "  Linear Probing | O(m)       | m slots (must be >= n), flat arrays\n";
    std::cout << "\n";
    std::cout << "  Key takeaways:\n";
    std::cout << "    - Memory grows linearly with the number of elements\n";
    std::cout << "    - Chaining has per-node pointer overhead (prev + next)\n";
    std::cout << "    - Probing allocates all slots upfront (empty slots waste space)\n";
    std::cout << "    - Lower load factor = faster ops but MORE memory used\n";
    std::cout << "    - Resize causes a temporary ~2x memory spike\n";

    // ── Write CSV ───────────────────────────────────────────────────────
    std::string repo_dir = REPO_DIR;
    std::string csv_path = repo_dir + "/results_space.csv";
    std::ofstream csv(csv_path);
    csv << "measurement,structure,complexity,n,memory_kb\n";
    for (const auto& r : results) {
        csv << r.measurement << "," << r.structure << "," << r.complexity
            << "," << r.n << "," << std::fixed << std::setprecision(4)
            << r.memory_kb << "\n";
    }
    csv.close();
    std::cout << "\n  Results written to CSV -- generating charts...\n";

    std::string cmd = "py -3 \"" + repo_dir + "/graph_space.py\" --graph-only";
    std::system(cmd.c_str());

    return 0;
}
