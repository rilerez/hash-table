#include <assert.h>
#include <boost/config.hpp>
#include <iostream>
#include <optional>
#include <vector>


#define FN(body) [&](auto _){return body;}

using namespace std;

// want to be sure we're going to visit every slot.
// making stride prime should do this
template <class Pred, class F>
auto probe_by_f(size_t idx, size_t pow_of_2, Pred&& stop, F f,
                const size_t stride = 3) {
  size_t bound = 1 << pow_of_2;
  for (size_t step = 0; step < bound;
       idx = (idx + f(++step) * stride) & (bound - 1))  //% by  pow of 2
    if (stop(idx)) return idx;
  return bound;
}

template <class Pred>
auto linear_probe(size_t idx, size_t pow_of_2, Pred stop) {
  return probe_by_f(idx, pow_of_2, stop, FN(_));
}
template <class Pred>
auto quad_probe(size_t idx, size_t pow_of_2, Pred stop) {
  return probe_by_f(idx, pow_of_2, stop, FN(_ * _));
}

template <class key_t, class val_t, class hasher = std::hash<key_t>>
class hashtable_insert {
  vector<bool> full;
  vector<key_t> keys;
  vector<val_t> vals;
  size_t pow_of_2;
  size_t size =0;
  float threshold;

  bool is_too_big(){return (size>= threshold*capacity());}

  void sizeup(){
    auto bigger = hashtable_insert(pow_of_2+1);
    for(int i= 0; i < capacity(); i++){
      if(full[i]){
        bigger.insert(keys[i],vals[i]);
      }
    }
    *this = move(bigger);
  }

public:
  auto hash(key_t key) const {
    return hasher{}(key);
  }

  hashtable_insert(size_t pow_of_2 = 5,float threshold = .6){
    this->pow_of_2 = pow_of_2;
    this->threshold = threshold;
    const auto cap = this->capacity();
    full.resize(cap);
    keys.resize(cap);
    vals.resize(cap);
  }

  auto capacity() const { return 1 << pow_of_2; }
  size_t clamp(size_t h) const { return h & (capacity()-1); }

  auto find_idx(key_t key, size_t hash) const {
    return quad_probe(
                      clamp(hash), pow_of_2,
                      FN(!full[_] || (full[_] && key == keys[_])));
  }
  const val_t* const get(key_t key, size_t hash) const {
    auto idx = find_idx(key, hash);
    if (idx >= capacity()) return nullptr;
    return &vals[idx];
  }
  const auto* get(key_t key) const  { return get(key, hash(key)); }
  val_t get_default(key_t key, val_t def, size_t hash) const {
    // will copies be elided?
    if(auto val = get(key,hash))
      return *val;
    return def;
  }
  auto get_default(key_t key,val_t def)  { return get_default(key,def, hash(key)); }
  // assumes key is not already in the hash table
  void insert_at(key_t key, val_t val,  size_t idx) {
    if(is_too_big()){
      sizeup();
    }
    keys[idx] = key;
    vals[idx] = val;
    assert(full[idx]==false);
    size++;
    full[idx] = true;
  }
  void insert(key_t key, val_t val, size_t hash){
    const auto idx = find_idx(key,hash);
    insert_at(key,val,idx);
  }
  void insert(key_t key, val_t val) {
    insert(key,val,hash(key));
  }
  void assoc(key_t key, val_t val){
    const auto hash = this->hash(key);
    const auto idx = find_idx(key,hash);
    if(!full[idx])
      insert_at(key,val,idx);
    vals[idx]=val;
  }

};

int main() {
  auto h = hashtable_insert<int,int>();
  for (int i = 0; i < 40; i++){
    h.assoc(i,i);
  }
  cout << h.capacity() << "\n";
  for(int i =0; i < 40; i++){
    assert(h.get_default(i,-1)==i);
  }
  for(int i = 0; i < 40; i++){
    h.assoc(i,i+1);
  }
  cout << h.capacity() << "\n";
  return 0;
}
