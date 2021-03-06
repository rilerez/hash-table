#include <assert.h>
#include <vector>

#define FN(...) [&](auto _) { return __VA_ARGS__; }

namespace hashtable {
using namespace std;

// want to be sure we're going to visit every slot.
// making stride prime should do this
template <class Pred, class F>
auto probe_by_f(size_t idx, size_t pow_of_2, Pred&& stop, F f,
                const size_t stride = 3) {
  size_t bound = 1 << pow_of_2;
  for(size_t step = 0; step < bound;
      idx = (idx + f(++step) * stride) & (bound - 1))  //% by  pow of 2
    if(stop(idx)) return idx;
  return bound;
}

template <class Pred>
auto linear_probe(size_t idx, size_t pow_of_2, Pred stop,
                  const size_t stride = 3) {
  return probe_by_f(idx, pow_of_2, stop, FN(_), stride);
}
template <class Pred>
auto quad_probe(size_t idx, size_t pow_of_2, Pred stop,
                const size_t stride = 3) {
  return probe_by_f(idx, pow_of_2, stop, FN(_ * _), stride);
}

template <class key_t, class val_t, class hasher = std::hash<key_t>>
class hashtable_insert {
  vector<bool> full;
  vector<key_t> keys;
  vector<val_t> vals;
  size_t pow_of_2;
  size_t size = 0;
  float threshold;

  bool is_too_big() { return (size >= threshold * capacity()); }

  void sizeup() {
    auto bigger = hashtable_insert(pow_of_2 + 1);
    for(int i = 0; i < capacity(); i++) {
      if(full[i]) {
        bigger.insert(keys[i], vals[i]);
      }
    }
    *this = move(bigger);
  }

  void insert_at(key_t key, val_t val, size_t idx) {
    if(is_too_big()) {
      sizeup();
      insert(key,val);
      return;
    }
    keys[idx] = key;
    vals[idx] = val;
    assert(full[idx] == false);
    size++;
    full[idx] = true;
    }

 public:
  auto hash(key_t key) const { return hasher{}(key); }

  hashtable_insert(size_t pow_of_2 = 5, float threshold = .6) {
    this->pow_of_2 = pow_of_2;
    this->threshold = threshold;
    const auto cap = this->capacity();
    full.resize(cap);
    keys.resize(cap);
    vals.resize(cap);
  }

  auto capacity() const { return 1 << pow_of_2; }
  size_t clamp(size_t h) const { return h & (capacity() - 1); }

  auto find_idx(key_t key, size_t hash) const {
    return quad_probe(clamp(hash), pow_of_2,
                      FN(!full[_] || (full[_] && key == keys[_])));
  }

  const val_t* const get(key_t key, size_t hash) const {
    auto idx = find_idx(key, hash);
    if(idx >= capacity()) return nullptr;
    return &vals[idx];
  }
  const auto* get(key_t key) const { return get(key, hash(key)); }

  val_t get_default(key_t key, val_t def, size_t hash) const {
    // will copies be elided?
    if(auto val = get(key, hash)) return *val;
    return def;
  }
  auto get_default(key_t key, val_t def) {
    return get_default(key, def, hash(key));
  }

  // assumes key is not already in the hash table
  // in debug mode, check if this assumption is true
  // in production mode, you're on your own
  void insert(key_t key, val_t val, size_t hash) {
    insert_at(key, val, find_idx(key, hash));
  }
  void insert(key_t key, val_t val) { insert(key, val, hash(key)); }
  // update or insert
  void assoc(key_t key, val_t val, size_t hash) {
    const auto idx = find_idx(key, hash);
    if(!full[idx]) insert_at(key, val, idx);
    vals[idx] = val;
  }
  void assoc(key_t key, val_t val) { assoc(key, val, hash(key)); }

  template <class F>
  void update_with_default(key_t key, val_t def, F update) {
    auto hash = this->hash(key);
    assoc(key, update(get_default(key, def)));
  }

  class iterator {
  private:
    hashtable_insert* the_table;
    size_t offset;
  public:
    using difference_type = ptrdiff_t;
    using value_type = val_t;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator_category = std::bidirectional_iterator_tag;

    pair<key_t&,val_t&> operator*(){
      assert (the_table->full[offset]);
      return {the_table->keys[offset],
              the_table->vals[offset]};
    }

    auto* operator->(){
      return &(*(*this));
    }

    auto operator++(){
      const auto& full = the_table->full;
      offset = find(++full.begin()+offset, full.end(), true) - full.begin();
    }

    auto operator--(){
      const auto& full = the_table->full;
      offset = find(++make_reverse_iterator(full.begin()+offset),
                    full.rend(),
                    true)
        - full.begin();
    }

    iterator operator++(int){
      auto it = *this;
      (*this)++;
      return it;
    }

    iterator operator--(int){
      auto it = *this;
      (*this)--;
      return it;
    }

    bool operator==(const iterator& other){
      return offset==other.offset;
    }

    bool operator!=(const iterator& other){
      return offset!=other.offset;
    }

    iterator(hashtable_insert* table, size_t offset)
      : the_table{table}, offset{offset}
    {}
  };

  iterator begin(){
    return iterator(this,0);
  }

  iterator end(){
    return iterator(this,full.size());
  }
};

#undef FN
}  // namespace hashtable
