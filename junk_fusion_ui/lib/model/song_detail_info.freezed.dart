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

 int get bitRate; int get sampleRate; String get channelLayout; int get bitDepth; String? get codecName; int? get bpm; String? get key; String? get hash; List<double> get timeDomainSpec;
/// Create a copy of SongDetailInfo
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$SongDetailInfoCopyWith<SongDetailInfo> get copyWith => _$SongDetailInfoCopyWithImpl<SongDetailInfo>(this as SongDetailInfo, _$identity);

  /// Serializes this SongDetailInfo to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is SongDetailInfo&&(identical(other.bitRate, bitRate) || other.bitRate == bitRate)&&(identical(other.sampleRate, sampleRate) || other.sampleRate == sampleRate)&&(identical(other.channelLayout, channelLayout) || other.channelLayout == channelLayout)&&(identical(other.bitDepth, bitDepth) || other.bitDepth == bitDepth)&&(identical(other.codecName, codecName) || other.codecName == codecName)&&(identical(other.bpm, bpm) || other.bpm == bpm)&&(identical(other.key, key) || other.key == key)&&(identical(other.hash, hash) || other.hash == hash)&&const DeepCollectionEquality().equals(other.timeDomainSpec, timeDomainSpec));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,bitRate,sampleRate,channelLayout,bitDepth,codecName,bpm,key,hash,const DeepCollectionEquality().hash(timeDomainSpec));

@override
String toString() {
  return 'SongDetailInfo(bitRate: $bitRate, sampleRate: $sampleRate, channelLayout: $channelLayout, bitDepth: $bitDepth, codecName: $codecName, bpm: $bpm, key: $key, hash: $hash, timeDomainSpec: $timeDomainSpec)';
}


}

