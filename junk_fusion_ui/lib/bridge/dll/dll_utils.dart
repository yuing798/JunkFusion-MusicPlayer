import 'dart:convert';
import 'dart:ffi';
import 'dart:io';
import 'dart:async';
import 'dart:isolate';

import 'package:ffi/ffi.dart';
import 'package:freezed_annotation/freezed_annotation.dart';
import 'package:junk_fusion_ui/bridge/dll/dllBridgeName.dart';
import 'package:junk_fusion_ui/bridge/dll/native_bindings_generated.dart';
import 'package:junk_fusion_ui/model/song_info.dart';

final DynamicLibrary _lib = () {
  final libName = Platform.isWindows
      ? "JunkFusionDLL.dll"
      : Platform.isMacOS
      ? "JunkFusionDLL.dylib"
      : "JunkFusionDLL.so";

  return DynamicLibrary.open(libName);
}();

final bindings = JunkFusionDLLBindings(_lib);

Future<int> sumAsync(int a, int b) async {
  final SendPort helperIsolateSendPort = await _isolateSendPort;
  final int requestId = _nextSumRequestId++;
  final request = _SumRequest(requestId, a, b);
  final completer = Completer<int>();
  _sumRequests[requestId] = completer;
  helperIsolateSendPort.send(request); //发送一个类实例作为请求
  return completer.future;
}

/// A request to compute `sum`.
///
/// Typically sent from one isolate to another.
class _TaskRequest {
  final String funcName; //函数名称
  final Map<String, Object?> params; //传参列表
  //Object：代表 “所有非空类型（Non-nullable）的基类”

  const _TaskRequest(this.funcName, this.params); //required只能使用在命名参数里面
}

class _TaskResponse {
  final String funcName; //函数名称
  final Map<String, Object?> results;

  const _TaskResponse(this.funcName, this.results);
}

Future<SendPort> _isolateSendPort = () async {
  final completer = Completer<SendPort>();

  // Receive port on the main isolate to receive messages from the helper.
  // We receive two types of messages:
  // 1. A port to send messages on.
  // 2. Responses to requests we sent.
  final receivePort = ReceivePort()
    ..listen((dynamic data) {
      if (data is SendPort) {
        // The helper isolate sent us the port on which we can sent it requests.
        completer.complete(data);
        return;
      }
      if (data is _SumResponse) {
        // The helper isolate sent us a response to a request we sent.
        final Completer<int> completer = _sumRequests[data.id]!;
        _sumRequests.remove(data.id);
        completer.complete(data.result);
        return;
      }
      throw UnsupportedError('Unsupported message type: ${data.runtimeType}');
    });

  // Start the helper isolate.
  await Isolate.spawn((SendPort sendPort) async {
    final ReceivePort helperReceivePort = ReceivePort()
      ..listen((dynamic data) {
        // On the helper isolate listen to requests and respond to them.
        if (data is _TaskRequest) {
          final name = data.funcName;
          final params = data.params;
          Map<String, Object?> results = {}; //Map为空和NULL是两种东西
          if (name == B_getAllSongs.name) {
            final cPtr = bindings.getAllSongs();
            final dartString = cPtr.cast<Utf8>().toDartString(); //解码
            Map<String, dynamic> obj = jsonDecode(dartString);
            //Object? 是“类型安全的未知类型”（你暂时不知道它是什么，但编译器会管着你）；
            //dynamic 是“彻底关闭类型检查的万能类型”（你爱怎么用就怎么用，编译器完全听你的，出错了运行时才报错）。
            bindings.freeString(cPtr);
            assert(obj.containsKey(B_getAllSongs.songsList));
            final songs =
                obj[B_getAllSongs.songsList] as List<Map<String, dynamic>>;
            List<SongInfo> songsList = [];
            for (int i = 0; i < songs.length; i++) {
              final song = songs[i];
              songsList[i] = SongInfo.fromJson(song);
            }
            results[B_getAllSongs.songsList] = songsList;
          } else if (name == B_getAllSongCount.name) {
            final count = bindings.getAllSongCount();
            results[B_getAllSongCount.count] = count;
          } else if (name == B_dbInit.name) {
            bindings.dbInit();
          } else if (name == B_toggleMyLike.name) {
            assert(params.containsKey(B_toggleMyLike.songId));
            final songId = params[B_toggleMyLike.songId] as int; // 强制转换为 int
            bool result = bindings.toggleMyLike(songId) == 1;
            results[B_toggleMyLike.successOrError] = result;
          } else if (name == B_saveComment.name) {
            assert(
              params.containsKey(B_saveComment.songId) &&
                  params.containsKey(B_saveComment.text),
            );
            final songId = params[B_saveComment.songId] as int;
            final commentText = params[B_saveComment.text] as String;
            final cPtr = commentText.toNativeUtf8().cast<Char>();
            bindings.saveComment(songId, cPtr);
            calloc.free(
              cPtr,
            ); //因为这个c指针是dart的内存管理器分配在堆上面的，所以需要使用dart的calloc.free释放内存
          }
          sendPort.send(_TaskResponse(name, results));
        }
        throw UnsupportedError('Unsupported message type: ${data.runtimeType}');
      });

    // Send the port to the main isolate on which we can receive requests.
    sendPort.send(helperReceivePort.sendPort); //建立主副隔离区的双向通信
  }, receivePort.sendPort);
  //第一个回调函数只会在第一次初始化的时候执行一次
  //Isolate.spawn参数：(入口函数, 初始消息);

  return completer.future;
}();
