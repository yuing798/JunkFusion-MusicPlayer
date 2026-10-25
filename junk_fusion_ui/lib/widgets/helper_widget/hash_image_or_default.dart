import 'dart:io';

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';

class HashImageOrDefault extends StatelessWidget {
  final String? hash;
  const HashImageOrDefault({super.key, this.hash});

  @override
  Widget build(BuildContext context) {
    return (hash != null)
        ? Image.file(
            File('${AppCache.cacheDirString}/image/$hash/original.jpg'),
            fit: BoxFit.cover,
          )
        : Image.asset("assets/image/JunkFusion.png", fit: BoxFit.cover);
  }
}
