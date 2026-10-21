// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'playlist_detail_info.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$PlaylistDetailInfo {

/// 歌单里的歌曲 id（按歌单内的顺序）
 List<int> get songsLists;
/// Create a copy of PlaylistDetailInfo
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$PlaylistDetailInfoCopyWith<PlaylistDetailInfo> get copyWith => _$PlaylistDetailInfoCopyWithImpl<PlaylistDetailInfo>(this as PlaylistDetailInfo, _$identity);

  /// Serializes this PlaylistDetailInfo to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is PlaylistDetailInfo&&const DeepCollectionEquality().equals(other.songsLists, songsLists));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,const DeepCollectionEquality().hash(songsLists));

@override
String toString() {
  return 'PlaylistDetailInfo(songsLists: $songsLists)';
}


}

/// @nodoc
abstract mixin class $PlaylistDetailInfoCopyWith<$Res>  {
  factory $PlaylistDetailInfoCopyWith(PlaylistDetailInfo value, $Res Function(PlaylistDetailInfo) _then) = _$PlaylistDetailInfoCopyWithImpl;
@useResult
$Res call({
 List<int> songsLists
});




}
/// @nodoc
class _$PlaylistDetailInfoCopyWithImpl<$Res>
    implements $PlaylistDetailInfoCopyWith<$Res> {
  _$PlaylistDetailInfoCopyWithImpl(this._self, this._then);

  final PlaylistDetailInfo _self;
  final $Res Function(PlaylistDetailInfo) _then;

/// Create a copy of PlaylistDetailInfo
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? songsLists = null,}) {
  return _then(_self.copyWith(
songsLists: null == songsLists ? _self.songsLists : songsLists // ignore: cast_nullable_to_non_nullable
as List<int>,
  ));
}

}


/// Adds pattern-matching-related methods to [PlaylistDetailInfo].
extension PlaylistDetailInfoPatterns on PlaylistDetailInfo {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _PlaylistDetailInfo value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _PlaylistDetailInfo() when $default != null:
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

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _PlaylistDetailInfo value)  $default,){
final _that = this;
switch (_that) {
case _PlaylistDetailInfo():
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _PlaylistDetailInfo value)?  $default,){
final _that = this;
switch (_that) {
case _PlaylistDetailInfo() when $default != null:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( List<int> songsLists)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _PlaylistDetailInfo() when $default != null:
return $default(_that.songsLists);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( List<int> songsLists)  $default,) {final _that = this;
switch (_that) {
case _PlaylistDetailInfo():
return $default(_that.songsLists);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( List<int> songsLists)?  $default,) {final _that = this;
switch (_that) {
case _PlaylistDetailInfo() when $default != null:
return $default(_that.songsLists);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _PlaylistDetailInfo implements PlaylistDetailInfo {
  const _PlaylistDetailInfo({required final  List<int> songsLists}): _songsLists = songsLists;
  factory _PlaylistDetailInfo.fromJson(Map<String, dynamic> json) => _$PlaylistDetailInfoFromJson(json);

/// 歌单里的歌曲 id（按歌单内的顺序）
 final  List<int> _songsLists;
/// 歌单里的歌曲 id（按歌单内的顺序）
@override List<int> get songsLists {
  if (_songsLists is EqualUnmodifiableListView) return _songsLists;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(_songsLists);
}


/// Create a copy of PlaylistDetailInfo
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$PlaylistDetailInfoCopyWith<_PlaylistDetailInfo> get copyWith => __$PlaylistDetailInfoCopyWithImpl<_PlaylistDetailInfo>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$PlaylistDetailInfoToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _PlaylistDetailInfo&&const DeepCollectionEquality().equals(other._songsLists, _songsLists));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,const DeepCollectionEquality().hash(_songsLists));

@override
String toString() {
  return 'PlaylistDetailInfo(songsLists: $songsLists)';
}


}

/// @nodoc
abstract mixin class _$PlaylistDetailInfoCopyWith<$Res> implements $PlaylistDetailInfoCopyWith<$Res> {
  factory _$PlaylistDetailInfoCopyWith(_PlaylistDetailInfo value, $Res Function(_PlaylistDetailInfo) _then) = __$PlaylistDetailInfoCopyWithImpl;
@override @useResult
$Res call({
 List<int> songsLists
});




}
/// @nodoc
class __$PlaylistDetailInfoCopyWithImpl<$Res>
    implements _$PlaylistDetailInfoCopyWith<$Res> {
  __$PlaylistDetailInfoCopyWithImpl(this._self, this._then);

  final _PlaylistDetailInfo _self;
  final $Res Function(_PlaylistDetailInfo) _then;

/// Create a copy of PlaylistDetailInfo
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? songsLists = null,}) {
  return _then(_PlaylistDetailInfo(
songsLists: null == songsLists ? _self._songsLists : songsLists // ignore: cast_nullable_to_non_nullable
as List<int>,
  ));
}


}

// dart format on
