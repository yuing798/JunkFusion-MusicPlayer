// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'lyric_info.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$LyricWord {

 int get startMs; int get endMs; String get text;
/// Create a copy of LyricWord
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$LyricWordCopyWith<LyricWord> get copyWith => _$LyricWordCopyWithImpl<LyricWord>(this as LyricWord, _$identity);

  /// Serializes this LyricWord to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is LyricWord&&(identical(other.startMs, startMs) || other.startMs == startMs)&&(identical(other.endMs, endMs) || other.endMs == endMs)&&(identical(other.text, text) || other.text == text));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,startMs,endMs,text);

@override
String toString() {
  return 'LyricWord(startMs: $startMs, endMs: $endMs, text: $text)';
}


}

/// @nodoc
abstract mixin class $LyricWordCopyWith<$Res>  {
  factory $LyricWordCopyWith(LyricWord value, $Res Function(LyricWord) _then) = _$LyricWordCopyWithImpl;
@useResult
$Res call({
 int startMs, int endMs, String text
});




}
/// @nodoc
class _$LyricWordCopyWithImpl<$Res>
    implements $LyricWordCopyWith<$Res> {
  _$LyricWordCopyWithImpl(this._self, this._then);

  final LyricWord _self;
  final $Res Function(LyricWord) _then;

/// Create a copy of LyricWord
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? startMs = null,Object? endMs = null,Object? text = null,}) {
  return _then(_self.copyWith(
startMs: null == startMs ? _self.startMs : startMs // ignore: cast_nullable_to_non_nullable
as int,endMs: null == endMs ? _self.endMs : endMs // ignore: cast_nullable_to_non_nullable
as int,text: null == text ? _self.text : text // ignore: cast_nullable_to_non_nullable
as String,
  ));
}

}


/// Adds pattern-matching-related methods to [LyricWord].
extension LyricWordPatterns on LyricWord {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _LyricWord value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _LyricWord() when $default != null:
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

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _LyricWord value)  $default,){
final _that = this;
switch (_that) {
case _LyricWord():
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _LyricWord value)?  $default,){
final _that = this;
switch (_that) {
case _LyricWord() when $default != null:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int startMs,  int endMs,  String text)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _LyricWord() when $default != null:
return $default(_that.startMs,_that.endMs,_that.text);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int startMs,  int endMs,  String text)  $default,) {final _that = this;
switch (_that) {
case _LyricWord():
return $default(_that.startMs,_that.endMs,_that.text);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int startMs,  int endMs,  String text)?  $default,) {final _that = this;
switch (_that) {
case _LyricWord() when $default != null:
return $default(_that.startMs,_that.endMs,_that.text);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _LyricWord implements LyricWord {
  const _LyricWord({required this.startMs, required this.endMs, required this.text});
  factory _LyricWord.fromJson(Map<String, dynamic> json) => _$LyricWordFromJson(json);

@override final  int startMs;
@override final  int endMs;
@override final  String text;

/// Create a copy of LyricWord
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$LyricWordCopyWith<_LyricWord> get copyWith => __$LyricWordCopyWithImpl<_LyricWord>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$LyricWordToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _LyricWord&&(identical(other.startMs, startMs) || other.startMs == startMs)&&(identical(other.endMs, endMs) || other.endMs == endMs)&&(identical(other.text, text) || other.text == text));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,startMs,endMs,text);

@override
String toString() {
  return 'LyricWord(startMs: $startMs, endMs: $endMs, text: $text)';
}


}

/// @nodoc
abstract mixin class _$LyricWordCopyWith<$Res> implements $LyricWordCopyWith<$Res> {
  factory _$LyricWordCopyWith(_LyricWord value, $Res Function(_LyricWord) _then) = __$LyricWordCopyWithImpl;
@override @useResult
$Res call({
 int startMs, int endMs, String text
});




}
/// @nodoc
class __$LyricWordCopyWithImpl<$Res>
    implements _$LyricWordCopyWith<$Res> {
  __$LyricWordCopyWithImpl(this._self, this._then);

  final _LyricWord _self;
  final $Res Function(_LyricWord) _then;

/// Create a copy of LyricWord
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? startMs = null,Object? endMs = null,Object? text = null,}) {
  return _then(_LyricWord(
startMs: null == startMs ? _self.startMs : startMs // ignore: cast_nullable_to_non_nullable
as int,endMs: null == endMs ? _self.endMs : endMs // ignore: cast_nullable_to_non_nullable
as int,text: null == text ? _self.text : text // ignore: cast_nullable_to_non_nullable
as String,
  ));
}


}


