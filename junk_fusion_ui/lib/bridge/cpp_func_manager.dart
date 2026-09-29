import 'dart:convert';
import 'dart:ffi' as ffi;
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:junk_fusion_ui/Macro/SongInfoMacro.dart';
import 'package:junk_fusion_ui/Macro/coordinatorMacro.dart';
import 'package:junk_fusion_ui/bridge/dll_invoke.dart';
import 'package:junk_fusion_ui/model/song_info.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/utils/global_key_defs.dart';
import 'package:junk_fusion_ui/utils/utils.dart';
import 'package:junk_fusion_ui/widgets/popup_window.dart';
import 'package:provider/provider.dart';
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
  ffi.NativeCallable<StringFuncFunction>? _onOnlineGetMatedataOver;
  ffi.NativeCallable<VoidFuncFunction>? _onPlayNextSongCallback;
  // ffi.NativeCallable<StringFuncFunction>? _onGetAllSongsOver;
  ffi.NativeCallable<StringFuncFunction>? _onLightSongDataImportOver;

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
      playback.setCurrentPTS(value);
    });
    _onOnlineGetMatedataOver = ffi.NativeCallable<StringFuncFunction>.listener((
      ffi.Pointer<ffi.Char> ptr,
    ) {
      String str = UtilFunction.cPtr2String(ptr);
      final obj = jsonDecode(str) as Map<String, dynamic>;

      final songId = obj[SongInfoMacro.songId] as int;
      // final title = obj[SongInfoMacro.title] as String?;
      // final album = obj[SongInfoMacro.album] as String?;
      // final hash = obj[SongInfoMacro.hash] as String?;
      // final artists = obj[SongInfoMacro.artists] == null
      //     ? null
      //     : List<String>.from(obj[SongInfoMacro.artists] as List);

      print(
        "准备根据联网获取的数据更新歌曲元数据信息:songId:$songId ",

        // $title $album $hash ${artists?.join(" / ")},
      );

      songProvider.requestUpdateSongInfo(songId: songId);
    });

    _onPlayNextSongCallback = ffi.NativeCallable<VoidFuncFunction>.listener(() {
      playback.playNextSong();
    });

    _onLightSongDataImportOver = ffi.NativeCallable<StringFuncFunction>.listener((
      ffi.Pointer<ffi.Char> ptr,
    ) {
      final dartString = UtilFunction.cPtr2String(ptr);
      Map<String, dynamic> obj = jsonDecode(dartString);
      final songsRaw = obj[CoordinatorMacro.songsList] as List<dynamic>;
      List<SongInfo> songsList = [];
      for (int i = 0; i < songsRaw.length; i++) {
        songsList.add(SongInfo.fromJson(songsRaw[i] as Map<String, dynamic>));
      }
      songProvider.addNewSong(songsList);

      final errorFiles = obj[CoordinatorMacro.errorFiles] as List<dynamic>;

      if (errorFiles.isEmpty) {
        DialogUtil.showInfoDialog("全部歌曲导入成功，总计${songsList.length}首歌曲");
      } else {
        final buffer = StringBuffer()
          ..write(
            '歌曲导入完成，总共导入${songsList.length + errorFiles.length}首，成功${songsList.length}首\n失败文件:\n',
          );

        for (final file in errorFiles) {
          final errorLine =
              "${file[CoordinatorMacro.errorFileName] as String}:${file[CoordinatorMacro.errorFileReason] as String}";
          buffer.writeln(errorLine); // writeln 会自动加上换行符
        }

        final message = buffer.toString();
        DialogUtil.showInfoDialog(message);
      }
    });

    // 将生成的函数指针传给 C++
    bindings.registerErrorSendCallback(_errorSendcallable!.nativeFunction);
    bindings.registerCurrentPTSCallback(_currentPTSCallback!.nativeFunction);
    bindings.registerOnOnlineGetMatedataOver(
      _onOnlineGetMatedataOver!.nativeFunction,
    );
    bindings.registerOnPlayNextSong(_onPlayNextSongCallback!.nativeFunction);
    // bindings.registerOnGetAllSongsOver(_onGetAllSongsOver!.nativeFunction);
    bindings.registerOnLightSongDataImportOver(
      _onLightSongDataImportOver!.nativeFunction,
    );
  }

  /// 3. 清理资源
  void dispose() {
    bindings.registerErrorSendCallback(ffi.nullptr); //给cpp的函数指针先分配一个nullPtr
    bindings.registerCurrentPTSCallback(ffi.nullptr);
    bindings.registerOnOnlineGetMatedataOver(ffi.nullptr);
    bindings.registerOnPlayNextSong(ffi.nullptr);
    // bindings.registerOnGetAllSongsOver(ffi.nullptr);
    bindings.registerOnLightSongDataImportOver(ffi.nullptr);

    // 当不再需要回调时，必须 close 掉，否则会造成内存泄漏
    _errorSendcallable?.close();
    _currentPTSCallback?.close();
    _onOnlineGetMatedataOver?.close();
    _onPlayNextSongCallback?.close();
    // _onGetAllSongsOver?.close();
    _onLightSongDataImportOver?.close();
  }
}
