// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'song_info.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$SongInfo {

 int get songId; double get duration; String get title; String? get artist; String? get album; String? get albumArtist; String? get genre; int? get trackNumber; int? get discNumber; int? get year; String? get composer; int get bitRate; int get sampleRate; String get channelLayout; int get bitDepth; String? get codecName; String? get aiGenre; int? get bpm; String? get key; bool get isMyLike; String? get comment; int get playNum; String? get hash;
/// Create a copy of SongInfo
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$SongInfoCopyWith<SongInfo> get copyWith => _$SongInfoCopyWithImpl<SongInfo>(this as SongInfo, _$identity);

  /// Serializes this SongInfo to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is SongInfo&&(identical(other.songId, songId) || other.songId == songId)&&(identical(other.duration, duration) || other.duration == duration)&&(identical(other.title, title) || other.title == title)&&(identical(other.artist, artist) || other.artist == artist)&&(identical(other.album, album) || other.album == album)&&(identical(other.albumArtist, albumArtist) || other.albumArtist == albumArtist)&&(identical(other.genre, genre) || other.genre == genre)&&(identical(other.trackNumber, trackNumber) || other.trackNumber == trackNumber)&&(identical(other.discNumber, discNumber) || other.discNumber == discNumber)&&(identical(other.year, year) || other.year == year)&&(identical(other.composer, composer) || other.composer == composer)&&(identical(other.bitRate, bitRate) || other.bitRate == bitRate)&&(identical(other.sampleRate, sampleRate) || other.sampleRate == sampleRate)&&(identical(other.channelLayout, channelLayout) || other.channelLayout == channelLayout)&&(identical(other.bitDepth, bitDepth) || other.bitDepth == bitDepth)&&(identical(other.codecName, codecName) || other.codecName == codecName)&&(identical(other.aiGenre, aiGenre) || other.aiGenre == aiGenre)&&(identical(other.bpm, bpm) || other.bpm == bpm)&&(identical(other.key, key) || other.key == key)&&(identical(other.isMyLike, isMyLike) || other.isMyLike == isMyLike)&&(identical(other.comment, comment) || other.comment == comment)&&(identical(other.playNum, playNum) || other.playNum == playNum)&&(identical(other.hash, hash) || other.hash == hash));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hashAll([runtimeType,songId,duration,title,artist,album,albumArtist,genre,trackNumber,discNumber,year,composer,bitRate,sampleRate,channelLayout,bitDepth,codecName,aiGenre,bpm,key,isMyLike,comment,playNum,hash]);

@override
String toString() {
  return 'SongInfo(songId: $songId, duration: $duration, title: $title, artist: $artist, album: $album, albumArtist: $albumArtist, genre: $genre, trackNumber: $trackNumber, discNumber: $discNumber, year: $year, composer: $composer, bitRate: $bitRate, sampleRate: $sampleRate, channelLayout: $channelLayout, bitDepth: $bitDepth, codecName: $codecName, aiGenre: $aiGenre, bpm: $bpm, key: $key, isMyLike: $isMyLike, comment: $comment, playNum: $playNum, hash: $hash)';
}


}

