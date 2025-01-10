import 'package:groom/data_models/user_model.dart';

import 'service_booking_model.dart';

class ClientUserModel {
  String clientAddress = '';

  List<ServiceBookingModel> clientBookings = [];

  ClientUserModel({required this.clientAddress, required this.clientBookings});

  ClientUserModel.fromJson(Map<String,dynamic>json){
    clientAddress = json['clientAddress'];
    if (json['clientBookings'] != null) {
      clientBookings = List<ServiceBookingModel>.empty(growable: true);
      json['clientBookings'].forEach((v) {
        clientBookings.add(ServiceBookingModel.fromJson(v));
      });
    }
  }
}
