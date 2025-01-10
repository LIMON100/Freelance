import 'package:groom/data_models/provider_service_model.dart';

class ServiceReservationModel {
  String reservationId;
  String bookingId;
  String clientId;
  String providerId;
  String serviceId;
  String paymentType;
  String transactionId;
  String status;
  String review_status;
  int createdOn;
  String selectedDate;
  ProviderServiceModel? serviceDetails;

  ServiceReservationModel({
    required this.reservationId,
    required this.selectedDate,
    required this.bookingId,
    required this.clientId,
    required this.providerId,
    required this.serviceId,
    required this.createdOn,
    required this.paymentType,
    required this.transactionId,
    required this.serviceDetails,
    required this.status,
    required this.review_status,
  });

  factory ServiceReservationModel.fromJson(Map<String, dynamic> json) {
    return ServiceReservationModel(
      reservationId: json['reservationId'] ?? '',
      bookingId: json['bookingId'] ?? '',
      clientId: json['clientId'] ?? '',
      providerId: json['providerId'] ?? '',
      serviceId: json['serviceId'] ?? '',
      paymentType: json['paymentType'] ?? '',
      transactionId: json['transactionId'] ?? '',
        status: json['status'] ?? '',
        review_status: json['review_status'] ?? '0',
      createdOn: json['createdOn'] != null
          ? int.parse(json['createdOn'].toString())
          : 0,
      selectedDate: json['selectedDate']??"",
        serviceDetails: ProviderServiceModel.fromJson(json['serviceDetails'])
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'selectedDate': selectedDate,
      'reservationId': reservationId,
      'bookingId': bookingId,
      'clientId': clientId,
      'providerId': providerId,
      'serviceId': serviceId,
      'createdOn': createdOn,
      'paymentType': paymentType,
      'transactionId': transactionId,
      'review_status': review_status,
      'status': status,
      "serviceDetails":serviceDetails!.toJson()
    };
  }
}