/// @nodoc
abstract mixin class $SongInfoCopyWith<$Res>  {
  factory $SongInfoCopyWith(SongInfo value, $Res Function(SongInfo) _then) = _$SongInfoCopyWithImpl;
@useResult
$Res call({
 int songId, double duration, String title, String? artist, String? album, String? albumArtist, String? genre, int? trackNumber, int? discNumber, int? year, String? composer, int bitRate, int sampleRate, String channelLayout, int bitDepth, String? codecName, String? aiGenre, int? bpm, String? key, bool isMyLike, String? comment, int playNum, String? hash
});




}
/// @nodoc
class _$SongInfoCopyWithImpl<$Res>
    implements $SongInfoCopyWith<$Res> {
  _$SongInfoCopyWithImpl(this._self, this._then);

  final SongInfo _self;
  final $Res Function(SongInfo) _then;

/// Create a copy of SongInfo
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? songId = null,Object? duration = null,Object? title = null,Object? artist = freezed,Object? album = freezed,Object? albumArtist = freezed,Object? genre = freezed,Object? trackNumber = freezed,Object? discNumber = freezed,Object? year = freezed,Object? composer = freezed,Object? bitRate = null,Object? sampleRate = null,Object? channelLayout = null,Object? bitDepth = null,Object? codecName = freezed,Object? aiGenre = freezed,Object? bpm = freezed,Object? key = freezed,Object? isMyLike = null,Object? comment = freezed,Object? playNum = null,Object? hash = freezed,}) {
  return _then(_self.copyWith(
songId: null == songId ? _self.songId : songId // ignore: cast_nullable_to_non_nullable
as int,duration: null == duration ? _self.duration : duration // ignore: cast_nullable_to_non_nullable
as double,title: null == title ? _self.title : title // ignore: cast_nullable_to_non_nullable
as String,artist: freezed == artist ? _self.artist : artist // ignore: cast_nullable_to_non_nullable
as String?,album: freezed == album ? _self.album : album // ignore: cast_nullable_to_non_nullable
as String?,albumArtist: freezed == albumArtist ? _self.albumArtist : albumArtist // ignore: cast_nullable_to_non_nullable
as String?,genre: freezed == genre ? _self.genre : genre // ignore: cast_nullable_to_non_nullable
as String?,trackNumber: freezed == trackNumber ? _self.trackNumber : trackNumber // ignore: cast_nullable_to_non_nullable
as int?,discNumber: freezed == discNumber ? _self.discNumber : discNumber // ignore: cast_nullable_to_non_nullable
as int?,year: freezed == year ? _self.year : year // ignore: cast_nullable_to_non_nullable
as int?,composer: freezed == composer ? _self.composer : composer // ignore: cast_nullable_to_non_nullable
as String?,bitRate: null == bitRate ? _self.bitRate : bitRate // ignore: cast_nullable_to_non_nullable
as int,sampleRate: null == sampleRate ? _self.sampleRate : sampleRate // ignore: cast_nullable_to_non_nullable
as int,channelLayout: null == channelLayout ? _self.channelLayout : channelLayout // ignore: cast_nullable_to_non_nullable
as String,bitDepth: null == bitDepth ? _self.bitDepth : bitDepth // ignore: cast_nullable_to_non_nullable
as int,codecName: freezed == codecName ? _self.codecName : codecName // ignore: cast_nullable_to_non_nullable
as String?,aiGenre: freezed == aiGenre ? _self.aiGenre : aiGenre // ignore: cast_nullable_to_non_nullable
as String?,bpm: freezed == bpm ? _self.bpm : bpm // ignore: cast_nullable_to_non_nullable
as int?,key: freezed == key ? _self.key : key // ignore: cast_nullable_to_non_nullable
as String?,isMyLike: null == isMyLike ? _self.isMyLike : isMyLike // ignore: cast_nullable_to_non_nullable
as bool,comment: freezed == comment ? _self.comment : comment // ignore: cast_nullable_to_non_nullable
as String?,playNum: null == playNum ? _self.playNum : playNum // ignore: cast_nullable_to_non_nullable
as int,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,
  ));
}

}


