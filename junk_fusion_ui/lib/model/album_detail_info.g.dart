// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'album_detail_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_AlbumDetailInfo _$AlbumDetailInfoFromJson(Map<String, dynamic> json) =>
    _AlbumDetailInfo(
      albumId: (json['albumId'] as num).toInt(),
      title: json['title'] as String,
      albumArtists: (json['albumArtists'] as List<dynamic>?)
          ?.map((e) => e as String)
          .toList(),
      hash: json['hash'] as String?,
      songsList: (json['songsList'] as List<dynamic>?)
          ?.map((e) => (e as num).toInt())
          .toList(),
    );

Map<String, dynamic> _$AlbumDetailInfoToJson(_AlbumDetailInfo instance) =>
    <String, dynamic>{
      'albumId': instance.albumId,
      'title': instance.title,
      'albumArtists': instance.albumArtists,
      'hash': instance.hash,
      'songsList': instance.songsList,
    };
