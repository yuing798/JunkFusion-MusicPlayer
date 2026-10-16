// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'song_detail_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_SongDetailInfo _$SongDetailInfoFromJson(Map<String, dynamic> json) =>
    _SongDetailInfo(
      bitRate: (json['bitRate'] as num).toInt(),
      sampleRate: (json['sampleRate'] as num).toInt(),
      channelLayout: json['channelLayout'] as String,
      bitDepth: (json['bitDepth'] as num).toInt(),
      codecName: json['codecName'] as String?,
      bpm: (json['bpm'] as num?)?.toInt(),
      key: json['key'] as String?,
      hash: json['hash'] as String?,
      timeDomainSpec: (json['timeDomainSpec'] as List<dynamic>)
          .map((e) => (e as num).toDouble())
          .toList(),
    );

Map<String, dynamic> _$SongDetailInfoToJson(_SongDetailInfo instance) =>
    <String, dynamic>{
      'bitRate': instance.bitRate,
      'sampleRate': instance.sampleRate,
      'channelLayout': instance.channelLayout,
      'bitDepth': instance.bitDepth,
      'codecName': instance.codecName,
      'bpm': instance.bpm,
      'key': instance.key,
      'hash': instance.hash,
      'timeDomainSpec': instance.timeDomainSpec,
    };
