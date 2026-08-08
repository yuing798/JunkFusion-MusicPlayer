import 'package:shared_preferences/shared_preferences.dart';

class AppCache {
  // 注意 static final 修饰
  static final frontCache = SharedPreferences.getInstance();
}
