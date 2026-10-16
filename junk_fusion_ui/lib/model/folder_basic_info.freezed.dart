// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'folder_basic_info.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$FolderBasicInfo {

/// 文件夹完整路径（唯一标识，也是 ValueKey / 路由参数）
 String get path;/// 文件夹名（只显示最后一级）
 String get name;/// 这个文件夹下（含子文件夹）有多少首歌
 int get songCount;/// 封面哈希：取该文件夹里第一首有封面的歌，没有就是空串
 String? get hash;
/// Create a copy of FolderBasicInfo
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$FolderBasicInfoCopyWith<FolderBasicInfo> get copyWith => _$FolderBasicInfoCopyWithImpl<FolderBasicInfo>(this as FolderBasicInfo, _$identity);

  /// Serializes this FolderBasicInfo to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is FolderBasicInfo&&(identical(other.path, path) || other.path == path)&&(identical(other.name, name) || other.name == name)&&(identical(other.songCount, songCount) || other.songCount == songCount)&&(identical(other.hash, hash) || other.hash == hash));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,path,name,songCount,hash);

@override
String toString() {
  return 'FolderBasicInfo(path: $path, name: $name, songCount: $songCount, hash: $hash)';
}


}

/// @nodoc
abstract mixin class $FolderBasicInfoCopyWith<$Res>  {
  factory $FolderBasicInfoCopyWith(FolderBasicInfo value, $Res Function(FolderBasicInfo) _then) = _$FolderBasicInfoCopyWithImpl;
@useResult
$Res call({
 String path, String name, int songCount, String? hash
});




}
/// @nodoc
class _$FolderBasicInfoCopyWithImpl<$Res>
    implements $FolderBasicInfoCopyWith<$Res> {
  _$FolderBasicInfoCopyWithImpl(this._self, this._then);

  final FolderBasicInfo _self;
  final $Res Function(FolderBasicInfo) _then;

/// Create a copy of FolderBasicInfo
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? path = null,Object? name = null,Object? songCount = null,Object? hash = freezed,}) {
  return _then(_self.copyWith(
path: null == path ? _self.path : path // ignore: cast_nullable_to_non_nullable
as String,name: null == name ? _self.name : name // ignore: cast_nullable_to_non_nullable
as String,songCount: null == songCount ? _self.songCount : songCount // ignore: cast_nullable_to_non_nullable
as int,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,
  ));
}

}


/// Adds pattern-matching-related methods to [FolderBasicInfo].
extension FolderBasicInfoPatterns on FolderBasicInfo {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _FolderBasicInfo value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _FolderBasicInfo() when $default != null:
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

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _FolderBasicInfo value)  $default,){
final _that = this;
switch (_that) {
case _FolderBasicInfo():
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _FolderBasicInfo value)?  $default,){
final _that = this;
switch (_that) {
case _FolderBasicInfo() when $default != null:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( String path,  String name,  int songCount,  String? hash)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _FolderBasicInfo() when $default != null:
return $default(_that.path,_that.name,_that.songCount,_that.hash);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( String path,  String name,  int songCount,  String? hash)  $default,) {final _that = this;
switch (_that) {
case _FolderBasicInfo():
return $default(_that.path,_that.name,_that.songCount,_that.hash);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( String path,  String name,  int songCount,  String? hash)?  $default,) {final _that = this;
switch (_that) {
case _FolderBasicInfo() when $default != null:
return $default(_that.path,_that.name,_that.songCount,_that.hash);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _FolderBasicInfo implements FolderBasicInfo {
  const _FolderBasicInfo({required this.path, required this.name, required this.songCount, this.hash});
  factory _FolderBasicInfo.fromJson(Map<String, dynamic> json) => _$FolderBasicInfoFromJson(json);

/// 文件夹完整路径（唯一标识，也是 ValueKey / 路由参数）
@override final  String path;
/// 文件夹名（只显示最后一级）
@override final  String name;
/// 这个文件夹下（含子文件夹）有多少首歌
@override final  int songCount;
/// 封面哈希：取该文件夹里第一首有封面的歌，没有就是空串
@override final  String? hash;

/// Create a copy of FolderBasicInfo
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$FolderBasicInfoCopyWith<_FolderBasicInfo> get copyWith => __$FolderBasicInfoCopyWithImpl<_FolderBasicInfo>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$FolderBasicInfoToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _FolderBasicInfo&&(identical(other.path, path) || other.path == path)&&(identical(other.name, name) || other.name == name)&&(identical(other.songCount, songCount) || other.songCount == songCount)&&(identical(other.hash, hash) || other.hash == hash));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,path,name,songCount,hash);

@override
String toString() {
  return 'FolderBasicInfo(path: $path, name: $name, songCount: $songCount, hash: $hash)';
}


}

/// @nodoc
abstract mixin class _$FolderBasicInfoCopyWith<$Res> implements $FolderBasicInfoCopyWith<$Res> {
  factory _$FolderBasicInfoCopyWith(_FolderBasicInfo value, $Res Function(_FolderBasicInfo) _then) = __$FolderBasicInfoCopyWithImpl;
@override @useResult
$Res call({
 String path, String name, int songCount, String? hash
});




}
/// @nodoc
class __$FolderBasicInfoCopyWithImpl<$Res>
    implements _$FolderBasicInfoCopyWith<$Res> {
  __$FolderBasicInfoCopyWithImpl(this._self, this._then);

  final _FolderBasicInfo _self;
  final $Res Function(_FolderBasicInfo) _then;

/// Create a copy of FolderBasicInfo
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? path = null,Object? name = null,Object? songCount = null,Object? hash = freezed,}) {
  return _then(_FolderBasicInfo(
path: null == path ? _self.path : path // ignore: cast_nullable_to_non_nullable
as String,name: null == name ? _self.name : name // ignore: cast_nullable_to_non_nullable
as String,songCount: null == songCount ? _self.songCount : songCount // ignore: cast_nullable_to_non_nullable
as int,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,
  ));
}


}

// dart format on
