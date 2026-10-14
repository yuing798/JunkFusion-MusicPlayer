// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'album_detail_info.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$AlbumDetailInfo {

 int get albumId; String get title; List<String>? get albumArtists; String? get hash; List<int>? get songsList;
/// Create a copy of AlbumDetailInfo
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$AlbumDetailInfoCopyWith<AlbumDetailInfo> get copyWith => _$AlbumDetailInfoCopyWithImpl<AlbumDetailInfo>(this as AlbumDetailInfo, _$identity);

  /// Serializes this AlbumDetailInfo to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is AlbumDetailInfo&&(identical(other.albumId, albumId) || other.albumId == albumId)&&(identical(other.title, title) || other.title == title)&&const DeepCollectionEquality().equals(other.albumArtists, albumArtists)&&(identical(other.hash, hash) || other.hash == hash)&&const DeepCollectionEquality().equals(other.songsList, songsList));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,albumId,title,const DeepCollectionEquality().hash(albumArtists),hash,const DeepCollectionEquality().hash(songsList));

@override
String toString() {
  return 'AlbumDetailInfo(albumId: $albumId, title: $title, albumArtists: $albumArtists, hash: $hash, songsList: $songsList)';
}


}

/// @nodoc
abstract mixin class $AlbumDetailInfoCopyWith<$Res>  {
  factory $AlbumDetailInfoCopyWith(AlbumDetailInfo value, $Res Function(AlbumDetailInfo) _then) = _$AlbumDetailInfoCopyWithImpl;
@useResult
$Res call({
 int albumId, String title, List<String>? albumArtists, String? hash, List<int>? songsList
});




}
/// @nodoc
class _$AlbumDetailInfoCopyWithImpl<$Res>
    implements $AlbumDetailInfoCopyWith<$Res> {
  _$AlbumDetailInfoCopyWithImpl(this._self, this._then);

  final AlbumDetailInfo _self;
  final $Res Function(AlbumDetailInfo) _then;

/// Create a copy of AlbumDetailInfo
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? albumId = null,Object? title = null,Object? albumArtists = freezed,Object? hash = freezed,Object? songsList = freezed,}) {
  return _then(_self.copyWith(
albumId: null == albumId ? _self.albumId : albumId // ignore: cast_nullable_to_non_nullable
as int,title: null == title ? _self.title : title // ignore: cast_nullable_to_non_nullable
as String,albumArtists: freezed == albumArtists ? _self.albumArtists : albumArtists // ignore: cast_nullable_to_non_nullable
as List<String>?,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,songsList: freezed == songsList ? _self.songsList : songsList // ignore: cast_nullable_to_non_nullable
as List<int>?,
  ));
}

}


/// Adds pattern-matching-related methods to [AlbumDetailInfo].
extension AlbumDetailInfoPatterns on AlbumDetailInfo {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _AlbumDetailInfo value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _AlbumDetailInfo() when $default != null:
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

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _AlbumDetailInfo value)  $default,){
final _that = this;
switch (_that) {
case _AlbumDetailInfo():
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _AlbumDetailInfo value)?  $default,){
final _that = this;
switch (_that) {
case _AlbumDetailInfo() when $default != null:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int albumId,  String title,  List<String>? albumArtists,  String? hash,  List<int>? songsList)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _AlbumDetailInfo() when $default != null:
return $default(_that.albumId,_that.title,_that.albumArtists,_that.hash,_that.songsList);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int albumId,  String title,  List<String>? albumArtists,  String? hash,  List<int>? songsList)  $default,) {final _that = this;
switch (_that) {
case _AlbumDetailInfo():
return $default(_that.albumId,_that.title,_that.albumArtists,_that.hash,_that.songsList);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int albumId,  String title,  List<String>? albumArtists,  String? hash,  List<int>? songsList)?  $default,) {final _that = this;
switch (_that) {
case _AlbumDetailInfo() when $default != null:
return $default(_that.albumId,_that.title,_that.albumArtists,_that.hash,_that.songsList);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _AlbumDetailInfo implements AlbumDetailInfo {
  const _AlbumDetailInfo({required this.albumId, required this.title, final  List<String>? albumArtists, this.hash, final  List<int>? songsList}): _albumArtists = albumArtists,_songsList = songsList;
  factory _AlbumDetailInfo.fromJson(Map<String, dynamic> json) => _$AlbumDetailInfoFromJson(json);

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
 final  List<int>? _songsList;
@override List<int>? get songsList {
  final value = _songsList;
  if (value == null) return null;
  if (_songsList is EqualUnmodifiableListView) return _songsList;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(value);
}


/// Create a copy of AlbumDetailInfo
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$AlbumDetailInfoCopyWith<_AlbumDetailInfo> get copyWith => __$AlbumDetailInfoCopyWithImpl<_AlbumDetailInfo>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$AlbumDetailInfoToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _AlbumDetailInfo&&(identical(other.albumId, albumId) || other.albumId == albumId)&&(identical(other.title, title) || other.title == title)&&const DeepCollectionEquality().equals(other._albumArtists, _albumArtists)&&(identical(other.hash, hash) || other.hash == hash)&&const DeepCollectionEquality().equals(other._songsList, _songsList));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,albumId,title,const DeepCollectionEquality().hash(_albumArtists),hash,const DeepCollectionEquality().hash(_songsList));

@override
String toString() {
  return 'AlbumDetailInfo(albumId: $albumId, title: $title, albumArtists: $albumArtists, hash: $hash, songsList: $songsList)';
}


}

/// @nodoc
abstract mixin class _$AlbumDetailInfoCopyWith<$Res> implements $AlbumDetailInfoCopyWith<$Res> {
  factory _$AlbumDetailInfoCopyWith(_AlbumDetailInfo value, $Res Function(_AlbumDetailInfo) _then) = __$AlbumDetailInfoCopyWithImpl;
@override @useResult
$Res call({
 int albumId, String title, List<String>? albumArtists, String? hash, List<int>? songsList
});




}
/// @nodoc
class __$AlbumDetailInfoCopyWithImpl<$Res>
    implements _$AlbumDetailInfoCopyWith<$Res> {
  __$AlbumDetailInfoCopyWithImpl(this._self, this._then);

  final _AlbumDetailInfo _self;
  final $Res Function(_AlbumDetailInfo) _then;

/// Create a copy of AlbumDetailInfo
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? albumId = null,Object? title = null,Object? albumArtists = freezed,Object? hash = freezed,Object? songsList = freezed,}) {
  return _then(_AlbumDetailInfo(
albumId: null == albumId ? _self.albumId : albumId // ignore: cast_nullable_to_non_nullable
as int,title: null == title ? _self.title : title // ignore: cast_nullable_to_non_nullable
as String,albumArtists: freezed == albumArtists ? _self._albumArtists : albumArtists // ignore: cast_nullable_to_non_nullable
as List<String>?,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,songsList: freezed == songsList ? _self._songsList : songsList // ignore: cast_nullable_to_non_nullable
as List<int>?,
  ));
}


}

// dart format on
