import 'dart:collection';

class LruCache<K, V> {
  final int capacity;
  // LinkedHashMap 会保持插入顺序
  final LinkedHashMap<K, V> _map = LinkedHashMap<K, V>();

  LruCache(this.capacity) {
    if (capacity <= 0) {
      throw ArgumentError('Capacity must be greater than 0');
    }
  }

  /// 获取缓存
  V? get(K key) {
    if (!_map.containsKey(key)) return null;

    // 命中缓存：删掉旧位置，重新插入到尾部（标记为最近使用）
    final value = _map.remove(key) as V;
    _map[key] = value;
    return value;
  }

  /// 写入缓存
  void put(K key, V value) {
    if (_map.containsKey(key)) {
      // 如果已存在，先删掉旧的
      _map.remove(key);
    } else if (_map.length >= capacity) {
      // 容量满了，淘汰头部（最久未使用）的元素
      final oldestKey = _map.keys.first;
      _map.remove(oldestKey);
    }
    // 插入到尾部
    _map[key] = value;
  }

  /// 检查是否存在（不改变使用顺序）
  bool containsKey(K key) => _map.containsKey(key);

  /// 移除指定缓存
  void remove(K key) => _map.remove(key);

  /// 清空缓存
  void clear() => _map.clear();

  /// 当前缓存大小
  int get length => _map.length;
}
