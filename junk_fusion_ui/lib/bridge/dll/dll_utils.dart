import 'dart:ffi';
import 'dart:io';
import 'dart:async';
import 'dart:isolate';

import 'package:freezed_annotation/freezed_annotation.dart';

import 'native_bindings_generated.dart' as bindings;

final DynamicLibrary _lib = () {
  final libName = Platform.isWindows
      ? "JunkFusionDLL.dll"
      : Platform.isMacOS
      ? "JunkFusionDLL.dylib"
      : "JunkFusionDLL.so";

  return DynamicLibrary.open(libName);
}();

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

  const _TaskRequest(this.funcName, this.params);
}

/// A response with the result of `sum`.
///
/// Typically sent from one isolate to another.
class _SumResponse {
  final int id;
  final int result;

  const _SumResponse(this.id, this.result);
}

/// Counter to identify [_SumRequest]s and [_SumResponse]s.
int _nextSumRequestId = 0;

/// Mapping from [_SumRequest] `id`s to the completers corresponding to the correct future of the pending request.
final _sumRequests = <int, Completer<int>>{};

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
        if (data is _SumRequest) {
          final int result = bindings.sum_long_running(data.a, data.b);
          final response = _SumResponse(data.id, result);
          sendPort.send(response);
          return;
        }
        throw UnsupportedError('Unsupported message type: ${data.runtimeType}');
      });

    // Send the port to the main isolate on which we can receive requests.
    sendPort.send(helperReceivePort.sendPort); //sendPort只是标记了receivePort的接口ID
    //这个 sendPort.send(helperReceivePort.sendPort) 是 Isolate 双向通信的“握手”协议
    //这样子才能完成双向通信
  }, receivePort.sendPort);
  //第一个回调函数只会在第一次初始化的时候执行一次
  //Isolate.spawn参数：(入口函数, 初始消息);

  // Wait until the helper isolate has sent us back the SendPort on which we
  // can start sending requests.
  return completer.future;
}(); //最后的 }();:（立即执行函数,也就是说定义完成这个函数后立刻执行一次
