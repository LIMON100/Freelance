import 'dart:typed_data';
import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:geocoding/geocoding.dart';
import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/utils/colors.dart';
import 'package:intl/intl.dart' show DateFormat;
import 'dart:ui' as ui;
class Tools{
  static GlobalKey<NavigatorState> navigatorKey = GlobalKey<NavigatorState>();

 static void ShowSuccessMessage(message,{Duration? duration}){
   if(message==null){
     return;
   }
   Get.showSnackbar(GetSnackBar(
     message:message,
     duration:duration??const Duration(seconds: 2),
     backgroundColor: primaryColorCode,
     borderRadius: 20,
     margin: const EdgeInsets.symmetric(horizontal: 20,vertical: 10),
   ));
 }
static void ShowErrorMessage(message){
  Get.showSnackbar(GetSnackBar(
    message:message,
    duration: const Duration(seconds: 2),
    backgroundColor: Colors.red,
    borderRadius: 20,
    margin: const EdgeInsets.symmetric(horizontal: 20,vertical: 10),
    icon: Icon(Icons.error,color: Colors.white,),
  ));
 }

static bool isEmailValid(email){
    return RegExp(r"^[a-zA-Z0-9.a-zA-Z0-9.!#$%&'*+-/=?^_`{|}~]+@[a-zA-Z0-9]+\.[a-zA-Z]+").hasMatch(email);
}
  static String changeDateFormat(String date,String format_type){
    final format22 =  DateFormat('yyyy-MM-dd HH:mm:ssZ','en-US');
    DateFormat format=DateFormat(format_type);
    DateTime dateTime=format22.parse(DateTime.parse(date).toLocal().toString());
    try {
      return format.format(dateTime);
    } catch (e, s) {
      return date;
    }

  }
  static DateTime changeToDate(String date, {String? frm}){
    final format =  DateFormat(frm??'yyyy-MM-dd','en-US');
    return format.parse(date);

  }
  static String changeDateFormat2(String date,String format_type,String old_format){
    final format22 =  DateFormat(old_format);
    DateFormat format=DateFormat(format_type);
    DateTime dateTime=format22.parse(DateTime.parse(date).toString());
    try {
      return format.format(dateTime);
    } catch (e, s) {
      return date;
    }

  }
  static String changeTimeFormat(String time){
    final format22 = new DateFormat('HH:mm','en-US');
    DateFormat format=DateFormat("hh:mm a");
    DateTime dateTime=format22.parse(time);
    return format.format(dateTime);
  }
  static String changeTimeFormat2(String time){
    final format22 = new DateFormat('HH:mm:ss','en-US');
    DateFormat format=DateFormat("HH:mm");
    DateTime dateTime=format22.parse(time);
    return format.format(dateTime);
  }
  static double getMonth(String date){
    final format22 = DateFormat('MM/dd/yyyy','en-US');
    DateTime dateTime=format22.parse(date);
    return dateTime.month.toDouble();
  }
  static String getCurrentDate(){
    final format = DateFormat('yyyy-MM-dd','en-US');
    return format.format(DateTime.now());
  }
  static String getTodayName(){
    final format = DateFormat('EEE','en-US');
    return format.format(DateTime.now());
  }
  static bool isExpire(String date){
    final old_format =  DateFormat('yyyy-MM-dd HH:mm:ssZ',"en");
    DateTime dateTime=old_format.parse(date.replaceAll("T", " "));
    DateTime newDate=DateTime.now();
    int leftDays=dateTime.difference(newDate).inDays;
    return leftDays<0;
  }
  static bool isAvailableFoCancel(String date){
    final old_format =  DateFormat('dd MMM yyyy');
    DateTime dateTime=old_format.parse(date);
    DateTime newDate=DateTime.now();
    int leftDays=dateTime.difference(newDate).inDays;
    return leftDays>0;
  }


  static Color hexToColor(String code) {
    return Color(int.parse(code.length>3?code.substring(1, 7):code, radix: 16) + 0xFF000000);
  }

  static void ShowDailog(context,widget){
    showDialog(
        context: context,
        barrierColor: Colors.grey.shade400.withOpacity(0.5),
        builder: (ctxt) => widget);
  }
  static void ShowBottomSheet(context,widget,{bool isScrollControlled=true}){
    showModalBottomSheet(
        context: context,
        builder: (context) {
          return widget;
        },
        backgroundColor: Colors.transparent,
        isScrollControlled: isScrollControlled);
  }
  static Future<String> getAddress(LatLng latlng) async {
    List<Placemark> placemarks = await placemarkFromCoordinates(latlng.latitude, latlng.longitude);
    StringBuffer b=StringBuffer();
    Placemark address=placemarks.first;
    /*if(address.name!=null){
      b.write("${address.name}, ");
    }if(address.street!=null){
      b.write("${address.street}, ");
    }*/if(address.locality!=null){
      b.write("${address.locality}, ");
    }if(address.administrativeArea!=null){
      b.write("${address.administrativeArea}, ");
    }if(address.country!=null){
      b.write("${address.country}, ");
    }
    return b.toString();
  }
  static Future<String> getPostalCode(LatLng latlng) async {
    List<Placemark> placemarks = await placemarkFromCoordinates(latlng.latitude, latlng.longitude);
    return placemarks.first.postalCode!;
  }
}