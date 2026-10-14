// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'song_detail_info.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$SongDetailInfo {

 int get songId; double get duration; String get title; List<String>? get artists; String? get album; int? get trackNumber; int? get discNumber; int get bitRate; int get sampleRate; String get channelLayout; int get bitDepth; String? get codecName; int? get bpm; String? get key; bool get isMyLike; int get playNum; String? get hash; List<double> get timeDomainSpec;
/// Create a copy of SongDetailInfo
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$SongDetailInfoCopyWith<SongDetailInfo> get copyWith => _$SongDetailInfoCopyWithImpl<SongDetailInfo>(this as SongDetailInfo, _$identity);

  /// Serializes this SongDetailInfo to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is SongDetailInfo&&(identical(other.songId, songId) || other.songId == songId)&&(identical(other.duration, duration) || other.duration == duration)&&(identical(other.title, title) || other.title == title)&&const DeepCollectionEquality().equals(other.artists, artists)&&(identical(other.album, album) || other.album == album)&&(identical(other.trackNumber, trackNumber) || other.trackNumber == trackNumber)&&(identical(other.discNumber, discNumber) || other.discNumber == discNumber)&&(identical(other.bitRate, bitRate) || other.bitRate == bitRate)&&(identical(other.sampleRate, sampleRate) || other.sampleRate == sampleRate)&&(identical(other.channelLayout, channelLayout) || other.channelLayout == channelLayout)&&(identical(other.bitDepth, bitDepth) || other.bitDepth == bitDepth)&&(identical(other.codecName, codecName) || other.codecName == codecName)&&(identical(other.bpm, bpm) || other.bpm == bpm)&&(identical(other.key, key) || other.key == key)&&(identical(other.isMyLike, isMyLike) || other.isMyLike == isMyLike)&&(identical(other.playNum, playNum) || other.playNum == playNum)&&(identical(other.hash, hash) || other.hash == hash)&&const DeepCollectionEquality().equals(other.timeDomainSpec, timeDomainSpec));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,songId,duration,title,const DeepCollectionEquality().hash(artists),album,trackNumber,discNumber,bitRate,sampleRate,channelLayout,bitDepth,codecName,bpm,key,isMyLike,playNum,hash,const DeepCollectionEquality().hash(timeDomainSpec));

@override
String toString() {
  return 'SongDetailInfo(songId: $songId, duration: $duration, title: $title, artists: $artists, album: $album, trackNumber: $trackNumber, discNumber: $discNumber, bitRate: $bitRate, sampleRate: $sampleRate, channelLayout: $channelLayout, bitDepth: $bitDepth, codecName: $codecName, bpm: $bpm, key: $key, isMyLike: $isMyLike, playNum: $playNum, hash: $hash, timeDomainSpec: $timeDomainSpec)';
}


}

