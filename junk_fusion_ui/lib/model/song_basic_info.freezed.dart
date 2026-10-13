// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'song_basic_info.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$SongBasicInfo {

 int get songId; double get duration; String get title; List<String>? get artists; String? get album; bool get isMyLike; int get playNum; String? get hash;
/// Create a copy of SongBasicInfo
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$SongBasicInfoCopyWith<SongBasicInfo> get copyWith => _$SongBasicInfoCopyWithImpl<SongBasicInfo>(this as SongBasicInfo, _$identity);

  /// Serializes this SongBasicInfo to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is SongBasicInfo&&(identical(other.songId, songId) || other.songId == songId)&&(identical(other.duration, duration) || other.duration == duration)&&(identical(other.title, title) || other.title == title)&&const DeepCollectionEquality().equals(other.artists, artists)&&(identical(other.album, album) || other.album == album)&&(identical(other.isMyLike, isMyLike) || other.isMyLike == isMyLike)&&(identical(other.playNum, playNum) || other.playNum == playNum)&&(identical(other.hash, hash) || other.hash == hash));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,songId,duration,title,const DeepCollectionEquality().hash(artists),album,isMyLike,playNum,hash);

@override
String toString() {
  return 'SongBasicInfo(songId: $songId, duration: $duration, title: $title, artists: $artists, album: $album, isMyLike: $isMyLike, playNum: $playNum, hash: $hash)';
}


}

/// @nodoc
abstract mixin class $SongBasicInfoCopyWith<$Res>  {
  factory $SongBasicInfoCopyWith(SongBasicInfo value, $Res Function(SongBasicInfo) _then) = _$SongBasicInfoCopyWithImpl;
@useResult
$Res call({
 int songId, double duration, String title, List<String>? artists, String? album, bool isMyLike, int playNum, String? hash
});




}
/// @nodoc
class _$SongBasicInfoCopyWithImpl<$Res>
    implements $SongBasicInfoCopyWith<$Res> {
  _$SongBasicInfoCopyWithImpl(this._self, this._then);

  final SongBasicInfo _self;
  final $Res Function(SongBasicInfo) _then;

/// Create a copy of SongBasicInfo
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? songId = null,Object? duration = null,Object? title = null,Object? artists = freezed,Object? album = freezed,Object? isMyLike = null,Object? playNum = null,Object? hash = freezed,}) {
  return _then(_self.copyWith(
songId: null == songId ? _self.songId : songId // ignore: cast_nullable_to_non_nullable
as int,duration: null == duration ? _self.duration : duration // ignore: cast_nullable_to_non_nullable
as double,title: null == title ? _self.title : title // ignore: cast_nullable_to_non_nullable
as String,artists: freezed == artists ? _self.artists : artists // ignore: cast_nullable_to_non_nullable
as List<String>?,album: freezed == album ? _self.album : album // ignore: cast_nullable_to_non_nullable
as String?,isMyLike: null == isMyLike ? _self.isMyLike : isMyLike // ignore: cast_nullable_to_non_nullable
as bool,playNum: null == playNum ? _self.playNum : playNum // ignore: cast_nullable_to_non_nullable
as int,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,
  ));
}

}


