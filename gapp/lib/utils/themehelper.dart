import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:groom/ext/customcolor.dart';

class ThemeText {
  static const String _helfitaFont = "Helfita";
  static TextStyle whiteHeadline = const TextStyle(
    color: Colors.white,
    fontSize: 45,
    fontFamily: _helfitaFont,
    fontWeight: FontWeight.w700,
  );
  static TextStyle whiteHeadlineOne = const TextStyle(
    color: Colors.white,
    fontSize: 35,
    fontFamily: _helfitaFont,
    fontWeight: FontWeight.w700,
  );
  static TextStyle whiteHead = TextStyle(
    color: Colors.grey.shade700,
    fontSize: 14,
    fontFamily: _helfitaFont,
    fontWeight: FontWeight.w700,
  );
  static TextStyle whiteHeadlineTwo = const TextStyle(
    color: Colors.white,
    fontSize: 20,
    fontFamily: _helfitaFont,
    fontWeight: FontWeight.bold,
  );
  static TextStyle blackHeadlineTwo = const TextStyle(
    color: Colors.white,
    fontSize: 23,
    fontFamily: _helfitaFont,
    fontWeight: FontWeight.w700,
  );
  static TextStyle blackHeadlineFour = const TextStyle(
    color: CustomColors.greyTwo,
    fontSize: 23,
    fontFamily: _helfitaFont,
    fontWeight: FontWeight.w700,
  );
  static TextStyle blackHeadlineThree = const TextStyle(
    color: Colors.white,
    fontSize: 17,
    fontFamily: _helfitaFont,
    fontWeight: FontWeight.w700,
  );
  static TextStyle bodyPrimaryOne = GoogleFonts.poppins(
    color: CustomColors.primaryColor,
    fontSize: 16,
    // fontFamily: _karlaFont,
    fontWeight: FontWeight.normal,
  );
  static TextStyle mediumText = GoogleFonts.poppins(
      fontSize: 16.63,
      fontWeight: FontWeight.w500,
      // height: 1.0, // This is equivalent to line-height: 16px
      letterSpacing: 0.0,
      color: Colors.black);
  static TextStyle samlltext = GoogleFonts.poppins(
      fontSize: 12,
      fontWeight: FontWeight.w400,
      height: 1.0, // This is equivalent to line-height: 16px
      letterSpacing: 0.0,
      color: Colors.black);
  static TextStyle bodyWhiteTwo = GoogleFonts.poppins(
    color: Colors.white,
    fontSize: 16,
    // fontFamily: _karlaFont,
    fontWeight: FontWeight.normal,
  );
  static TextStyle subtitle = GoogleFonts.poppins(
    color: Colors.white,
    fontSize: 14,
    // fontFamily: _karlaFont,
    fontWeight: FontWeight.normal,
  );
  static TextStyle bodyWhiteTwowithocpticy = GoogleFonts.poppins(
    color: Colors.white.withOpacity(0.5),
    fontSize: 16,
    // fontFamily: _karlaFont,
    fontWeight: FontWeight.normal,
  );
  static TextStyle subtitleOne = GoogleFonts.poppins(
    color: CustomColors.greyTwo,
    fontSize: 13,
    // fontFamily: _karlaFont,
    fontWeight: FontWeight.normal,
  );
  static TextStyle subtitlegreyone = GoogleFonts.poppins(
    color: CustomColors.greyOne,
    fontSize: 13,
    // fontFamily: _karlaFont,
    fontWeight: FontWeight.normal,
  );
  static TextStyle titlebalck = GoogleFonts.poppins(
    color: Colors.black,
    fontSize: 20,
    // fontFamily: _karlaFont,
    fontWeight: FontWeight.bold,
  );
  static TextStyle bodyGreyOne = GoogleFonts.poppins(
    fontSize: 14,
    fontWeight: FontWeight.w400,
    height: 1.21, // This is equivalent to line-height in Flutter
    letterSpacing: 0,
  );

  static TextStyle normaltext = GoogleFonts.poppins(
    fontSize: 14.0,
    fontWeight: FontWeight.w400,
    color: Color(0x384144).withOpacity(0.5),
    height: 1.5, // This corresponds to the line height, use the value you need
    letterSpacing: 0.0, // This corresponds to the letter spacing
  );
  static TextStyle Text1 = GoogleFonts.poppins(
    color: Colors.black,
    fontSize: 20,
    fontWeight: FontWeight.w600,
    letterSpacing: 0,
  );
  static TextStyle bodyTwo = GoogleFonts.poppins(
    fontSize: 38.0,
    fontWeight: FontWeight.w700,
    color: Colors.white,
    height: 1.08, // This corresponds to line-height: 41px / font-size: 38px
    letterSpacing: 0.0, // letter-spacing: 0em
  );
}
