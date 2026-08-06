import 'package:ffi/ffi.dart';
import 'package:junk_fusion_ui/bridge/dll/dll_invoke.dart';

final cacheDir = () {
  final dirPtr = bindings.getCacheDir();
  final dirString = dirPtr.cast<Utf8>().toDartString();
  bindings.freeString(dirPtr);
  return dirString;
}();
