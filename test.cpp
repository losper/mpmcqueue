#include <iostream>
#include <atomic>
#include <thread>
#include <memory>
#include <vector>

// 节点结构
struct Node {
    size_t thread_id;
    std::atomic<Node*> next;

    Node(size_t id) : thread_id(id), next(nullptr) {}
};

// 无锁链表类
class LockFreeList {
public:
    LockFreeList() : head(new Node(0)) {}  // 头节点初始化为0

    // 查找或创建节点
    Node* get_or_create(size_t thread_id) {
        Node* current = head.load();
        while (current != nullptr) {
            if (current->thread_id == thread_id) {
                return current;
            }
            current = current->next.load();
        }

        // 创建新节点并插入链表
        Node* new_node = new Node(thread_id);
        Node* expected = nullptr;
        if (tail.compare_exchange_strong(expected, new_node)) {
            head.load()->next.store(new_node);
        } else {
            delete new_node;
        }

        return new_node;
    }

private:
    std::atomic<Node*> head;
    std::atomic<Node*> tail{nullptr};
};

// 每个线程的唯一ID
thread_local static size_t thread_id;

// 线程ID累加器
std::atomic<size_t> id_counter{0};

// 线程工作函数
void thread_function(LockFreeList& list) {
    thread_id = id_counter.fetch_add(1);
    Node* node = list.get_or_create(thread_id);
    std::cout << "Thread ID: " << thread_id << ", Node: " << node << std::endl;
}

int main() {
    LockFreeList list;
    const size_t num_threads = 10;
    std::vector<std::thread> threads;

    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_function, std::ref(list));
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
