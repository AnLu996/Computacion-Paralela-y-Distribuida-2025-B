// linked_list_lockfree.cpp
#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

struct Node {
    int key;
    std::atomic<Node*> next;
    Node(int k, Node* n=nullptr): key(k), next(n) {}
};

static inline Node* mark(Node* p){ return (Node*)((uintptr_t)p | 1ULL); }
static inline Node* unmark(Node* p){ return (Node*)((uintptr_t)p & ~1ULL); }
static inline bool  is_marked(Node* p){ return ((uintptr_t)p & 1ULL) != 0; }

struct LockFreeList {
    Node* head;
    LockFreeList(){
        // centinelas -inf y +inf
        head = new Node(INT32_MIN, new Node(INT32_MAX));
    }

    // Busca posición (pred, curr) para key, limpiando marcas intermedias
    bool find(int key, Node** out_pred, Node** out_curr){
        retry:
        Node* pred = head;
        Node* curr = unmark(pred->next.load(std::memory_order_acquire));
        while (true){
            if (!curr) break;
            Node* succ = curr->next.load(std::memory_order_acquire);
            // salta nodos marcados (eliminados lógicamente)
            if (is_marked(succ)){
                if (!pred->next.compare_exchange_strong(curr, unmark(succ),
                        std::memory_order_acq_rel, std::memory_order_acquire)){
                    goto retry; // colisión: reintentar
                }
                curr = unmark(succ);
                continue;
            }
            if (curr->key >= key){
                *out_pred = pred; *out_curr = curr;
                return (curr->key == key);
            }
            pred = curr;
            curr = unmark(succ);
        }
        *out_pred = pred; *out_curr = curr;
        return false;
    }

    bool contains(int key){
        Node* curr = head;
        while (curr && curr->key < key){
            curr = unmark(curr->next.load(std::memory_order_acquire));
        }
        return curr && curr->key == key && !is_marked(curr->next.load());
    }

    bool insert(int key){
        Node *pred, *curr;
        while (true){
            bool found = find(key, &pred, &curr);
            if (found) return false;
            Node* node = new Node(key, curr);
            if (pred->next.compare_exchange_strong(curr, node,
                    std::memory_order_acq_rel, std::memory_order_acquire)){
                return true;
            }
            delete node; // no se enlazó, liberar y reintentar
        }
    }

    bool remove(int key){
        Node *pred, *curr;
        while (true){
            bool found = find(key, &pred, &curr);
            if (!found) return false;
            Node* succ = curr->next.load(std::memory_order_acquire);
            // marcar lógico
            if (!is_marked(succ)){
                if (!curr->next.compare_exchange_strong(succ, mark(succ),
                        std::memory_order_acq_rel, std::memory_order_acquire))
                    continue; // alguien más tocó, reintentar
            }
            // ayudar a unlink físico
            if (pred->next.compare_exchange_strong(curr, unmark(succ),
                    std::memory_order_acq_rel, std::memory_order_acquire)){
                delete curr; // memoria liberada tras unlink
            }
            return true;
        }
    }

    void print(){
        Node* n = unmark(head->next.load());
        std::cout << "[";
        bool first=true;
        while (n && n->key < INT32_MAX){
            if (!is_marked(n->next.load())){
                if(!first) std::cout<<", ";
                std::cout << n->key;
                first=false;
            }
            n = unmark(n->next.load());
        }
        std::cout << "]\n";
    }
};

int main(){
    LockFreeList l;
    // demo secuencial
    l.insert(10); l.insert(5); l.insert(7); l.insert(7); // duplicado ignorado
    l.print(); // [5, 7, 10]
    std::cout << "contains(7)=" << l.contains(7) << "\n";
    l.remove(7);
    l.print(); // [5, 10]

    // demo concurrente
    LockFreeList L2;
    const int T=4, N=1000;
    std::vector<std::thread> th;
    for(int t=0;t<T;++t){
        th.emplace_back([&,t]{
            for(int i=1;i<=N;++i){
                int v = t*N + i;
                L2.insert(v);
                if(i%3==0) L2.remove(v); // algunos se eliminan
            }
        });
    }
    for(auto& x: th) x.join();
    L2.print();
    return 0;
}
