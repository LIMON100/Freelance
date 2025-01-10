class CustomerPlan {
  String planName;
  double price;
  int updatedOn;
  int serviceRequests;
  int resetDate;
  int offers;

  CustomerPlan({
    required this.planName,
    required this.price,
    required this.updatedOn,
    required this.serviceRequests,
    required this.offers,
    required this.resetDate,
  });

  // Convert a CustomerPlan object into a JSON map
  Map<String, dynamic> toJson() {
    return {
      'planName': planName,
      'price': price,
      'updatedOn': updatedOn,
      'serviceRequests': serviceRequests,
      'offers': offers,
      'resetDate': resetDate,
    };
  }

  // Create a CustomerPlan object from a JSON map
  factory CustomerPlan.fromJson(Map<String, dynamic> json) {
    return CustomerPlan(
      planName: json['planName'] ?? '',
      price: json['price']?.toDouble() ?? 0.0,
      updatedOn: json['updatedOn'] ?? 0,
      serviceRequests: json['serviceRequests'] ?? 0,
      offers: json['offers'] ?? 0,
      resetDate: json['resetDate']??0,
    );
  }
}

