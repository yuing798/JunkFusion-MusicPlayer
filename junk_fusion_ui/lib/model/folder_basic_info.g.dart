// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'folder_basic_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_FolderBasicInfo _$FolderBasicInfoFromJson(Map<String, dynamic> json) =>
    _FolderBasicInfo(
      path: json['path'] as String,
      name: json['name'] as String,
      songCount: (json['songCount'] as num).toInt(),
      hash: json['hash'] as String?,
    );

Map<String, dynamic> _$FolderBasicInfoToJson(_FolderBasicInfo instance) =>
    <String, dynamic>{
      'path': instance.path,
      'name': instance.name,
      'songCount': instance.songCount,
      'hash': instance.hash,
    };