/// @nodoc
abstract mixin class $SongDetailInfoCopyWith<$Res>  {
  factory $SongDetailInfoCopyWith(SongDetailInfo value, $Res Function(SongDetailInfo) _then) = _$SongDetailInfoCopyWithImpl;
@useResult
$Res call({
 int songId, double duration, String title, List<String>? artists, String? album, int? trackNumber, int? discNumber, int bitRate, int sampleRate, String channelLayout, int bitDepth, String? codecName, int? bpm, String? key, bool isMyLike, int playNum, String? hash, List<double> timeDomainSpec
});




}
/// @nodoc
class _$SongDetailInfoCopyWithImpl<$Res>
    implements $SongDetailInfoCopyWith<$Res> {
  _$SongDetailInfoCopyWithImpl(this._self, this._then);

  final SongDetailInfo _self;
  final $Res Function(SongDetailInfo) _then;

/// Create a copy of SongDetailInfo
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? songId = null,Object? duration = null,Object? title = null,Object? artists = freezed,Object? album = freezed,Object? trackNumber = freezed,Object? discNumber = freezed,Object? bitRate = null,Object? sampleRate = null,Object? channelLayout = null,Object? bitDepth = null,Object? codecName = freezed,Object? bpm = freezed,Object? key = freezed,Object? isMyLike = null,Object? playNum = null,Object? hash = freezed,Object? timeDomainSpec = null,}) {
  return _then(_self.copyWith(
songId: null == songId ? _self.songId : songId // ignore: cast_nullable_to_non_nullable
as int,duration: null == duration ? _self.duration : duration // ignore: cast_nullable_to_non_nullable
as double,title: null == title ? _self.title : title // ignore: cast_nullable_to_non_nullable
as String,artists: freezed == artists ? _self.artists : artists // ignore: cast_nullable_to_non_nullable
as List<String>?,album: freezed == album ? _self.album : album // ignore: cast_nullable_to_non_nullable
as String?,trackNumber: freezed == trackNumber ? _self.trackNumber : trackNumber // ignore: cast_nullable_to_non_nullable
as int?,discNumber: freezed == discNumber ? _self.discNumber : discNumber // ignore: cast_nullable_to_non_nullable
as int?,bitRate: null == bitRate ? _self.bitRate : bitRate // ignore: cast_nullable_to_non_nullable
as int,sampleRate: null == sampleRate ? _self.sampleRate : sampleRate // ignore: cast_nullable_to_non_nullable
as int,channelLayout: null == channelLayout ? _self.channelLayout : channelLayout // ignore: cast_nullable_to_non_nullable
as String,bitDepth: null == bitDepth ? _self.bitDepth : bitDepth // ignore: cast_nullable_to_non_nullable
as int,codecName: freezed == codecName ? _self.codecName : codecName // ignore: cast_nullable_to_non_nullable
as String?,bpm: freezed == bpm ? _self.bpm : bpm // ignore: cast_nullable_to_non_nullable
as int?,key: freezed == key ? _self.key : key // ignore: cast_nullable_to_non_nullable
as String?,isMyLike: null == isMyLike ? _self.isMyLike : isMyLike // ignore: cast_nullable_to_non_nullable
as bool,playNum: null == playNum ? _self.playNum : playNum // ignore: cast_nullable_to_non_nullable
as int,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,timeDomainSpec: null == timeDomainSpec ? _self.timeDomainSpec : timeDomainSpec // ignore: cast_nullable_to_non_nullable
as List<double>,
  ));
}

}


