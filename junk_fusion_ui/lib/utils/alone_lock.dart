//单应用模式应用锁
import 'dart:io';

import 'package:flutter_alone/flutter_alone.dart';

FlutterAloneConfig? flutterAloneConfig = () {
  FlutterAloneConfig? config;
  if (Platform.isWindows) {
    config = FlutterAloneConfig.forWindows(
      windowsConfig: const DefaultWindowsMutexConfig(
        packageId: 'com.YusekX.JunkFusion',
        appName: 'Junk Fusion',
      ),
      messageConfig: const EnMessageConfig(),
    );
  } else if (Platform.isMacOS) {
    config = FlutterAloneConfig.forMacOS(
      macOSConfig: MacOSConfig(lockFileName: 'Junk_Fusion.lock'),
      messageConfig: const EnMessageConfig(),
    );
  } else if (Platform.isLinux) {
    config = FlutterAloneConfig.forLinux(
      linuxConfig: LinuxConfig(lockFileName: 'Junk_Fusion.lock'),
      messageConfig: const EnMessageConfig(),
    );
  }
  return config;
}();
