#include <optional>
#include <unordered_map>
#include <list>
#include <utility>
#include <stdexcept>

template <typename Key, typename Value>
class LRUCachePool {
public:

    explicit LRUCachePool(size_t c) : capacity(c) {
        if (c == 0) throw std::invalid_argument("capacity must be > 0");
    }

    // 获取值，若不存在则返回默认构造的 Value（要求 Value 有默认构造函数）
    std::optional<Value&> get(const Key& key) {
        auto it = cacheMap.find(key);
        if (it == cacheMap.end()) {
            return std::nullopt;
        }
        // 将节点移至链表头部（最近使用）
        cacheList.splice(cacheList.begin(), cacheList, it->second);
        return it->second->second;
    }

    // 插入或更新键值对
    void put(const Key& key, const Value& value) {
        auto it = cacheMap.find(key);
        if (it != cacheMap.end()) {
            // 更新已有节点
            it->second->second = value;
            cacheList.splice(cacheList.begin(), cacheList, it->second);
            // std::list::splice 是“无痛搬家”——它可以把一个 std::list 中的节点（Node），
            // 零拷贝、零移动地“剪贴”到另一个 std::list 中（或者同一个列表的不同位置）
            //splice(pos, other, it)	把 other 列表里的 单个元素（it指向的）移到 pos 之前。
            return;
        }

        // 若缓存已满，淘汰尾部节点
        if (cacheMap.size() >= capacity) {
            auto last = std::prev(cacheList.end());
            cacheMap.erase(last->first);
            cacheList.pop_back();
        }

        // 插入新节点到头部
        cacheList.emplace_front(key, value);
        cacheMap[key] = cacheList.begin();
    }

    // 可选：获取当前大小
    size_t size() const { return cacheMap.size(); }

private:
    size_t capacity;
    std::list<std::pair<Key, Value>> cacheList;          // 双向链表
    std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator> cacheMap;      // 键 → 链表迭代器
    //因为 it->second 是迭代器，指向 cacheList 中的一个节点，
    // 该节点是一个 std::pair<Key, Value>，所以 it->second->second 就是该节点存储的 Value 值。
};