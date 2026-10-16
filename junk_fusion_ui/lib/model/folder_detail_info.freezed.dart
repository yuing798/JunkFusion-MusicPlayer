// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'folder_detail_info.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$FolderDetailInfo {

/// 文件夹完整路径（唯一标识）
 String get path;/// 文件夹名
 String get name;/// 封面哈希：该文件夹里第一首有封面的歌，没有就是空串
 String? get hash;/// 这个文件夹里的歌曲 id
 List<int>? get songsLists;
/// Create a copy of FolderDetailInfo
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$FolderDetailInfoCopyWith<FolderDetailInfo> get copyWith => _$FolderDetailInfoCopyWithImpl<FolderDetailInfo>(this as FolderDetailInfo, _$identity);

  /// Serializes this FolderDetailInfo to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is FolderDetailInfo&&(identical(other.path, path) || other.path == path)&&(identical(other.name, name) || other.name == name)&&(identical(other.hash, hash) || other.hash == hash)&&const DeepCollectionEquality().equals(other.songsLists, songsLists));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,path,name,hash,const DeepCollectionEquality().hash(songsLists));

@override
String toString() {
  return 'FolderDetailInfo(path: $path, name: $name, hash: $hash, songsLists: $songsLists)';
}


}

/// @nodoc
abstract mixin class $FolderDetailInfoCopyWith<$Res>  {
  factory $FolderDetailInfoCopyWith(FolderDetailInfo value, $Res Function(FolderDetailInfo) _then) = _$FolderDetailInfoCopyWithImpl;
@useResult
$Res call({
 String path, String name, String? hash, List<int>? songsLists
});




}
/// @nodoc
class _$FolderDetailInfoCopyWithImpl<$Res>
    implements $FolderDetailInfoCopyWith<$Res> {
  _$FolderDetailInfoCopyWithImpl(this._self, this._then);

  final FolderDetailInfo _self;
  final $Res Function(FolderDetailInfo) _then;

/// Create a copy of FolderDetailInfo
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? path = null,Object? name = null,Object? hash = freezed,Object? songsLists = freezed,}) {
  return _then(_self.copyWith(
path: null == path ? _self.path : path // ignore: cast_nullable_to_non_nullable
as String,name: null == name ? _self.name : name // ignore: cast_nullable_to_non_nullable
as String,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,songsLists: freezed == songsLists ? _self.songsLists : songsLists // ignore: cast_nullable_to_non_nullable
as List<int>?,
  ));
}

}


/// Adds pattern-matching-related methods to [FolderDetailInfo].
extension FolderDetailInfoPatterns on FolderDetailInfo {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _FolderDetailInfo value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _FolderDetailInfo() when $default != null:
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

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _FolderDetailInfo value)  $default,){
final _that = this;
switch (_that) {
case _FolderDetailInfo():
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _FolderDetailInfo value)?  $default,){
final _that = this;
switch (_that) {
case _FolderDetailInfo() when $default != null:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( String path,  String name,  String? hash,  List<int>? songsLists)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _FolderDetailInfo() when $default != null:
return $default(_that.path,_that.name,_that.hash,_that.songsLists);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( String path,  String name,  String? hash,  List<int>? songsLists)  $default,) {final _that = this;
switch (_that) {
case _FolderDetailInfo():
return $default(_that.path,_that.name,_that.hash,_that.songsLists);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( String path,  String name,  String? hash,  List<int>? songsLists)?  $default,) {final _that = this;
switch (_that) {
case _FolderDetailInfo() when $default != null:
return $default(_that.path,_that.name,_that.hash,_that.songsLists);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _FolderDetailInfo implements FolderDetailInfo {
  const _FolderDetailInfo({required this.path, required this.name, this.hash, final  List<int>? songsLists}): _songsLists = songsLists;
  factory _FolderDetailInfo.fromJson(Map<String, dynamic> json) => _$FolderDetailInfoFromJson(json);

/// 文件夹完整路径（唯一标识）
@override final  String path;
/// 文件夹名
@override final  String name;
/// 封面哈希：该文件夹里第一首有封面的歌，没有就是空串
@override final  String? hash;
/// 这个文件夹里的歌曲 id
 final  List<int>? _songsLists;
/// 这个文件夹里的歌曲 id
@override List<int>? get songsLists {
  final value = _songsLists;
  if (value == null) return null;
  if (_songsLists is EqualUnmodifiableListView) return _songsLists;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(value);
}


/// Create a copy of FolderDetailInfo
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$FolderDetailInfoCopyWith<_FolderDetailInfo> get copyWith => __$FolderDetailInfoCopyWithImpl<_FolderDetailInfo>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$FolderDetailInfoToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _FolderDetailInfo&&(identical(other.path, path) || other.path == path)&&(identical(other.name, name) || other.name == name)&&(identical(other.hash, hash) || other.hash == hash)&&const DeepCollectionEquality().equals(other._songsLists, _songsLists));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,path,name,hash,const DeepCollectionEquality().hash(_songsLists));

@override
String toString() {
  return 'FolderDetailInfo(path: $path, name: $name, hash: $hash, songsLists: $songsLists)';
}


}

/// @nodoc
abstract mixin class _$FolderDetailInfoCopyWith<$Res> implements $FolderDetailInfoCopyWith<$Res> {
  factory _$FolderDetailInfoCopyWith(_FolderDetailInfo value, $Res Function(_FolderDetailInfo) _then) = __$FolderDetailInfoCopyWithImpl;
@override @useResult
$Res call({
 String path, String name, String? hash, List<int>? songsLists
});




}
/// @nodoc
class __$FolderDetailInfoCopyWithImpl<$Res>
    implements _$FolderDetailInfoCopyWith<$Res> {
  __$FolderDetailInfoCopyWithImpl(this._self, this._then);

  final _FolderDetailInfo _self;
  final $Res Function(_FolderDetailInfo) _then;

/// Create a copy of FolderDetailInfo
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? path = null,Object? name = null,Object? hash = freezed,Object? songsLists = freezed,}) {
  return _then(_FolderDetailInfo(
path: null == path ? _self.path : path // ignore: cast_nullable_to_non_nullable
as String,name: null == name ? _self.name : name // ignore: cast_nullable_to_non_nullable
as String,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,songsLists: freezed == songsLists ? _self._songsLists : songsLists // ignore: cast_nullable_to_non_nullable
as List<int>?,
  ));
}


}

// dart format on