/// @nodoc
mixin _$LyricLine {

 int get startMs; String get text; String? get translation; List<LyricWord> get words;
/// Create a copy of LyricLine
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$LyricLineCopyWith<LyricLine> get copyWith => _$LyricLineCopyWithImpl<LyricLine>(this as LyricLine, _$identity);

  /// Serializes this LyricLine to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is LyricLine&&(identical(other.startMs, startMs) || other.startMs == startMs)&&(identical(other.text, text) || other.text == text)&&(identical(other.translation, translation) || other.translation == translation)&&const DeepCollectionEquality().equals(other.words, words));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,startMs,text,translation,const DeepCollectionEquality().hash(words));

@override
String toString() {
  return 'LyricLine(startMs: $startMs, text: $text, translation: $translation, words: $words)';
}


}

/// @nodoc
abstract mixin class $LyricLineCopyWith<$Res>  {
  factory $LyricLineCopyWith(LyricLine value, $Res Function(LyricLine) _then) = _$LyricLineCopyWithImpl;
@useResult
$Res call({
 int startMs, String text, String? translation, List<LyricWord> words
});




}
/// @nodoc
class _$LyricLineCopyWithImpl<$Res>
    implements $LyricLineCopyWith<$Res> {
  _$LyricLineCopyWithImpl(this._self, this._then);

  final LyricLine _self;
  final $Res Function(LyricLine) _then;

/// Create a copy of LyricLine
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? startMs = null,Object? text = null,Object? translation = freezed,Object? words = null,}) {
  return _then(_self.copyWith(
startMs: null == startMs ? _self.startMs : startMs // ignore: cast_nullable_to_non_nullable
as int,text: null == text ? _self.text : text // ignore: cast_nullable_to_non_nullable
as String,translation: freezed == translation ? _self.translation : translation // ignore: cast_nullable_to_non_nullable
as String?,words: null == words ? _self.words : words // ignore: cast_nullable_to_non_nullable
as List<LyricWord>,
  ));
}

}


/// Adds pattern-matching-related methods to [LyricLine].
extension LyricLinePatterns on LyricLine {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _LyricLine value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _LyricLine() when $default != null:
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

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _LyricLine value)  $default,){
final _that = this;
switch (_that) {
case _LyricLine():
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _LyricLine value)?  $default,){
final _that = this;
switch (_that) {
case _LyricLine() when $default != null:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int startMs,  String text,  String? translation,  List<LyricWord> words)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _LyricLine() when $default != null:
return $default(_that.startMs,_that.text,_that.translation,_that.words);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int startMs,  String text,  String? translation,  List<LyricWord> words)  $default,) {final _that = this;
switch (_that) {
case _LyricLine():
return $default(_that.startMs,_that.text,_that.translation,_that.words);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int startMs,  String text,  String? translation,  List<LyricWord> words)?  $default,) {final _that = this;
switch (_that) {
case _LyricLine() when $default != null:
return $default(_that.startMs,_that.text,_that.translation,_that.words);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _LyricLine implements LyricLine {
  const _LyricLine({required this.startMs, required this.text, this.translation, final  List<LyricWord> words = const <LyricWord>[]}): _words = words;
  factory _LyricLine.fromJson(Map<String, dynamic> json) => _$LyricLineFromJson(json);

@override final  int startMs;
@override final  String text;
@override final  String? translation;
 final  List<LyricWord> _words;
@override@JsonKey() List<LyricWord> get words {
  if (_words is EqualUnmodifiableListView) return _words;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(_words);
}


/// Create a copy of LyricLine
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$LyricLineCopyWith<_LyricLine> get copyWith => __$LyricLineCopyWithImpl<_LyricLine>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$LyricLineToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _LyricLine&&(identical(other.startMs, startMs) || other.startMs == startMs)&&(identical(other.text, text) || other.text == text)&&(identical(other.translation, translation) || other.translation == translation)&&const DeepCollectionEquality().equals(other._words, _words));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,startMs,text,translation,const DeepCollectionEquality().hash(_words));

@override
String toString() {
  return 'LyricLine(startMs: $startMs, text: $text, translation: $translation, words: $words)';
}


}

/// @nodoc
abstract mixin class _$LyricLineCopyWith<$Res> implements $LyricLineCopyWith<$Res> {
  factory _$LyricLineCopyWith(_LyricLine value, $Res Function(_LyricLine) _then) = __$LyricLineCopyWithImpl;
@override @useResult
$Res call({
 int startMs, String text, String? translation, List<LyricWord> words
});




}
/// @nodoc
class __$LyricLineCopyWithImpl<$Res>
    implements _$LyricLineCopyWith<$Res> {
  __$LyricLineCopyWithImpl(this._self, this._then);

  final _LyricLine _self;
  final $Res Function(_LyricLine) _then;

/// Create a copy of LyricLine
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? startMs = null,Object? text = null,Object? translation = freezed,Object? words = null,}) {
  return _then(_LyricLine(
startMs: null == startMs ? _self.startMs : startMs // ignore: cast_nullable_to_non_nullable
as int,text: null == text ? _self.text : text // ignore: cast_nullable_to_non_nullable
as String,translation: freezed == translation ? _self.translation : translation // ignore: cast_nullable_to_non_nullable
as String?,words: null == words ? _self._words : words // ignore: cast_nullable_to_non_nullable
as List<LyricWord>,
  ));
}


}


