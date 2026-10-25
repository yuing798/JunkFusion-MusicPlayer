// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'playlist_basic_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_PlaylistBasicInfo _$PlaylistBasicInfoFromJson(Map<String, dynamic> json) =>
    _PlaylistBasicInfo(
      id: (json['id'] as num).toInt(),
      title: json['title'] as String,
      createTime: json['createTime'] as String,
      hash: json['hash'] as String?,
    );

Map<String, dynamic> _$PlaylistBasicInfoToJson(_PlaylistBasicInfo instance) =>
    <String, dynamic>{
      'id': instance.id,
      'title': instance.title,
      'createTime': instance.createTime,
      'hash': instance.hash,
    };
