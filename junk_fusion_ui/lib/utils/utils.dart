import 'package:path_provider/path_provider.dart';
import 'package:shared_preferences/shared_preferences.dart';

class AppCache {
  static late SharedPreferences frontCacheRef; //前端UI缓存
  static late String cacheDirString; //其他缓存

  //其他内容缓存都在这里
  // static final cacheDirString = () async {
  //   final cacheDir = getApplicationSupportDirectory();
  //   final cacheDirPath = await cacheDir;
  //   return cacheDirPath.path;
  // }();
}