/// @nodoc
mixin _$LyricDocument {

 bool get hasTranslation; List<LyricLine> get lines;
/// Create a copy of LyricDocument
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$LyricDocumentCopyWith<LyricDocument> get copyWith => _$LyricDocumentCopyWithImpl<LyricDocument>(this as LyricDocument, _$identity);

  /// Serializes this LyricDocument to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is LyricDocument&&(identical(other.hasTranslation, hasTranslation) || other.hasTranslation == hasTranslation)&&const DeepCollectionEquality().equals(other.lines, lines));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,hasTranslation,const DeepCollectionEquality().hash(lines));

@override
String toString() {
  return 'LyricDocument(hasTranslation: $hasTranslation, lines: $lines)';
}


}

/// @nodoc
abstract mixin class $LyricDocumentCopyWith<$Res>  {
  factory $LyricDocumentCopyWith(LyricDocument value, $Res Function(LyricDocument) _then) = _$LyricDocumentCopyWithImpl;
@useResult
$Res call({
 bool hasTranslation, List<LyricLine> lines
});




}
/// @nodoc
class _$LyricDocumentCopyWithImpl<$Res>
    implements $LyricDocumentCopyWith<$Res> {
  _$LyricDocumentCopyWithImpl(this._self, this._then);

  final LyricDocument _self;
  final $Res Function(LyricDocument) _then;

/// Create a copy of LyricDocument
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? hasTranslation = null,Object? lines = null,}) {
  return _then(_self.copyWith(
hasTranslation: null == hasTranslation ? _self.hasTranslation : hasTranslation // ignore: cast_nullable_to_non_nullable
as bool,lines: null == lines ? _self.lines : lines // ignore: cast_nullable_to_non_nullable
as List<LyricLine>,
  ));
}

}


/// Adds pattern-matching-related methods to [LyricDocument].
extension LyricDocumentPatterns on LyricDocument {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _LyricDocument value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _LyricDocument() when $default != null:
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

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _LyricDocument value)  $default,){
final _that = this;
switch (_that) {
case _LyricDocument():
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _LyricDocument value)?  $default,){
final _that = this;
switch (_that) {
case _LyricDocument() when $default != null:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( bool hasTranslation,  List<LyricLine> lines)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _LyricDocument() when $default != null:
return $default(_that.hasTranslation,_that.lines);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( bool hasTranslation,  List<LyricLine> lines)  $default,) {final _that = this;
switch (_that) {
case _LyricDocument():
return $default(_that.hasTranslation,_that.lines);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( bool hasTranslation,  List<LyricLine> lines)?  $default,) {final _that = this;
switch (_that) {
case _LyricDocument() when $default != null:
return $default(_that.hasTranslation,_that.lines);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _LyricDocument implements LyricDocument {
  const _LyricDocument({this.hasTranslation = false, final  List<LyricLine> lines = const <LyricLine>[]}): _lines = lines;
  factory _LyricDocument.fromJson(Map<String, dynamic> json) => _$LyricDocumentFromJson(json);

@override@JsonKey() final  bool hasTranslation;
 final  List<LyricLine> _lines;
@override@JsonKey() List<LyricLine> get lines {
  if (_lines is EqualUnmodifiableListView) return _lines;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(_lines);
}


/// Create a copy of LyricDocument
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$LyricDocumentCopyWith<_LyricDocument> get copyWith => __$LyricDocumentCopyWithImpl<_LyricDocument>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$LyricDocumentToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _LyricDocument&&(identical(other.hasTranslation, hasTranslation) || other.hasTranslation == hasTranslation)&&const DeepCollectionEquality().equals(other._lines, _lines));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,hasTranslation,const DeepCollectionEquality().hash(_lines));

@override
String toString() {
  return 'LyricDocument(hasTranslation: $hasTranslation, lines: $lines)';
}


}

/// @nodoc
abstract mixin class _$LyricDocumentCopyWith<$Res> implements $LyricDocumentCopyWith<$Res> {
  factory _$LyricDocumentCopyWith(_LyricDocument value, $Res Function(_LyricDocument) _then) = __$LyricDocumentCopyWithImpl;
@override @useResult
$Res call({
 bool hasTranslation, List<LyricLine> lines
});




}
/// @nodoc
class __$LyricDocumentCopyWithImpl<$Res>
    implements _$LyricDocumentCopyWith<$Res> {
  __$LyricDocumentCopyWithImpl(this._self, this._then);

  final _LyricDocument _self;
  final $Res Function(_LyricDocument) _then;

/// Create a copy of LyricDocument
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? hasTranslation = null,Object? lines = null,}) {
  return _then(_LyricDocument(
hasTranslation: null == hasTranslation ? _self.hasTranslation : hasTranslation // ignore: cast_nullable_to_non_nullable
as bool,lines: null == lines ? _self._lines : lines // ignore: cast_nullable_to_non_nullable
as List<LyricLine>,
  ));
}


}

// dart format on