/// Adds pattern-matching-related methods to [SongBasicInfo].
extension SongBasicInfoPatterns on SongBasicInfo {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _SongBasicInfo value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _SongBasicInfo() when $default != null:
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

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _SongBasicInfo value)  $default,){
final _that = this;
switch (_that) {
case _SongBasicInfo():
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _SongBasicInfo value)?  $default,){
final _that = this;
switch (_that) {
case _SongBasicInfo() when $default != null:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int songId,  double duration,  String title,  List<String>? artists,  String? album,  bool isMyLike,  int playNum,  String? hash)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _SongBasicInfo() when $default != null:
return $default(_that.songId,_that.duration,_that.title,_that.artists,_that.album,_that.isMyLike,_that.playNum,_that.hash);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int songId,  double duration,  String title,  List<String>? artists,  String? album,  bool isMyLike,  int playNum,  String? hash)  $default,) {final _that = this;
switch (_that) {
case _SongBasicInfo():
return $default(_that.songId,_that.duration,_that.title,_that.artists,_that.album,_that.isMyLike,_that.playNum,_that.hash);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int songId,  double duration,  String title,  List<String>? artists,  String? album,  bool isMyLike,  int playNum,  String? hash)?  $default,) {final _that = this;
switch (_that) {
case _SongBasicInfo() when $default != null:
return $default(_that.songId,_that.duration,_that.title,_that.artists,_that.album,_that.isMyLike,_that.playNum,_that.hash);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _SongBasicInfo implements SongBasicInfo {
  const _SongBasicInfo({required this.songId, required this.duration, required this.title, required final  List<String>? artists, this.album, required this.isMyLike, required this.playNum, this.hash}): _artists = artists;
  factory _SongBasicInfo.fromJson(Map<String, dynamic> json) => _$SongBasicInfoFromJson(json);

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
@override final  bool isMyLike;
@override final  int playNum;
@override final  String? hash;

/// Create a copy of SongBasicInfo
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$SongBasicInfoCopyWith<_SongBasicInfo> get copyWith => __$SongBasicInfoCopyWithImpl<_SongBasicInfo>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$SongBasicInfoToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _SongBasicInfo&&(identical(other.songId, songId) || other.songId == songId)&&(identical(other.duration, duration) || other.duration == duration)&&(identical(other.title, title) || other.title == title)&&const DeepCollectionEquality().equals(other._artists, _artists)&&(identical(other.album, album) || other.album == album)&&(identical(other.isMyLike, isMyLike) || other.isMyLike == isMyLike)&&(identical(other.playNum, playNum) || other.playNum == playNum)&&(identical(other.hash, hash) || other.hash == hash));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,songId,duration,title,const DeepCollectionEquality().hash(_artists),album,isMyLike,playNum,hash);

@override
String toString() {
  return 'SongBasicInfo(songId: $songId, duration: $duration, title: $title, artists: $artists, album: $album, isMyLike: $isMyLike, playNum: $playNum, hash: $hash)';
}


}

/// @nodoc
abstract mixin class _$SongBasicInfoCopyWith<$Res> implements $SongBasicInfoCopyWith<$Res> {
  factory _$SongBasicInfoCopyWith(_SongBasicInfo value, $Res Function(_SongBasicInfo) _then) = __$SongBasicInfoCopyWithImpl;
@override @useResult
$Res call({
 int songId, double duration, String title, List<String>? artists, String? album, bool isMyLike, int playNum, String? hash
});




}
/// @nodoc
class __$SongBasicInfoCopyWithImpl<$Res>
    implements _$SongBasicInfoCopyWith<$Res> {
  __$SongBasicInfoCopyWithImpl(this._self, this._then);

  final _SongBasicInfo _self;
  final $Res Function(_SongBasicInfo) _then;

/// Create a copy of SongBasicInfo
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? songId = null,Object? duration = null,Object? title = null,Object? artists = freezed,Object? album = freezed,Object? isMyLike = null,Object? playNum = null,Object? hash = freezed,}) {
  return _then(_SongBasicInfo(
songId: null == songId ? _self.songId : songId // ignore: cast_nullable_to_non_nullable
as int,duration: null == duration ? _self.duration : duration // ignore: cast_nullable_to_non_nullable
as double,title: null == title ? _self.title : title // ignore: cast_nullable_to_non_nullable
as String,artists: freezed == artists ? _self._artists : artists // ignore: cast_nullable_to_non_nullable
as List<String>?,album: freezed == album ? _self.album : album // ignore: cast_nullable_to_non_nullable
as String?,isMyLike: null == isMyLike ? _self.isMyLike : isMyLike // ignore: cast_nullable_to_non_nullable
as bool,playNum: null == playNum ? _self.playNum : playNum // ignore: cast_nullable_to_non_nullable
as int,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,
  ));
}


}

// dart format on
