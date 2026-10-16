// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'playlist_basic_info.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$PlaylistBasicInfo {

 int get playlistId; String get title;/// 歌单里的歌曲数量（网格卡片上显示，也用来排序）
 int get songCount;/// 封面哈希：取歌单里第一首有封面的歌（和专辑一个做法），没有就是空串
 String? get hash;
/// Create a copy of PlaylistBasicInfo
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$PlaylistBasicInfoCopyWith<PlaylistBasicInfo> get copyWith => _$PlaylistBasicInfoCopyWithImpl<PlaylistBasicInfo>(this as PlaylistBasicInfo, _$identity);

  /// Serializes this PlaylistBasicInfo to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is PlaylistBasicInfo&&(identical(other.playlistId, playlistId) || other.playlistId == playlistId)&&(identical(other.title, title) || other.title == title)&&(identical(other.songCount, songCount) || other.songCount == songCount)&&(identical(other.hash, hash) || other.hash == hash));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,playlistId,title,songCount,hash);

@override
String toString() {
  return 'PlaylistBasicInfo(playlistId: $playlistId, title: $title, songCount: $songCount, hash: $hash)';
}


}

/// @nodoc
abstract mixin class $PlaylistBasicInfoCopyWith<$Res>  {
  factory $PlaylistBasicInfoCopyWith(PlaylistBasicInfo value, $Res Function(PlaylistBasicInfo) _then) = _$PlaylistBasicInfoCopyWithImpl;
@useResult
$Res call({
 int playlistId, String title, int songCount, String? hash
});




}
/// @nodoc
class _$PlaylistBasicInfoCopyWithImpl<$Res>
    implements $PlaylistBasicInfoCopyWith<$Res> {
  _$PlaylistBasicInfoCopyWithImpl(this._self, this._then);

  final PlaylistBasicInfo _self;
  final $Res Function(PlaylistBasicInfo) _then;

/// Create a copy of PlaylistBasicInfo
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? playlistId = null,Object? title = null,Object? songCount = null,Object? hash = freezed,}) {
  return _then(_self.copyWith(
playlistId: null == playlistId ? _self.playlistId : playlistId // ignore: cast_nullable_to_non_nullable
as int,title: null == title ? _self.title : title // ignore: cast_nullable_to_non_nullable
as String,songCount: null == songCount ? _self.songCount : songCount // ignore: cast_nullable_to_non_nullable
as int,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,
  ));
}

}


/// Adds pattern-matching-related methods to [PlaylistBasicInfo].
extension PlaylistBasicInfoPatterns on PlaylistBasicInfo {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _PlaylistBasicInfo value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _PlaylistBasicInfo() when $default != null:
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

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _PlaylistBasicInfo value)  $default,){
final _that = this;
switch (_that) {
case _PlaylistBasicInfo():
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _PlaylistBasicInfo value)?  $default,){
final _that = this;
switch (_that) {
case _PlaylistBasicInfo() when $default != null:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int playlistId,  String title,  int songCount,  String? hash)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _PlaylistBasicInfo() when $default != null:
return $default(_that.playlistId,_that.title,_that.songCount,_that.hash);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int playlistId,  String title,  int songCount,  String? hash)  $default,) {final _that = this;
switch (_that) {
case _PlaylistBasicInfo():
return $default(_that.playlistId,_that.title,_that.songCount,_that.hash);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int playlistId,  String title,  int songCount,  String? hash)?  $default,) {final _that = this;
switch (_that) {
case _PlaylistBasicInfo() when $default != null:
return $default(_that.playlistId,_that.title,_that.songCount,_that.hash);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _PlaylistBasicInfo implements PlaylistBasicInfo {
  const _PlaylistBasicInfo({required this.playlistId, required this.title, required this.songCount, this.hash});
  factory _PlaylistBasicInfo.fromJson(Map<String, dynamic> json) => _$PlaylistBasicInfoFromJson(json);

@override final  int playlistId;
@override final  String title;
/// 歌单里的歌曲数量（网格卡片上显示，也用来排序）
@override final  int songCount;
/// 封面哈希：取歌单里第一首有封面的歌（和专辑一个做法），没有就是空串
@override final  String? hash;

/// Create a copy of PlaylistBasicInfo
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$PlaylistBasicInfoCopyWith<_PlaylistBasicInfo> get copyWith => __$PlaylistBasicInfoCopyWithImpl<_PlaylistBasicInfo>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$PlaylistBasicInfoToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _PlaylistBasicInfo&&(identical(other.playlistId, playlistId) || other.playlistId == playlistId)&&(identical(other.title, title) || other.title == title)&&(identical(other.songCount, songCount) || other.songCount == songCount)&&(identical(other.hash, hash) || other.hash == hash));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,playlistId,title,songCount,hash);

@override
String toString() {
  return 'PlaylistBasicInfo(playlistId: $playlistId, title: $title, songCount: $songCount, hash: $hash)';
}


}

/// @nodoc
abstract mixin class _$PlaylistBasicInfoCopyWith<$Res> implements $PlaylistBasicInfoCopyWith<$Res> {
  factory _$PlaylistBasicInfoCopyWith(_PlaylistBasicInfo value, $Res Function(_PlaylistBasicInfo) _then) = __$PlaylistBasicInfoCopyWithImpl;
@override @useResult
$Res call({
 int playlistId, String title, int songCount, String? hash
});




}
/// @nodoc
class __$PlaylistBasicInfoCopyWithImpl<$Res>
    implements _$PlaylistBasicInfoCopyWith<$Res> {
  __$PlaylistBasicInfoCopyWithImpl(this._self, this._then);

  final _PlaylistBasicInfo _self;
  final $Res Function(_PlaylistBasicInfo) _then;

/// Create a copy of PlaylistBasicInfo
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? playlistId = null,Object? title = null,Object? songCount = null,Object? hash = freezed,}) {
  return _then(_PlaylistBasicInfo(
playlistId: null == playlistId ? _self.playlistId : playlistId // ignore: cast_nullable_to_non_nullable
as int,title: null == title ? _self.title : title // ignore: cast_nullable_to_non_nullable
as String,songCount: null == songCount ? _self.songCount : songCount // ignore: cast_nullable_to_non_nullable
as int,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,
  ));
}


}

// dart format on
