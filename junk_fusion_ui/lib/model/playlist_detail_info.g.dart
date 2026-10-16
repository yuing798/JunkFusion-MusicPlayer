// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'playlist_detail_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_PlaylistDetailInfo _$PlaylistDetailInfoFromJson(Map<String, dynamic> json) =>
    _PlaylistDetailInfo(
      playlistId: (json['playlistId'] as num).toInt(),
      title: json['title'] as String,
      hash: json['hash'] as String?,
      songsLists: (json['songsLists'] as List<dynamic>?)
          ?.map((e) => (e as num).toInt())
          .toList(),
    );

Map<String, dynamic> _$PlaylistDetailInfoToJson(_PlaylistDetailInfo instance) =>
    <String, dynamic>{
      'playlistId': instance.playlistId,
      'title': instance.title,
      'hash': instance.hash,
      'songsLists': instance.songsLists,
    };