/// Adds pattern-matching-related methods to [SongDetailInfo].
extension SongDetailInfoPatterns on SongDetailInfo {
/// A variant of `map` that fallback to returning `orElse`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _SongDetailInfo value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _SongDetailInfo() when $default != null:
return $default(_that);case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// Callbacks receives the raw object, upcasted.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case final Subclass2 value:
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _SongDetailInfo value)  $default,){
final _that = this;
switch (_that) {
case _SongDetailInfo():
return $default(_that);case _:
  throw StateError('Unexpected subclass');

}
}
/// A variant of `map` that fallback to returning `null`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _SongDetailInfo value)?  $default,){
final _that = this;
switch (_that) {
case _SongDetailInfo() when $default != null:
return $default(_that);case _:
  return null;

}
}
/// A variant of `when` that fallback to an `orElse` callback.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int songId,  double duration,  String title,  List<String>? artists,  String? album,  int? trackNumber,  int? discNumber,  int bitRate,  int sampleRate,  String channelLayout,  int bitDepth,  String? codecName,  int? bpm,  String? key,  bool isMyLike,  int playNum,  String? hash,  List<double> timeDomainSpec)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _SongDetailInfo() when $default != null:
return $default(_that.songId,_that.duration,_that.title,_that.artists,_that.album,_that.trackNumber,_that.discNumber,_that.bitRate,_that.sampleRate,_that.channelLayout,_that.bitDepth,_that.codecName,_that.bpm,_that.key,_that.isMyLike,_that.playNum,_that.hash,_that.timeDomainSpec);case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// As opposed to `map`, this offers destructuring.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case Subclass2(:final field2):
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int songId,  double duration,  String title,  List<String>? artists,  String? album,  int? trackNumber,  int? discNumber,  int bitRate,  int sampleRate,  String channelLayout,  int bitDepth,  String? codecName,  int? bpm,  String? key,  bool isMyLike,  int playNum,  String? hash,  List<double> timeDomainSpec)  $default,) {final _that = this;
switch (_that) {
case _SongDetailInfo():
return $default(_that.songId,_that.duration,_that.title,_that.artists,_that.album,_that.trackNumber,_that.discNumber,_that.bitRate,_that.sampleRate,_that.channelLayout,_that.bitDepth,_that.codecName,_that.bpm,_that.key,_that.isMyLike,_that.playNum,_that.hash,_that.timeDomainSpec);case _:
  throw StateError('Unexpected subclass');

}
}
/// A variant of `when` that fallback to returning `null`
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int songId,  double duration,  String title,  List<String>? artists,  String? album,  int? trackNumber,  int? discNumber,  int bitRate,  int sampleRate,  String channelLayout,  int bitDepth,  String? codecName,  int? bpm,  String? key,  bool isMyLike,  int playNum,  String? hash,  List<double> timeDomainSpec)?  $default,) {final _that = this;
switch (_that) {
case _SongDetailInfo() when $default != null:
return $default(_that.songId,_that.duration,_that.title,_that.artists,_that.album,_that.trackNumber,_that.discNumber,_that.bitRate,_that.sampleRate,_that.channelLayout,_that.bitDepth,_that.codecName,_that.bpm,_that.key,_that.isMyLike,_that.playNum,_that.hash,_that.timeDomainSpec);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _SongDetailInfo implements SongDetailInfo {
  const _SongDetailInfo({required this.songId, required this.duration, required this.title, required final  List<String>? artists, this.album, this.trackNumber, this.discNumber, required this.bitRate, required this.sampleRate, required this.channelLayout, required this.bitDepth, this.codecName, this.bpm, this.key, required this.isMyLike, required this.playNum, this.hash, required final  List<double> timeDomainSpec}): _artists = artists,_timeDomainSpec = timeDomainSpec;
  factory _SongDetailInfo.fromJson(Map<String, dynamic> json) => _$SongDetailInfoFromJson(json);

@override final  int songId;
@override final  double duration;
@override final  String title;
 final  List<String>? _artists;
@override List<String>? get artists {
  final value = _artists;
  if (value == null) return null;
  if (_artists is EqualUnmodifiableListView) return _artists;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(value);
}

@override final  String? album;
@override final  int? trackNumber;
@override final  int? discNumber;
@override final  int bitRate;
@override final  int sampleRate;
@override final  String channelLayout;
@override final  int bitDepth;
@override final  String? codecName;
@override final  int? bpm;
@override final  String? key;
@override final  bool isMyLike;
@override final  int playNum;
@override final  String? hash;
 final  List<double> _timeDomainSpec;
@override List<double> get timeDomainSpec {
  if (_timeDomainSpec is EqualUnmodifiableListView) return _timeDomainSpec;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(_timeDomainSpec);
}


/// Create a copy of SongDetailInfo
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$SongDetailInfoCopyWith<_SongDetailInfo> get copyWith => __$SongDetailInfoCopyWithImpl<_SongDetailInfo>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$SongDetailInfoToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _SongDetailInfo&&(identical(other.songId, songId) || other.songId == songId)&&(identical(other.duration, duration) || other.duration == duration)&&(identical(other.title, title) || other.title == title)&&const DeepCollectionEquality().equals(other._artists, _artists)&&(identical(other.album, album) || other.album == album)&&(identical(other.trackNumber, trackNumber) || other.trackNumber == trackNumber)&&(identical(other.discNumber, discNumber) || other.discNumber == discNumber)&&(identical(other.bitRate, bitRate) || other.bitRate == bitRate)&&(identical(other.sampleRate, sampleRate) || other.sampleRate == sampleRate)&&(identical(other.channelLayout, channelLayout) || other.channelLayout == channelLayout)&&(identical(other.bitDepth, bitDepth) || other.bitDepth == bitDepth)&&(identical(other.codecName, codecName) || other.codecName == codecName)&&(identical(other.bpm, bpm) || other.bpm == bpm)&&(identical(other.key, key) || other.key == key)&&(identical(other.isMyLike, isMyLike) || other.isMyLike == isMyLike)&&(identical(other.playNum, playNum) || other.playNum == playNum)&&(identical(other.hash, hash) || other.hash == hash)&&const DeepCollectionEquality().equals(other._timeDomainSpec, _timeDomainSpec));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,songId,duration,title,const DeepCollectionEquality().hash(_artists),album,trackNumber,discNumber,bitRate,sampleRate,channelLayout,bitDepth,codecName,bpm,key,isMyLike,playNum,hash,const DeepCollectionEquality().hash(_timeDomainSpec));

@override
String toString() {
  return 'SongDetailInfo(songId: $songId, duration: $duration, title: $title, artists: $artists, album: $album, trackNumber: $trackNumber, discNumber: $discNumber, bitRate: $bitRate, sampleRate: $sampleRate, channelLayout: $channelLayout, bitDepth: $bitDepth, codecName: $codecName, bpm: $bpm, key: $key, isMyLike: $isMyLike, playNum: $playNum, hash: $hash, timeDomainSpec: $timeDomainSpec)';
}


}

