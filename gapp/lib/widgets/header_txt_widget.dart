import 'package:flutter/material.dart';
import 'package:groom/ext/themehelper.dart';

import '../utils/colors.dart';

class HeaderTxtWidget extends StatelessWidget {
  String txt;
  var textAlign;
  var maxLines;
  var color;
  double fontSize;
  TextDecoration? decoration;
  TextOverflow? overflow;
  FontWeight? fontWeight;

  HeaderTxtWidget(this.txt,
      {this.fontWeight,
      this.textAlign,
      this.maxLines,
      this.color,
      this.fontSize = 16,
      this.overflow,
      this.decoration});

  @override
  Widget build(BuildContext context) {
    return Text(
      txt,
      textAlign: textAlign,
      maxLines: maxLines,
      style: ThemeText.mediumText.copyWith(
          fontSize: fontSize,
          fontStyle: FontStyle.normal,
          fontWeight: fontWeight ?? FontWeight.w900,
          color: color ?? titleColor,
          overflow: overflow,
          decoration: decoration ?? TextDecoration.none),
    );
  }
}
