// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'playlist_basic_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_PlaylistBasicInfo _$PlaylistBasicInfoFromJson(Map<String, dynamic> json) =>
    _PlaylistBasicInfo(
      playlistId: (json['playlistId'] as num).toInt(),
      title: json['title'] as String,
      songCount: (json['songCount'] as num).toInt(),
      hash: json['hash'] as String?,
    );

Map<String, dynamic> _$PlaylistBasicInfoToJson(_PlaylistBasicInfo instance) =>
    <String, dynamic>{
      'playlistId': instance.playlistId,
      'title': instance.title,
      'songCount': instance.songCount,
      'hash': instance.hash,
    };