/// @nodoc
abstract mixin class _$SongDetailInfoCopyWith<$Res> implements $SongDetailInfoCopyWith<$Res> {
  factory _$SongDetailInfoCopyWith(_SongDetailInfo value, $Res Function(_SongDetailInfo) _then) = __$SongDetailInfoCopyWithImpl;
@override @useResult
$Res call({
 int songId, double duration, String title, List<String>? artists, String? album, int? trackNumber, int? discNumber, int bitRate, int sampleRate, String channelLayout, int bitDepth, String? codecName, int? bpm, String? key, bool isMyLike, int playNum, String? hash, List<double> timeDomainSpec
});




}
/// @nodoc
class __$SongDetailInfoCopyWithImpl<$Res>
    implements _$SongDetailInfoCopyWith<$Res> {
  __$SongDetailInfoCopyWithImpl(this._self, this._then);

  final _SongDetailInfo _self;
  final $Res Function(_SongDetailInfo) _then;

/// Create a copy of SongDetailInfo
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? songId = null,Object? duration = null,Object? title = null,Object? artists = freezed,Object? album = freezed,Object? trackNumber = freezed,Object? discNumber = freezed,Object? bitRate = null,Object? sampleRate = null,Object? channelLayout = null,Object? bitDepth = null,Object? codecName = freezed,Object? bpm = freezed,Object? key = freezed,Object? isMyLike = null,Object? playNum = null,Object? hash = freezed,Object? timeDomainSpec = null,}) {
  return _then(_SongDetailInfo(
songId: null == songId ? _self.songId : songId // ignore: cast_nullable_to_non_nullable
as int,duration: null == duration ? _self.duration : duration // ignore: cast_nullable_to_non_nullable
as double,title: null == title ? _self.title : title // ignore: cast_nullable_to_non_nullable
as String,artists: freezed == artists ? _self._artists : artists // ignore: cast_nullable_to_non_nullable
as List<String>?,album: freezed == album ? _self.album : album // ignore: cast_nullable_to_non_nullable
as String?,trackNumber: freezed == trackNumber ? _self.trackNumber : trackNumber // ignore: cast_nullable_to_non_nullable
as int?,discNumber: freezed == discNumber ? _self.discNumber : discNumber // ignore: cast_nullable_to_non_nullable
as int?,bitRate: null == bitRate ? _self.bitRate : bitRate // ignore: cast_nullable_to_non_nullable
as int,sampleRate: null == sampleRate ? _self.sampleRate : sampleRate // ignore: cast_nullable_to_non_nullable
as int,channelLayout: null == channelLayout ? _self.channelLayout : channelLayout // ignore: cast_nullable_to_non_nullable
as String,bitDepth: null == bitDepth ? _self.bitDepth : bitDepth // ignore: cast_nullable_to_non_nullable
as int,codecName: freezed == codecName ? _self.codecName : codecName // ignore: cast_nullable_to_non_nullable
as String?,bpm: freezed == bpm ? _self.bpm : bpm // ignore: cast_nullable_to_non_nullable
as int?,key: freezed == key ? _self.key : key // ignore: cast_nullable_to_non_nullable
as String?,isMyLike: null == isMyLike ? _self.isMyLike : isMyLike // ignore: cast_nullable_to_non_nullable
as bool,playNum: null == playNum ? _self.playNum : playNum // ignore: cast_nullable_to_non_nullable
as int,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,timeDomainSpec: null == timeDomainSpec ? _self._timeDomainSpec : timeDomainSpec // ignore: cast_nullable_to_non_nullable
as List<double>,
  ));
}


}

// dart format on
