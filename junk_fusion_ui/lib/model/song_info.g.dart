// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'song_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_SongInfo _$SongInfoFromJson(Map<String, dynamic> json) => _SongInfo(
  songId: (json['songId'] as num).toInt(),
  duration: (json['duration'] as num).toDouble(),
  title: json['title'] as String,
  artists: (json['artists'] as List<dynamic>?)
      ?.map((e) => e as String)
      .toList(),
  album: json['album'] as String?,
  trackNumber: (json['trackNumber'] as num?)?.toInt(),
  bitRate: (json['bitRate'] as num).toInt(),
  sampleRate: (json['sampleRate'] as num).toInt(),
  channelLayout: json['channelLayout'] as String,
  bitDepth: (json['bitDepth'] as num).toInt(),
  codecName: json['codecName'] as String?,
  bpm: (json['bpm'] as num?)?.toInt(),
  key: json['key'] as String?,
  isMyLike: json['isMyLike'] as bool,
  comment: json['comment'] as String?,
  playNum: (json['playNum'] as num).toInt(),
  hash: json['hash'] as String?,
);

Map<String, dynamic> _$SongInfoToJson(_SongInfo instance) => <String, dynamic>{
  'songId': instance.songId,
  'duration': instance.duration,
  'title': instance.title,
  'artists': instance.artists,
  'album': instance.album,
  'trackNumber': instance.trackNumber,
  'bitRate': instance.bitRate,
  'sampleRate': instance.sampleRate,
  'channelLayout': instance.channelLayout,
  'bitDepth': instance.bitDepth,
  'codecName': instance.codecName,
  'bpm': instance.bpm,
  'key': instance.key,
  'isMyLike': instance.isMyLike,
  'comment': instance.comment,
  'playNum': instance.playNum,
  'hash': instance.hash,
};
