class RequestReservationModel {
  String reservationId;
  String requestId;
  String clientId;
  String providerId;
  String offerId;
  String status;
  int createdOn;
  int selectedDate;
  String paymentType;
  String transactionId;
  String description;
  int depositAmount;
  bool? deposit;


  RequestReservationModel({
    required this.reservationId,
    required this.selectedDate,
    required this.requestId,
    required this.clientId,
    required this.providerId,
    required this.offerId,
    required this.createdOn,
    required this.paymentType,
    required this.transactionId,
    required this.status,
    required this.description,
    required this.depositAmount,
  this.deposit
  });

  factory RequestReservationModel.fromJson(Map<String, dynamic> json) {
    return RequestReservationModel(
      reservationId: json['reservationId'] ?? '',
      requestId: json['requestId'] ?? '',
      clientId: json['clientId'] ?? '',
      providerId: json['providerId'] ?? '',
      offerId: json['offerId'] ?? '',
      paymentType: json['paymentType'] ?? '',
      transactionId: json['transactionId'] ?? '',
      status: json['status'] ?? '',
      depositAmount: json['depositAmount'] ?? 0,
      description: json['description'] ?? '',
deposit: json['deposit'] ?? false,
      createdOn: json['createdOn'] != null
          ? int.parse(json['createdOn'].toString())
          : 0,
      selectedDate: json['selectedDate'] != null
          ? int.parse(json['selectedDate'].toString())
          : 0,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'selectedDate': selectedDate,
      'reservationId': reservationId,
      'requestId': requestId,
      'clientId': clientId,
      'providerId': providerId,
      'offerId': offerId,
      'createdOn': createdOn,
      'transactionId': transactionId,
      'paymentType': paymentType,
      'description': description,
      'status': status,
      'deposit': deposit,
    };
  }
}
