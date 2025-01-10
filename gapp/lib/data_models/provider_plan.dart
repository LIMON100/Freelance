class ProviderPlan {
  String planName;
  double price;
  int updatedOn;
  int serviceRequests;
  int offers;

  ProviderPlan({
    required this.planName,
    required this.price,
    required this.updatedOn,
    required this.serviceRequests,
    required this.offers,
  });

  // Convert a CustomerPlan object into a JSON map
  Map<String, dynamic> toJson() {
    return {
      'planName': planName,
      'price': price,
      'updatedOn': updatedOn,
      'serviceRequests': serviceRequests,
      'offers': offers,
    };
  }

  // Create a CustomerPlan object from a JSON map
  factory ProviderPlan.fromJson(Map<String, dynamic> json) {
    return ProviderPlan(
      planName: json['planName'] ?? '',
      price: json['price']?.toDouble() ?? 0.0,
      updatedOn: json['updatedOn'] ?? 0,
      serviceRequests: json['serviceRequests'] ?? 0,
      offers: json['offers'] ?? 0,
    );
  }
}
