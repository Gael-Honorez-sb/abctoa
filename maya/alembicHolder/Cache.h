#pragma once

#include <memory>
#include <unordered_map>
#include <cstdint>


namespace AlembicHolder {

template <
    typename Key, typename Value,
    typename KeyHash = std::hash<Key>, typename KeyEq = std::equal_to<Key>,
    typename ValueDeleter = std::default_delete<Value>>
class Cache {
public:
    Cache(ValueDeleter value_deleter = ValueDeleter()) : m_value_deleter(value_deleter) {}

    typedef std::shared_ptr<Value> ValuePtr;
    ValuePtr get(const Key& key) const
    {
        auto it = m_map.find(key);
        if (it == m_map.end())
            return nullptr;
        return it->second.lock();
    }

    // Will take ownership of raw_ptr.
    ValuePtr put(const Key& key, Value*&& raw_ptr)
    {
        auto ptr = ValuePtr(raw_ptr,
            [this, key](Value* ptr) {
                erase(key);
                m_value_deleter(ptr);
            });
        raw_ptr = nullptr;
        m_map[key] = ptr;
        return ptr;
    }

    // Will allocate a new Value copy constructed from value.
    ValuePtr put(const Key& key, const Value& value)
    {
        return put(key, new Value(value));
    }

    void erase(const Key& key)
    {
        m_map.erase(key);
    }

private:
    typedef std::weak_ptr<Value> WPtr;
    std::unordered_map<Key, WPtr, KeyHash, KeyEq> m_map;
    ValueDeleter m_value_deleter;
};

} // namespace AlembicHolder
