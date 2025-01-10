import 'package:groom/data_models/provider_service_model.dart';

class ServiceBookingModel {
  String bookingId;
  String serviceId;
  String providerId;
  String clientId;
  int createdOn;
  String selectedDate;
  String status;
  String selectedTime;
  ProviderServiceModel? serviceDetails;

  ServiceBookingModel({
    required this.bookingId,
    required this.serviceId,
    required this.providerId,
    required this.clientId,
    required this.createdOn,
    required this.status,
    required this.selectedDate,
    required this.selectedTime,
    this.serviceDetails,
  });

  factory ServiceBookingModel.fromJson(Map<String, dynamic> json) {
    return ServiceBookingModel(
      bookingId: json["bookingId"] ?? '',
      serviceId: json["serviceId"] ?? '',
      providerId: json["providerId"] ?? '',
      clientId: json["clientId"] ?? '',
      status: json["status"] ?? '',
      selectedTime: json['selectedTime']??"",
      selectedDate: json['selectedDateTime']??"",
      createdOn: json['createdOn'] != null
          ? int.parse(json['createdOn'].toString())
          : 0,
      serviceDetails: ProviderServiceModel.fromJson(json['serviceDetails'])

    );
  }

  Map<String, dynamic> toJson() {
    return {
      'bookingId': bookingId,
      'serviceId': serviceId,
      'providerId': providerId,
      'clientId': clientId,
      'createdOn': createdOn,
      'selectedDateTime': selectedDate,
      'status': status,
      'selectedTime': selectedTime,
      "serviceDetails":serviceDetails!.toJson()
    };
  }
}
