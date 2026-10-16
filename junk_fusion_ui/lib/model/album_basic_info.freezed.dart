// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'album_basic_info.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$AlbumBasicInfo {

 int get albumId; String get title; List<String>? get albumArtists; String? get hash;
/// Create a copy of AlbumBasicInfo
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$AlbumBasicInfoCopyWith<AlbumBasicInfo> get copyWith => _$AlbumBasicInfoCopyWithImpl<AlbumBasicInfo>(this as AlbumBasicInfo, _$identity);

  /// Serializes this AlbumBasicInfo to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is AlbumBasicInfo&&(identical(other.albumId, albumId) || other.albumId == albumId)&&(identical(other.title, title) || other.title == title)&&const DeepCollectionEquality().equals(other.albumArtists, albumArtists)&&(identical(other.hash, hash) || other.hash == hash));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,albumId,title,const DeepCollectionEquality().hash(albumArtists),hash);

@override
String toString() {
  return 'AlbumBasicInfo(albumId: $albumId, title: $title, albumArtists: $albumArtists, hash: $hash)';
}


}

/// @nodoc
abstract mixin class $AlbumBasicInfoCopyWith<$Res>  {
  factory $AlbumBasicInfoCopyWith(AlbumBasicInfo value, $Res Function(AlbumBasicInfo) _then) = _$AlbumBasicInfoCopyWithImpl;
@useResult
$Res call({
 int albumId, String title, List<String>? albumArtists, String? hash
});




}
/// @nodoc
class _$AlbumBasicInfoCopyWithImpl<$Res>
    implements $AlbumBasicInfoCopyWith<$Res> {
  _$AlbumBasicInfoCopyWithImpl(this._self, this._then);

  final AlbumBasicInfo _self;
  final $Res Function(AlbumBasicInfo) _then;

/// Create a copy of AlbumBasicInfo
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? albumId = null,Object? title = null,Object? albumArtists = freezed,Object? hash = freezed,}) {
  return _then(_self.copyWith(
albumId: null == albumId ? _self.albumId : albumId // ignore: cast_nullable_to_non_nullable
as int,title: null == title ? _self.title : title // ignore: cast_nullable_to_non_nullable
as String,albumArtists: freezed == albumArtists ? _self.albumArtists : albumArtists // ignore: cast_nullable_to_non_nullable
as List<String>?,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,
  ));
}

}


/// Adds pattern-matching-related methods to [AlbumBasicInfo].
extension AlbumBasicInfoPatterns on AlbumBasicInfo {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _AlbumBasicInfo value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _AlbumBasicInfo() when $default != null:
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

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _AlbumBasicInfo value)  $default,){
final _that = this;
switch (_that) {
case _AlbumBasicInfo():
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _AlbumBasicInfo value)?  $default,){
final _that = this;
switch (_that) {
case _AlbumBasicInfo() when $default != null:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int albumId,  String title,  List<String>? albumArtists,  String? hash)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _AlbumBasicInfo() when $default != null:
return $default(_that.albumId,_that.title,_that.albumArtists,_that.hash);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int albumId,  String title,  List<String>? albumArtists,  String? hash)  $default,) {final _that = this;
switch (_that) {
case _AlbumBasicInfo():
return $default(_that.albumId,_that.title,_that.albumArtists,_that.hash);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int albumId,  String title,  List<String>? albumArtists,  String? hash)?  $default,) {final _that = this;
switch (_that) {
case _AlbumBasicInfo() when $default != null:
return $default(_that.albumId,_that.title,_that.albumArtists,_that.hash);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _AlbumBasicInfo implements AlbumBasicInfo {
  const _AlbumBasicInfo({required this.albumId, required this.title, final  List<String>? albumArtists, this.hash}): _albumArtists = albumArtists;
  factory _AlbumBasicInfo.fromJson(Map<String, dynamic> json) => _$AlbumBasicInfoFromJson(json);

@override final  int albumId;
@override final  String title;
 final  List<String>? _albumArtists;
@override List<String>? get albumArtists {
  final value = _albumArtists;
  if (value == null) return null;
  if (_albumArtists is EqualUnmodifiableListView) return _albumArtists;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(value);
}

@override final  String? hash;

/// Create a copy of AlbumBasicInfo
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$AlbumBasicInfoCopyWith<_AlbumBasicInfo> get copyWith => __$AlbumBasicInfoCopyWithImpl<_AlbumBasicInfo>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$AlbumBasicInfoToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _AlbumBasicInfo&&(identical(other.albumId, albumId) || other.albumId == albumId)&&(identical(other.title, title) || other.title == title)&&const DeepCollectionEquality().equals(other._albumArtists, _albumArtists)&&(identical(other.hash, hash) || other.hash == hash));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,albumId,title,const DeepCollectionEquality().hash(_albumArtists),hash);

@override
String toString() {
  return 'AlbumBasicInfo(albumId: $albumId, title: $title, albumArtists: $albumArtists, hash: $hash)';
}


}

/// @nodoc
abstract mixin class _$AlbumBasicInfoCopyWith<$Res> implements $AlbumBasicInfoCopyWith<$Res> {
  factory _$AlbumBasicInfoCopyWith(_AlbumBasicInfo value, $Res Function(_AlbumBasicInfo) _then) = __$AlbumBasicInfoCopyWithImpl;
@override @useResult
$Res call({
 int albumId, String title, List<String>? albumArtists, String? hash
});




}
/// @nodoc
class __$AlbumBasicInfoCopyWithImpl<$Res>
    implements _$AlbumBasicInfoCopyWith<$Res> {
  __$AlbumBasicInfoCopyWithImpl(this._self, this._then);

  final _AlbumBasicInfo _self;
  final $Res Function(_AlbumBasicInfo) _then;

/// Create a copy of AlbumBasicInfo
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? albumId = null,Object? title = null,Object? albumArtists = freezed,Object? hash = freezed,}) {
  return _then(_AlbumBasicInfo(
albumId: null == albumId ? _self.albumId : albumId // ignore: cast_nullable_to_non_nullable
as int,title: null == title ? _self.title : title // ignore: cast_nullable_to_non_nullable
as String,albumArtists: freezed == albumArtists ? _self._albumArtists : albumArtists // ignore: cast_nullable_to_non_nullable
as List<String>?,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,
  ));
}


}

// dart format on
