// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'song_basic_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_SongBasicInfo _$SongBasicInfoFromJson(Map<String, dynamic> json) =>
    _SongBasicInfo(
      songId: (json['songId'] as num).toInt(),
      duration: (json['duration'] as num).toDouble(),
      title: json['title'] as String,
      artists: (json['artists'] as List<dynamic>?)
          ?.map((e) => e as String)
          .toList(),
      album: json['album'] as String?,
      isMyLike: json['isMyLike'] as bool,
      playNum: (json['playNum'] as num).toInt(),
      hash: json['hash'] as String?,
    );

Map<String, dynamic> _$SongBasicInfoToJson(_SongBasicInfo instance) =>
    <String, dynamic>{
      'songId': instance.songId,
      'duration': instance.duration,
      'title': instance.title,
      'artists': instance.artists,
      'album': instance.album,
      'isMyLike': instance.isMyLike,
      'playNum': instance.playNum,
      'hash': instance.hash,
    };
