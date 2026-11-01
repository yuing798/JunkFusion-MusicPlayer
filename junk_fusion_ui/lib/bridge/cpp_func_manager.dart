import 'dart:convert';
import 'dart:ffi' as ffi;
import 'dart:io';
import 'package:junk_fusion_ui/model/song_basic_info.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';
import './native_bindings_generated.dart';

final ffi.DynamicLibrary _libPath = () {
  final libName = Platform.isWindows
      ? "JunkFusionCoordinator.dll"
      : Platform.isMacOS
      ? "JunkFusionCoordinator.dylib"
      : "JunkFusionCoordinator.so";

  return ffi.DynamicLibrary.open(
    "${AppCache.getExeDirectory()}${Platform.pathSeparator}$libName",
  );
}();

final bindings = JunkFusionDLLBindings(_libPath);

//统一实现cpp传过来的回调函数
class CppCallbackManager {
  ffi.NativeCallable<StringFuncFunction>? _errorSendcallable;
  ffi.NativeCallable<DoubleFuncFunction>? _currentPTSCallback;
  ffi.NativeCallable<IntFuncFunction>? _onPlayNextOrPreviousSong;
  ffi.NativeCallable<IntFuncFunction>? _onPlayStateSync;
  ffi.NativeCallable<StringFuncFunction>? _onAddOrUpdateSong;
  ffi.NativeCallable<Int64FuncFunction>? _onDeleteSong;

  /// 2. 注册回调到 C++
  void setupCallbacks(SongProvider songProvider, PlaybackProvider playback) {
    _errorSendcallable = ffi.NativeCallable<StringFuncFunction>.listener((
      ffi.Pointer<ffi.Char> ptr,
    ) {
      // 将 C 的 char* 转换为 Dart 的 String
      final errorMessage = UtilFunction.cPtr2String(ptr);

      print("收到来自 C++ 的错误通知: $errorMessage");

      // TODO: 在这里触发错误弹窗逻辑
    });
    _currentPTSCallback = ffi.NativeCallable<DoubleFuncFunction>.listener((
      double value,
    ) {
      playback.updateCurrentPTS(value);
    });
    _onAddOrUpdateSong = ffi.NativeCallable<StringFuncFunction>.listener((
      ffi.Pointer<ffi.Char> ptr,
    ) {
      final dartStr = UtilFunction.cPtr2String(ptr);
      final obj = SongBasicInfo.fromJson(jsonDecode(dartStr));
      songProvider.addOrUpdateSong(obj);
    });

    _onPlayNextOrPreviousSong = ffi.NativeCallable<IntFuncFunction>.listener((
      int value,
    ) {
      if (value == 1) {
        playback.playNextSong();
      } else {
        playback.playPreviousSong();
      }
    });

    _onDeleteSong = ffi.NativeCallable<Int64FuncFunction>.listener((int id) {
      songProvider.deleteSong(id);
    });
    _onPlayStateSync = ffi.NativeCallable<IntFuncFunction>.listener((
      int value,
    ) {
      playback.setPlayPauseIcon(value == 1 ? true : false);
    });

    // 将生成的函数指针传给 C++
    bindings.registerErrorSendCallback(_errorSendcallable!.nativeFunction);
    bindings.registerCurrentPTSCallback(_currentPTSCallback!.nativeFunction);
    bindings.registerOnPlayNextOrPreviousSong(
      _onPlayNextOrPreviousSong!.nativeFunction,
    );
    bindings.registerOnAddOrUpdateSong(_onAddOrUpdateSong!.nativeFunction);
    bindings.registerOnDeleteSong(_onDeleteSong!.nativeFunction);
    bindings.requestOnPlayStateSync(_onPlayStateSync!.nativeFunction);
  }

  /// 3. 清理资源
  void dispose() {
    bindings.registerErrorSendCallback(ffi.nullptr); //给cpp的函数指针先分配一个nullPtr
    bindings.registerCurrentPTSCallback(ffi.nullptr);
    bindings.registerOnAddOrUpdateSong(ffi.nullptr);
    bindings.registerOnPlayNextOrPreviousSong(ffi.nullptr);
    bindings.registerOnDeleteSong(ffi.nullptr);
    bindings.requestOnPlayStateSync(ffi.nullptr);

    // 当不再需要回调时，必须 close 掉，否则会造成内存泄漏
    _errorSendcallable?.close();
    _currentPTSCallback?.close();
    _onAddOrUpdateSong?.close();
    _onPlayNextOrPreviousSong?.close();
    _onDeleteSong?.close();
    _onPlayStateSync?.close();
  }
}