/// Adds pattern-matching-related methods to [SongInfo].
extension SongInfoPatterns on SongInfo {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _SongInfo value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _SongInfo() when $default != null:
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

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _SongInfo value)  $default,){
final _that = this;
switch (_that) {
case _SongInfo():
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _SongInfo value)?  $default,){
final _that = this;
switch (_that) {
case _SongInfo() when $default != null:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int songId,  double duration,  String title,  String? artist,  String? album,  String? albumArtist,  String? genre,  int? trackNumber,  int? discNumber,  int? year,  String? composer,  int bitRate,  int sampleRate,  String channelLayout,  int bitDepth,  String? codecName,  String? aiGenre,  int? bpm,  String? key,  bool isMyLike,  String? comment,  int playNum,  String? hash)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _SongInfo() when $default != null:
return $default(_that.songId,_that.duration,_that.title,_that.artist,_that.album,_that.albumArtist,_that.genre,_that.trackNumber,_that.discNumber,_that.year,_that.composer,_that.bitRate,_that.sampleRate,_that.channelLayout,_that.bitDepth,_that.codecName,_that.aiGenre,_that.bpm,_that.key,_that.isMyLike,_that.comment,_that.playNum,_that.hash);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int songId,  double duration,  String title,  String? artist,  String? album,  String? albumArtist,  String? genre,  int? trackNumber,  int? discNumber,  int? year,  String? composer,  int bitRate,  int sampleRate,  String channelLayout,  int bitDepth,  String? codecName,  String? aiGenre,  int? bpm,  String? key,  bool isMyLike,  String? comment,  int playNum,  String? hash)  $default,) {final _that = this;
switch (_that) {
case _SongInfo():
return $default(_that.songId,_that.duration,_that.title,_that.artist,_that.album,_that.albumArtist,_that.genre,_that.trackNumber,_that.discNumber,_that.year,_that.composer,_that.bitRate,_that.sampleRate,_that.channelLayout,_that.bitDepth,_that.codecName,_that.aiGenre,_that.bpm,_that.key,_that.isMyLike,_that.comment,_that.playNum,_that.hash);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int songId,  double duration,  String title,  String? artist,  String? album,  String? albumArtist,  String? genre,  int? trackNumber,  int? discNumber,  int? year,  String? composer,  int bitRate,  int sampleRate,  String channelLayout,  int bitDepth,  String? codecName,  String? aiGenre,  int? bpm,  String? key,  bool isMyLike,  String? comment,  int playNum,  String? hash)?  $default,) {final _that = this;
switch (_that) {
case _SongInfo() when $default != null:
return $default(_that.songId,_that.duration,_that.title,_that.artist,_that.album,_that.albumArtist,_that.genre,_that.trackNumber,_that.discNumber,_that.year,_that.composer,_that.bitRate,_that.sampleRate,_that.channelLayout,_that.bitDepth,_that.codecName,_that.aiGenre,_that.bpm,_that.key,_that.isMyLike,_that.comment,_that.playNum,_that.hash);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _SongInfo implements SongInfo {
  const _SongInfo({required this.songId, required this.duration, required this.title, this.artist, this.album, this.albumArtist, this.genre, this.trackNumber, this.discNumber, this.year, this.composer, required this.bitRate, required this.sampleRate, required this.channelLayout, required this.bitDepth, this.codecName, this.aiGenre, this.bpm, this.key, required this.isMyLike, this.comment, required this.playNum, this.hash});
  factory _SongInfo.fromJson(Map<String, dynamic> json) => _$SongInfoFromJson(json);

@override final  int songId;
@override final  double duration;
@override final  String title;
@override final  String? artist;
@override final  String? album;
@override final  String? albumArtist;
@override final  String? genre;
@override final  int? trackNumber;
@override final  int? discNumber;
@override final  int? year;
@override final  String? composer;
@override final  int bitRate;
@override final  int sampleRate;
@override final  String channelLayout;
@override final  int bitDepth;
@override final  String? codecName;
@override final  String? aiGenre;
@override final  int? bpm;
@override final  String? key;
@override final  bool isMyLike;
@override final  String? comment;
@override final  int playNum;
@override final  String? hash;

/// Create a copy of SongInfo
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$SongInfoCopyWith<_SongInfo> get copyWith => __$SongInfoCopyWithImpl<_SongInfo>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$SongInfoToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _SongInfo&&(identical(other.songId, songId) || other.songId == songId)&&(identical(other.duration, duration) || other.duration == duration)&&(identical(other.title, title) || other.title == title)&&(identical(other.artist, artist) || other.artist == artist)&&(identical(other.album, album) || other.album == album)&&(identical(other.albumArtist, albumArtist) || other.albumArtist == albumArtist)&&(identical(other.genre, genre) || other.genre == genre)&&(identical(other.trackNumber, trackNumber) || other.trackNumber == trackNumber)&&(identical(other.discNumber, discNumber) || other.discNumber == discNumber)&&(identical(other.year, year) || other.year == year)&&(identical(other.composer, composer) || other.composer == composer)&&(identical(other.bitRate, bitRate) || other.bitRate == bitRate)&&(identical(other.sampleRate, sampleRate) || other.sampleRate == sampleRate)&&(identical(other.channelLayout, channelLayout) || other.channelLayout == channelLayout)&&(identical(other.bitDepth, bitDepth) || other.bitDepth == bitDepth)&&(identical(other.codecName, codecName) || other.codecName == codecName)&&(identical(other.aiGenre, aiGenre) || other.aiGenre == aiGenre)&&(identical(other.bpm, bpm) || other.bpm == bpm)&&(identical(other.key, key) || other.key == key)&&(identical(other.isMyLike, isMyLike) || other.isMyLike == isMyLike)&&(identical(other.comment, comment) || other.comment == comment)&&(identical(other.playNum, playNum) || other.playNum == playNum)&&(identical(other.hash, hash) || other.hash == hash));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hashAll([runtimeType,songId,duration,title,artist,album,albumArtist,genre,trackNumber,discNumber,year,composer,bitRate,sampleRate,channelLayout,bitDepth,codecName,aiGenre,bpm,key,isMyLike,comment,playNum,hash]);

@override
String toString() {
  return 'SongInfo(songId: $songId, duration: $duration, title: $title, artist: $artist, album: $album, albumArtist: $albumArtist, genre: $genre, trackNumber: $trackNumber, discNumber: $discNumber, year: $year, composer: $composer, bitRate: $bitRate, sampleRate: $sampleRate, channelLayout: $channelLayout, bitDepth: $bitDepth, codecName: $codecName, aiGenre: $aiGenre, bpm: $bpm, key: $key, isMyLike: $isMyLike, comment: $comment, playNum: $playNum, hash: $hash)';
}


}

/// @nodoc
abstract mixin class _$SongInfoCopyWith<$Res> implements $SongInfoCopyWith<$Res> {
  factory _$SongInfoCopyWith(_SongInfo value, $Res Function(_SongInfo) _then) = __$SongInfoCopyWithImpl;
@override @useResult
$Res call({
 int songId, double duration, String title, String? artist, String? album, String? albumArtist, String? genre, int? trackNumber, int? discNumber, int? year, String? composer, int bitRate, int sampleRate, String channelLayout, int bitDepth, String? codecName, String? aiGenre, int? bpm, String? key, bool isMyLike, String? comment, int playNum, String? hash
});




}
/// @nodoc
class __$SongInfoCopyWithImpl<$Res>
    implements _$SongInfoCopyWith<$Res> {
  __$SongInfoCopyWithImpl(this._self, this._then);

  final _SongInfo _self;
  final $Res Function(_SongInfo) _then;

/// Create a copy of SongInfo
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? songId = null,Object? duration = null,Object? title = null,Object? artist = freezed,Object? album = freezed,Object? albumArtist = freezed,Object? genre = freezed,Object? trackNumber = freezed,Object? discNumber = freezed,Object? year = freezed,Object? composer = freezed,Object? bitRate = null,Object? sampleRate = null,Object? channelLayout = null,Object? bitDepth = null,Object? codecName = freezed,Object? aiGenre = freezed,Object? bpm = freezed,Object? key = freezed,Object? isMyLike = null,Object? comment = freezed,Object? playNum = null,Object? hash = freezed,}) {
  return _then(_SongInfo(
songId: null == songId ? _self.songId : songId // ignore: cast_nullable_to_non_nullable
as int,duration: null == duration ? _self.duration : duration // ignore: cast_nullable_to_non_nullable
as double,title: null == title ? _self.title : title // ignore: cast_nullable_to_non_nullable
as String,artist: freezed == artist ? _self.artist : artist // ignore: cast_nullable_to_non_nullable
as String?,album: freezed == album ? _self.album : album // ignore: cast_nullable_to_non_nullable
as String?,albumArtist: freezed == albumArtist ? _self.albumArtist : albumArtist // ignore: cast_nullable_to_non_nullable
as String?,genre: freezed == genre ? _self.genre : genre // ignore: cast_nullable_to_non_nullable
as String?,trackNumber: freezed == trackNumber ? _self.trackNumber : trackNumber // ignore: cast_nullable_to_non_nullable
as int?,discNumber: freezed == discNumber ? _self.discNumber : discNumber // ignore: cast_nullable_to_non_nullable
as int?,year: freezed == year ? _self.year : year // ignore: cast_nullable_to_non_nullable
as int?,composer: freezed == composer ? _self.composer : composer // ignore: cast_nullable_to_non_nullable
as String?,bitRate: null == bitRate ? _self.bitRate : bitRate // ignore: cast_nullable_to_non_nullable
as int,sampleRate: null == sampleRate ? _self.sampleRate : sampleRate // ignore: cast_nullable_to_non_nullable
as int,channelLayout: null == channelLayout ? _self.channelLayout : channelLayout // ignore: cast_nullable_to_non_nullable
as String,bitDepth: null == bitDepth ? _self.bitDepth : bitDepth // ignore: cast_nullable_to_non_nullable
as int,codecName: freezed == codecName ? _self.codecName : codecName // ignore: cast_nullable_to_non_nullable
as String?,aiGenre: freezed == aiGenre ? _self.aiGenre : aiGenre // ignore: cast_nullable_to_non_nullable
as String?,bpm: freezed == bpm ? _self.bpm : bpm // ignore: cast_nullable_to_non_nullable
as int?,key: freezed == key ? _self.key : key // ignore: cast_nullable_to_non_nullable
as String?,isMyLike: null == isMyLike ? _self.isMyLike : isMyLike // ignore: cast_nullable_to_non_nullable
as bool,comment: freezed == comment ? _self.comment : comment // ignore: cast_nullable_to_non_nullable
as String?,playNum: null == playNum ? _self.playNum : playNum // ignore: cast_nullable_to_non_nullable
as int,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,
  ));
}


}

// dart format on
