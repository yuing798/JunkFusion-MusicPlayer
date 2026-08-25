import 'dart:convert';
import 'dart:ffi' as ffi;
import 'package:ffi/ffi.dart';
import 'package:junk_fusion_ui/bridge/dll_invoke.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/utils/global_key_defs.dart';
import 'package:provider/provider.dart';
import './native_bindings_generated.dart';

//统一实现cpp传过来的回调函数
class CppCallbackManager {
  ffi.NativeCallable<StringFuncFunction>? _errorSendcallable;
  ffi.NativeCallable<DoubleFuncFunction>? _currentPTSCallback;
  ffi.NativeCallable<StringFuncFunction>? _timeDomainSpecInsertOverCallback;

  void _handleErrorFromCpp(ffi.Pointer<ffi.Char> strPtr) {
    // 将 C 的 char* 转换为 Dart 的 String
    final errorMessage = strPtr.cast<Utf8>().toDartString();
    bindings.freeString(strPtr); //释放cpp分配的堆内存

    print("收到来自 C++ 的错误通知: $errorMessage");

    // TODO: 在这里触发错误弹窗逻辑
  }

  /// 2. 注册回调到 C++
  void setupCallbacks() {
    // 使用 NativeCallable.listener 将 Dart 函数包装成 C 指针。
    // .listener 的巨大优势是它是线程安全的：C++ 可以在任何后台线程调用它，
    // Dart 会自动将其调度回主 Isolate 执行，非常适合 UI 更新。
    _errorSendcallable = ffi.NativeCallable<StringFuncFunction>.listener(
      _handleErrorFromCpp,
    );
    _currentPTSCallback = ffi.NativeCallable<DoubleFuncFunction>.listener((
      double value,
    ) {
      final context = navigatorKey.currentContext;
      if (context != null) {
        context.read<PlaybackProvider>().setCurrentPTS(value);
      }
    });
    _timeDomainSpecInsertOverCallback =
        ffi.NativeCallable<StringFuncFunction>.listener((
          ffi.Pointer<ffi.Char> ptr,
        ) {
          String str = ptr.cast<Utf8>().toDartString();
          bindings.freeString(ptr);
          // TODO:重型歌曲插入任务完成
          print("重型歌曲插入任务完成:文件:$str");
        });

    // 将生成的函数指针传给 C++
    bindings.registerErrorSendCallback(_errorSendcallable!.nativeFunction);
    bindings.registerCurrentPTSCallback(_currentPTSCallback!.nativeFunction);
    bindings.registerTimeDomainSpecInsertOver(
      _timeDomainSpecInsertOverCallback!.nativeFunction,
    );
  }

  /// 3. 清理资源
  void dispose() {
    bindings.registerErrorSendCallback(
      ffi.Pointer.fromAddress(0).cast(),
    ); //给cpp的函数指针先分配一个nullPtr
    bindings.registerCurrentPTSCallback(ffi.Pointer.fromAddress(0).cast());
    bindings.registerTimeDomainSpecInsertOver(
      ffi.Pointer.fromAddress(0).cast(),
    );

    // 当不再需要回调时，必须 close 掉，否则会造成内存泄漏
    _errorSendcallable?.close();
    _currentPTSCallback?.close();
    _timeDomainSpecInsertOverCallback?.close();
  }
}
