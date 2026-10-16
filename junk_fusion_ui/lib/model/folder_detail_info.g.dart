// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'folder_detail_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_FolderDetailInfo _$FolderDetailInfoFromJson(Map<String, dynamic> json) =>
    _FolderDetailInfo(
      path: json['path'] as String,
      name: json['name'] as String,
      hash: json['hash'] as String?,
      songsLists: (json['songsLists'] as List<dynamic>?)
          ?.map((e) => (e as num).toInt())
          .toList(),
    );

Map<String, dynamic> _$FolderDetailInfoToJson(_FolderDetailInfo instance) =>
    <String, dynamic>{
      'path': instance.path,
      'name': instance.name,
      'hash': instance.hash,
      'songsLists': instance.songsLists,
    };
