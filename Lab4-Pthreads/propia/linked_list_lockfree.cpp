#include <atomic>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <iostream>
#include <climits>

struct Node {
    int key;
    std::atomic<Node*> next;
    Node(int k) : key(k), next(nullptr) {}
};

// Lista enlazada lock-free
class LockFreeList {
    std::atomic<Node*> head;

public:
    LockFreeList() {
        head.store(new Node(INT_MIN), std::memory_order_relaxed);
    }

    Node* find_pred(int key) {
        Node* pred = head.load(std::memory_order_acquire);
        Node* curr = pred->next.load(std::memory_order_acquire);
        while (curr && curr->key < key) {
            pred = curr;
            curr = curr->next.load(std::memory_order_acquire);
        }
        return pred;
    }

    bool contains(int key) {
        Node* curr = head.load(std::memory_order_acquire)->next.load();
        while (curr && curr->key < key)
            curr = curr->next.load(std::memory_order_acquire);
        return curr && curr->key == key;
    }

    bool insert(int key) {
        while (true) {
            Node* pred = find_pred(key);
            Node* curr = pred->next.load(std::memory_order_acquire);
            if (curr && curr->key == key) return false; // duplicado

            Node* node = new Node(key);
            node->next.store(curr, std::memory_order_relaxed);

            if (pred->next.compare_exchange_strong(
                    curr, node,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire))
                return true; // éxito
            delete node;
        }
    }

    bool remove(int key) {
        while (true) {
            Node* pred = find_pred(key);
            Node* curr = pred->next.load(std::memory_order_acquire);
            if (!curr || curr->key != key) return false; // no existe

            Node* succ = curr->next.load(std::memory_order_acquire);
            if (pred->next.compare_exchange_strong(
                    curr, succ,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                return true;
            }
        }
    }

    void print_sample(int limit = 10) {
        Node* curr = head.load(std::memory_order_acquire)->next.load();
        int count = 0;
        std::cout << "[";
        while (curr && count < limit) {
            std::cout << curr->key;
            curr = curr->next.load();
            if (curr && count < limit - 1) std::cout << ", ";
            count++;
        }
        std::cout << "]\n";
    }
};


int main(int argc, char** argv) {
    int n_ops = (argc >= 2) ? std::atoi(argv[1]) : 200000;
    int T     = (argc >= 3) ? std::atoi(argv[2]) : 8;

    LockFreeList L;
    for (int i = 1; i <= 1000; ++i) L.insert(i * 2);

    auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> th;
    for (int t = 0; t < T; ++t) {
        th.emplace_back([&, t]() {
            std::mt19937 rng(1234 + t);
            std::uniform_int_distribution<int> key(1, 2000);
            std::uniform_int_distribution<int> op(0, 99);
            for (int i = 0; i < n_ops / T; ++i) {
                int k = key(rng);
                int o = op(rng);
                if (o < 50)
                    L.contains(k);
                else if (o < 75)
                    L.insert(k);
                else
                    L.remove(k);
            }
        });
    }

    for (auto& x : th) x.join();
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "linked_list_lockfree," << n_ops << "," << T << "," << ms << "\n";
    return 0;
}