/// @nodoc
abstract mixin class $SongDetailInfoCopyWith<$Res>  {
  factory $SongDetailInfoCopyWith(SongDetailInfo value, $Res Function(SongDetailInfo) _then) = _$SongDetailInfoCopyWithImpl;
@useResult
$Res call({
 int bitRate, int sampleRate, String channelLayout, int bitDepth, String? codecName, int? bpm, String? key, String? hash, List<double> timeDomainSpec
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
@pragma('vm:prefer-inline') @override $Res call({Object? bitRate = null,Object? sampleRate = null,Object? channelLayout = null,Object? bitDepth = null,Object? codecName = freezed,Object? bpm = freezed,Object? key = freezed,Object? hash = freezed,Object? timeDomainSpec = null,}) {
  return _then(_self.copyWith(
bitRate: null == bitRate ? _self.bitRate : bitRate // ignore: cast_nullable_to_non_nullable
as int,sampleRate: null == sampleRate ? _self.sampleRate : sampleRate // ignore: cast_nullable_to_non_nullable
as int,channelLayout: null == channelLayout ? _self.channelLayout : channelLayout // ignore: cast_nullable_to_non_nullable
as String,bitDepth: null == bitDepth ? _self.bitDepth : bitDepth // ignore: cast_nullable_to_non_nullable
as int,codecName: freezed == codecName ? _self.codecName : codecName // ignore: cast_nullable_to_non_nullable
as String?,bpm: freezed == bpm ? _self.bpm : bpm // ignore: cast_nullable_to_non_nullable
as int?,key: freezed == key ? _self.key : key // ignore: cast_nullable_to_non_nullable
as String?,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int bitRate,  int sampleRate,  String channelLayout,  int bitDepth,  String? codecName,  int? bpm,  String? key,  String? hash,  List<double> timeDomainSpec)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _SongDetailInfo() when $default != null:
return $default(_that.bitRate,_that.sampleRate,_that.channelLayout,_that.bitDepth,_that.codecName,_that.bpm,_that.key,_that.hash,_that.timeDomainSpec);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int bitRate,  int sampleRate,  String channelLayout,  int bitDepth,  String? codecName,  int? bpm,  String? key,  String? hash,  List<double> timeDomainSpec)  $default,) {final _that = this;
switch (_that) {
case _SongDetailInfo():
return $default(_that.bitRate,_that.sampleRate,_that.channelLayout,_that.bitDepth,_that.codecName,_that.bpm,_that.key,_that.hash,_that.timeDomainSpec);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int bitRate,  int sampleRate,  String channelLayout,  int bitDepth,  String? codecName,  int? bpm,  String? key,  String? hash,  List<double> timeDomainSpec)?  $default,) {final _that = this;
switch (_that) {
case _SongDetailInfo() when $default != null:
return $default(_that.bitRate,_that.sampleRate,_that.channelLayout,_that.bitDepth,_that.codecName,_that.bpm,_that.key,_that.hash,_that.timeDomainSpec);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _SongDetailInfo implements SongDetailInfo {
  const _SongDetailInfo({required this.bitRate, required this.sampleRate, required this.channelLayout, required this.bitDepth, this.codecName, this.bpm, this.key, this.hash, required final  List<double> timeDomainSpec}): _timeDomainSpec = timeDomainSpec;
  factory _SongDetailInfo.fromJson(Map<String, dynamic> json) => _$SongDetailInfoFromJson(json);

@override final  int bitRate;
@override final  int sampleRate;
@override final  String channelLayout;
@override final  int bitDepth;
@override final  String? codecName;
@override final  int? bpm;
@override final  String? key;
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
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _SongDetailInfo&&(identical(other.bitRate, bitRate) || other.bitRate == bitRate)&&(identical(other.sampleRate, sampleRate) || other.sampleRate == sampleRate)&&(identical(other.channelLayout, channelLayout) || other.channelLayout == channelLayout)&&(identical(other.bitDepth, bitDepth) || other.bitDepth == bitDepth)&&(identical(other.codecName, codecName) || other.codecName == codecName)&&(identical(other.bpm, bpm) || other.bpm == bpm)&&(identical(other.key, key) || other.key == key)&&(identical(other.hash, hash) || other.hash == hash)&&const DeepCollectionEquality().equals(other._timeDomainSpec, _timeDomainSpec));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,bitRate,sampleRate,channelLayout,bitDepth,codecName,bpm,key,hash,const DeepCollectionEquality().hash(_timeDomainSpec));

@override
String toString() {
  return 'SongDetailInfo(bitRate: $bitRate, sampleRate: $sampleRate, channelLayout: $channelLayout, bitDepth: $bitDepth, codecName: $codecName, bpm: $bpm, key: $key, hash: $hash, timeDomainSpec: $timeDomainSpec)';
}


}

/// @nodoc
abstract mixin class _$SongDetailInfoCopyWith<$Res> implements $SongDetailInfoCopyWith<$Res> {
  factory _$SongDetailInfoCopyWith(_SongDetailInfo value, $Res Function(_SongDetailInfo) _then) = __$SongDetailInfoCopyWithImpl;
@override @useResult
$Res call({
 int bitRate, int sampleRate, String channelLayout, int bitDepth, String? codecName, int? bpm, String? key, String? hash, List<double> timeDomainSpec
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
@override @pragma('vm:prefer-inline') $Res call({Object? bitRate = null,Object? sampleRate = null,Object? channelLayout = null,Object? bitDepth = null,Object? codecName = freezed,Object? bpm = freezed,Object? key = freezed,Object? hash = freezed,Object? timeDomainSpec = null,}) {
  return _then(_SongDetailInfo(
bitRate: null == bitRate ? _self.bitRate : bitRate // ignore: cast_nullable_to_non_nullable
as int,sampleRate: null == sampleRate ? _self.sampleRate : sampleRate // ignore: cast_nullable_to_non_nullable
as int,channelLayout: null == channelLayout ? _self.channelLayout : channelLayout // ignore: cast_nullable_to_non_nullable
as String,bitDepth: null == bitDepth ? _self.bitDepth : bitDepth // ignore: cast_nullable_to_non_nullable
as int,codecName: freezed == codecName ? _self.codecName : codecName // ignore: cast_nullable_to_non_nullable
as String?,bpm: freezed == bpm ? _self.bpm : bpm // ignore: cast_nullable_to_non_nullable
as int?,key: freezed == key ? _self.key : key // ignore: cast_nullable_to_non_nullable
as String?,hash: freezed == hash ? _self.hash : hash // ignore: cast_nullable_to_non_nullable
as String?,timeDomainSpec: null == timeDomainSpec ? _self._timeDomainSpec : timeDomainSpec // ignore: cast_nullable_to_non_nullable
as List<double>,
  ));
}


}

// dart format on
