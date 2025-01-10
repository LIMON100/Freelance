import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:groom/ext/hex_color.dart';

import '../generated/assets.dart';
import '../utils/colors.dart';

// ignore: must_be_immutable
class SearchInputWidget extends StatelessWidget {
  TextEditingController? controller;
  Function(String)? onChanged;
  Function(String)? onDone;
  String? hint;

  SearchInputWidget({super.key, this.controller,
    this.hint,
    this.onChanged,
    this.onDone});
  OutlineInputBorder border= OutlineInputBorder(
  borderRadius: BorderRadius.circular(10),
    borderSide:  BorderSide(color: Colors.black.withOpacity(0.1),width: 1),
  );
  OutlineInputBorder errorBorder= OutlineInputBorder(
  borderRadius: BorderRadius.circular(10),
  borderSide: BorderSide(color: errorColor,width: 1),
  );
  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.only(bottom: 10, left: 10,right: 10,top: 10),
      alignment: AlignmentDirectional.topCenter,
      child: TextFormField(
        controller: controller ?? TextEditingController(),
        onChanged: onChanged,
        onFieldSubmitted: onDone,
        decoration: InputDecoration(
          fillColor:"#EBF0F5".toColor(),
          contentPadding:const EdgeInsets.symmetric(horizontal: 5, vertical: 5),
          focusedBorder: border,
          errorBorder: errorBorder,
          errorStyle: TextStyle(color: errorColor),
          enabledBorder: border,
          border:border,
          prefixIcon:Padding(
            padding: const EdgeInsets.all(10),
            child: SvgPicture.asset(Assets.svgSearch,),
          ),
          focusedErrorBorder:border,
          filled: true,
          hoverColor: Colors.transparent,
          hintText: hint ?? "",
          hintStyle:  TextStyle(
            overflow: TextOverflow.ellipsis,
            fontWeight: FontWeight.w400,
            fontSize: 12,
            color: "#5E5E5E7A".toColor().withOpacity(0.5),),
        ),
      ),
    );
  }
}
