// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'playlist_detail_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_PlaylistDetailInfo _$PlaylistDetailInfoFromJson(Map<String, dynamic> json) =>
    _PlaylistDetailInfo(
      songsLists: (json['songsLists'] as List<dynamic>?)
          ?.map((e) => (e as num).toInt())
          .toList(),
    );

Map<String, dynamic> _$PlaylistDetailInfoToJson(_PlaylistDetailInfo instance) =>
    <String, dynamic>{'songsLists': instance.songsLists};
