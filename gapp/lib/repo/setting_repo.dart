import 'package:flutter/material.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/data_models/user_model.dart';

ValueNotifier<UserModel> auth = ValueNotifier(UserModel.fromJson({}));
ValueNotifier<bool> isProvider = ValueNotifier(false);
ValueNotifier<bool> isGuest = ValueNotifier(false);
ValueNotifier<bool> isDismissMembership = ValueNotifier(false);
ValueNotifier<Map<String,dynamic>> filter = ValueNotifier({});
double radius=200;
String globalTimeFormat="MM-dd-yyyy";
LatLng defalultLatLng=const LatLng(36.171566009502634, -115.13909570424758);