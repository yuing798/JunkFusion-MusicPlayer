// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'album_detail_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_AlbumDetailInfo _$AlbumDetailInfoFromJson(Map<String, dynamic> json) =>
    _AlbumDetailInfo(
      songsLists: (json['songsLists'] as List<dynamic>)
          .map((e) => (e as num).toInt())
          .toList(),
    );

Map<String, dynamic> _$AlbumDetailInfoToJson(_AlbumDetailInfo instance) =>
    <String, dynamic>{'songsLists': instance.songsLists};
