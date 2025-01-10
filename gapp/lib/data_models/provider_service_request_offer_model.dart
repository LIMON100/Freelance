class ProviderServiceRequestOfferModel {
  String requestId;
  String providerId;
  String serviceId;
  int createdOn;
  bool deposit;
  int depositAmount;
  String clientId;
  String description;
  int selectedDate;

  ProviderServiceRequestOfferModel({
    required this.selectedDate,
    required this.description,
    required this.requestId,
    required this.providerId,
    required this.serviceId,
    required this.createdOn,
    required this.deposit,
    required this.depositAmount,
    required this.clientId,
  });

  // Convert a ProviderServiceRequestOfferModel object into a Map object
  Map<String, dynamic> toJson() {
    return {
      'selectedDate':selectedDate,
      'description':description,
      'requestId':requestId,
      'providerId': providerId,
      'serviceId': serviceId,
      'createdOn': createdOn,
      'deposit': deposit,
      'depositAmount': depositAmount,
      'clientId': clientId,
    };
  }

  // Create a ProviderServiceRequestOfferModel object from a Map object
  factory ProviderServiceRequestOfferModel.fromJson(Map<String, dynamic> json) {
    return ProviderServiceRequestOfferModel(
      description: json['description']as String,
      requestId: json['requestId']as String,
      providerId: json['providerId'] as String,
      serviceId: json['serviceId'] as String,
      createdOn: json['createdOn'] as int,
      selectedDate: json['selectedDate'] as int,
      deposit: json['deposit'] as bool,
      depositAmount: json['depositAmount'] as int,
      clientId: json['clientId'] as String,
    );
  }
}
