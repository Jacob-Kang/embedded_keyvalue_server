#ifndef __MY_LRUCACHE_H
#define __MY_LRUCACHE_H

#include <boost/optional.hpp>
#include <list>
#include <map>
template <class Key, class Value>
class lru_cache {
 public:
  typedef Key key_type;
  typedef Value value_type;
  typedef std::list<key_type> list_type;
  typedef std::map<key_type,
                   std::pair<value_type, typename list_type::iterator> >
      map_type;

  lru_cache() {}
  ~lru_cache() {}
  size_t size() const { return m_list.size(); }
  bool empty() const { return m_map.empty(); }
  bool contains(const key_type &key) { return m_map.find(key) != m_map.end(); }
  void insert(const key_type &key, const value_type &value) {
    typename map_type::iterator i = m_map.find(key);
    if (i == m_map.end()) {
      // insert the new item
      m_list.push_front(key);
      m_map[key] = std::make_pair(value, m_list.begin());
    }
  }

  value_type get(const key_type &key) {
    // lookup value in the cache
    typename map_type::iterator i = m_map.find(key);
    if (i == m_map.end()) {
      // value not in cache
      return NULL;
    }
    // return the value, but first update its place in the most
    // recently used list
    typename list_type::iterator j = i->second.second;
    if (j != m_list.begin()) {
      // move item to the front of the most recently used list
      m_list.erase(j);
      m_list.push_front(key);

      // update iterator in map
      j = m_list.begin();
      const value_type &value = i->second.first;
      m_map[key] = std::make_pair(value, j);

      // return the value
      return value;
    } else {
      // the item is already at the front of the most recently
      // used list so just return it
      return i->second.first;
    }
  }
  value_type evict() {
    // evict item from the end of most recently used list
    typename list_type::iterator i = --m_list.end();
    const value_type &value = m_map.find(*i)->second.first;
    // const value_type &value = m_map.find(i)->second.second;

    m_list.erase(i);
    return value;
  }
  void erase(const key_type &key) { m_map.erase(key); }
  void clear() {
    m_map.clear();
    m_list.clear();
  }

 private:
 private:
  map_type m_map;
  list_type m_list;
};

#endif