import 'package:easy_localization/easy_localization.dart';
import 'package:flutter/material.dart';

extension DateTimeFormat on String {
   String toTime12Hrs() {
     if(isEmpty)return "";
     DateFormat format=DateFormat("hh:mm");
     DateTime time=format.parse(this);
     DateFormat newFormat=DateFormat("hh:mm a");
    return newFormat.format(time);
  }
}