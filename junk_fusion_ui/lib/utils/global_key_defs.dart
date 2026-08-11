import 'package:flutter/material.dart';

final GlobalKey<NavigatorState> navigatorKey = GlobalKey<NavigatorState>();
//GlobalKey<XXXState>.currentState 返回的，永远是“绑定了这个Key的那个Widget”所对应的State对象。
