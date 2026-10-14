// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'album_detail_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_AlbumDetailInfo _$AlbumDetailInfoFromJson(Map<String, dynamic> json) =>
    _AlbumDetailInfo(
      songsList: (json['songsList'] as List<dynamic>)
          .map((e) => (e as num).toInt())
          .toList(),
      comment: json['comment'] as String?,
    );

Map<String, dynamic> _$AlbumDetailInfoToJson(_AlbumDetailInfo instance) =>
    <String, dynamic>{
      'songsList': instance.songsList,
      'comment': instance.comment,
    };
