import 'package:flutter/material.dart';

extension HexColor on String {
  /// String is in the format "aabbcc" or "ffaabbcc" with an optional leading "#".
   Color toColor() {
    final buffer = StringBuffer();
    if (length == 6 || length == 7) buffer.write('ff');
    buffer.write(replaceFirst('#', ''));
    return Color(int.parse(buffer.toString(), radix: 16));
  }
  String toMiniAddress({int limit=30}){
     if(isEmpty)return "No address available";
     if(length>limit) return "${substring(0,limit)}...";
     return this;
  }
}