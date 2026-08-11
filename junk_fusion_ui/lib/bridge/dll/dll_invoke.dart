import 'dart:convert';
import 'dart:ffi';
import 'dart:io';
import 'dart:async';
import 'dart:isolate';

import 'package:ffi/ffi.dart';
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

Future<Map<String, Object?>> sendDLLIsolateTask(
  String funcName,
  Map<String, dynamic> params,
) async {
  final completer = Completer<Map<String, Object?>>();
  final id = _nextRequestId++;

  final port = await isolateSendPort;
  // 把 Completer 存起来，等待响应回来时被取走
  _pendingRequests[id] = completer;

  // 发送带 ID 的请求给辅助隔离区
  port.send(_TaskRequest(id, funcName, params));

  // 把 Future 返回给外部调用者
  return completer.future;
}

class _TaskRequest {
  final int id;
  final String funcName; //函数名称
  final Map<String, Object?> params; //传参列表
  //Object：代表 “所有非空类型（Non-nullable）的基类”

  const _TaskRequest(this.id, this.funcName, this.params); //required只能使用在命名参数里面
}

class _TaskResponse {
  final int id;
  final Map<String, Object?> results;

  const _TaskResponse(this.id, this.results);
}

final Map<int, Completer<Map<String, Object?>>> _pendingRequests = {};
int _nextRequestId = 0;

Future<SendPort> isolateSendPort = () async {
  final initCompleter = Completer<SendPort>();

  // Receive port on the main isolate to receive messages from the helper.
  // We receive two types of messages:
  // 1. A port to send messages on.
  // 2. Responses to requests we sent.
  final receivePort = ReceivePort()
    ..listen((dynamic data) {
      if (data is SendPort) {
        // The helper isolate sent us the port on which we can sent it requests.
        initCompleter.complete(data);
        return;
      }
      if (data is _TaskResponse) {
        // 把辅助隔离区中的异常打印到终端（否则静默崩溃无法调试）
        // print("接收到回复消息");
        // if (data.results.containsKey('error')) {
        //   print('❌ DLL Isolate 错误: ${data.results['error']}');
        //   print('堆栈: ${data.results['stack']}');
        // }
        // 根据响应中的 id 取出对应的 Completer
        final completer = _pendingRequests.remove(data.id);
        if (completer != null && !completer.isCompleted) {
          // 这里就把结果“发送到外部”了——因为 completer.future 正被外部 await
          completer.complete(data.results);
        }
        return;
      }
      throw UnsupportedError('Unsupported message type: ${data.runtimeType}');
    });

  // Start the helper isolate.
  await Isolate.spawn((SendPort sendPort) async {
    // final DynamicLibrary isolateLib = () {
    //   final isolateLibName = Platform.isWindows
    //       ? "JunkFusionDLL.dll"
    //       : Platform.isMacOS
    //       ? "JunkFusionDLL.dylib"
    //       : "JunkFusionDLL.so";

    //   return DynamicLibrary.open(isolateLibName);
    // }();

    // final isolateBindings = JunkFusionDLLBindings(isolateLib);

    final ReceivePort helperReceivePort = ReceivePort()
      ..listen((dynamic data) {
        // On the helper isolate listen to requests and respond to them.
        // print("开始执行DLL隔离区函数");
        if (data is _TaskRequest) {
          try {
            final name = data.funcName;
            final params = data.params;
            Map<String, Object?> results = {}; //Map为空和NULL是两种东西
            if (name == B_getAllSongs.name) {
              print("开始全量加载歌曲元数据");
              final cPtr = bindings.getAllSongs();
              final dartString = cPtr.cast<Utf8>().toDartString(); //解码
              Map<String, dynamic> obj = jsonDecode(dartString);
              //Object? 是“类型安全的未知类型”（你暂时不知道它是什么，但编译器会管着你）；
              //dynamic 是“彻底关闭类型检查的万能类型”（你爱怎么用就怎么用，编译器完全听你的，出错了运行时才报错）。
              bindings.freeString(cPtr);
              assert(obj.containsKey(B_getAllSongs.songsList));
              final songs = obj[B_getAllSongs.songsList] as List<dynamic>;
              List<SongInfo> songsList = [];
              for (int i = 0; i < songs.length; i++) {
                final song = songs[i];
                songsList.add(SongInfo.fromJson(song as Map<String, dynamic>));
              }
              results[B_getAllSongs.songsList] = songsList;
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
              malloc.free(
                cPtr,
              ); //因为这个c指针是dart的内存管理器分配在堆上面的，所以需要使用dart的calloc.free释放内存
            } else if (name == B_songImport.name) {
              // print("开始导入文件");
              assert(params.containsKey(B_songImport.filePaths));
              // print("1");
              String jsonStr = jsonEncode(params);
              // print("2");
              final cPtr = jsonStr.toNativeUtf8().cast<Char>();
              // print("3");
              final resultPtr = bindings.someImport(cPtr);
              // print("4");
              final resultJsonString = resultPtr.cast<Utf8>().toDartString();
              //   print(resultJsonString);
              // print("5");
              bindings.freeString(resultPtr);
              // print("6");
              malloc.free(cPtr);
              // print("导入成功");

              final resultObj =
                  jsonDecode(resultJsonString) as Map<String, dynamic>;

              // 用 List<dynamic> 接收，避免空数组时 as List<Map<...>> 类型转换失败
              //对象只能用as Map<String,dynamic>接收,数组只能用List<dynamic>接收
              //第一次写jsonDecode(resultJsonString) as Map<String, Object?>;没有报错的原因是Object是dynamic的基类
              //向上转型永远成功
              final songsRaw = resultObj[B_songImport.songs] as List<dynamic>;
              final errorFilesRaw =
                  resultObj[B_songImport.errorFiles] as List<dynamic>;

              List<SongInfo> songsList = [];
              for (int i = 0; i < songsRaw.length; i++) {
                songsList.add(
                  SongInfo.fromJson(songsRaw[i] as Map<String, dynamic>),
                );
              }
              results[B_songImport.songs] = songsList;
              results[B_songImport.errorFiles] = List<String>.from(
                errorFilesRaw,
              );
            }
            // print("准备发送回复消息");
            sendPort.send(_TaskResponse(data.id, results));
          } catch (e, stack) {
            // print("堆栈错误");
            sendPort.send(
              _TaskResponse(data.id, {'error': '$e', 'stack': '$stack'}),
            );
          }
        } else {
          // print("类型错误");
          sendPort.send(
            _TaskResponse(data.id, {
              'error': 'Unsupported message type: ${data.runtimeType}',
            }),
          );
        }
      });

    // Send the port to the main isolate on which we can receive requests.
    sendPort.send(helperReceivePort.sendPort); //建立主副隔离区的双向通信
  }, receivePort.sendPort);
  //第一个回调函数只会在第一次初始化的时候执行一次
  //Isolate.spawn参数：(入口函数, 初始消息);

  return initCompleter.future;
}(); //立即执行函数只执行一次，且后续永远不会因为任何变量“改变”而重新执行

// Completer 的所有成员（API 清单）
// 成员	类型	作用（一句话概括）
// Completer<T>()	构造函数	创建一个新遥控器，T 是你要返回的数据类型。
// future	Getter（属性）	获取遥控器对应的那张“小票”（Future<T>），将其交给调用方。
// complete(value)	方法	成功完成：将数据塞进 Future，所有等待的 await 会立即拿到 value。
// completeError(error, [stackTrace])	方法	失败完成：让 Future 抛出异常，触发 catchError。
// isCompleted	Getter（属性）	检查遥控器是否已经按过（无论成功还是失败），返回 bool。
