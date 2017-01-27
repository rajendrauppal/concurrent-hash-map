
#ifndef CONCURRENT_HASH_MAP_HPP
#define CONCURRENT_HASH_MAP_HPP

#include <shared_mutex>


namespace chm
{

template<typename K, typename V>
class hash_node
{
public:
    hash_node(): next_(nullptr)
    {}

    hash_node(K key, V value): key_(key), value_(value)
    {}

    ~hash_node() { this->next_ = nullptr; }

    const K& key() const { return this->key_; }

    const V& value() const { return this->value_; }

    void set_value(const V& value) { this->value_ = value; }

private:
    K key_;
    V value_;
public:
    hash_node * next_;

}; // class hash_node


template<typename K, typename V>
class hash_bucket
{
public:
    hash_bucket()
    {
        head_ = new hash_node<K, V>();
    }

    ~hash_bucket()
    {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        hash_node<K, V> * prev = nullptr;
        hash_node<K, V> * curr = head_;
        while (curr) {
            prev = curr;
            curr = curr->next_;
            delete prev;
        }
    }

    void insert(const K& key, const V& value)
        /// if the key already exists, update the value
        /// else, insert a new <key, value> pair
    {
        // acquire the lock before inserting any new key-value pair
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        hash_node<K, V> * prev = head_;
        hash_node<K, V> * curr = head_->next_;
        // search for the key, update if already exists, insert new pair otherwise
        while (curr and curr->key() != key) {
            prev = curr;
            curr = curr->next_;
        }
        // check if we have traversed complete list and the key is not found
        if (not curr) {
            // insert new <key, value> pair
            prev->next_ = new hash_node<K, V>(key, value);
        }
        else {
            // key found, update the value
            curr->set_value(value);
        }
    }

    bool find(const K& key, V& value) const
        /// find a key in the bucket
        /// if found, return true and copy value in the parameter value
        /// if not found, return false
    {
        std::shared_lock<std::shared_timed_mutex> lock(mutex_);
        hash_node<K, V> * curr = head_->next_;
        bool result = false;
        while (curr) {
            if (curr->key() == key) {
                value = curr->value();
                result = true;
                break;
            }
            curr = curr->next_;
        }
        return result;
    }

    void remove(const K& key)
        /// removes a <key, value> pair from the bucket if key is found.
    {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        hash_node<K, V> * prev = head_;
        hash_node<K, V> * curr = head_->next_;
        while (curr and curr->key() != key) {
            prev = curr;
            curr = curr->next_;
        }
        if (curr) {
            prev->next_ = curr->next_;
            delete curr;
        }
        // if key not found, do nothing
    }

    void clear()
        /// remove all nodes except head node
    {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        hash_node<K, V> * prev = head_;
        hash_node<K, V> * curr = head_->next_;
        while (curr) {
            prev->next_ = curr->next_;
            delete curr;
            curr = prev->next_;
        }
    }

private:
    hash_node<K, V> * head_;
    mutable std::shared_timed_mutex mutex_;

}; // class hash_bucket


template<typename K, typename V, typename F = std::hash<K>>
class hash_map
{
public:
    hash_map(size_t hash_size = hash_size_default): hash_size_(hash_size)
    {
        hash_table_ = new hash_bucket<K, V>*[hash_size_]();
        for (int i = 0; i < hash_size_; ++i)
            hash_table_[i] = new hash_bucket<K, V>();
    }

    ~hash_map()
    {
        for (int i = 0; i < hash_size_; ++i)
            delete hash_table_[i];
        delete [] hash_table_;
    }

    // copy, move and assignments are not supported
    hash_map(const hash_map&) = delete;
    hash_map(hash_map&&) = delete;
    hash_map& operator=(const hash_map&) = delete;
    hash_map& operator=(hash_map&&) = delete;

    void insert(const K& key, const V& value)
    {
        size_t hash_value = hash_func_(key) % hash_size_;
        hash_table_[hash_value]->insert(key, value);
    }

    bool find(const K& key, V& value) const
    {
        size_t hash_value = hash_func_(key) % hash_size_;
        return hash_table_[hash_value]->find(key, value);
    }

    void remove(const K& key)
    {
        size_t hash_value = hash_func_(key) % hash_size_;
        hash_table_[hash_value]->remove(key);
    }

    void clear()
    {
        for (int i = 0; i < hash_size_; ++i)
            (hash_table_[i])->clear();
    }

private:
    hash_bucket<K, V> ** hash_table_;
    F hash_func_;
    static constexpr size_t hash_size_default = 1031;
    size_t hash_size_;

}; // class hash_map

} // namespace chm

#endif // CONCURRENT_HASH_MAP_HPP
