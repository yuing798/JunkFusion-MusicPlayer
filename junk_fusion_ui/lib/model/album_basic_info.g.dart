// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'album_basic_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_AlbumBasicInfo _$AlbumBasicInfoFromJson(Map<String, dynamic> json) =>
    _AlbumBasicInfo(
      albumId: (json['albumId'] as num).toInt(),
      title: json['title'] as String,
      albumArtists: (json['albumArtists'] as List<dynamic>?)
          ?.map((e) => e as String)
          .toList(),
      hash: json['hash'] as String?,
    );

Map<String, dynamic> _$AlbumBasicInfoToJson(_AlbumBasicInfo instance) =>
    <String, dynamic>{
      'albumId': instance.albumId,
      'title': instance.title,
      'albumArtists': instance.albumArtists,
      'hash': instance.hash,
    };
