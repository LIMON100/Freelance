import 'package:flutter/material.dart';

class NearbyServiceCardWidget extends StatelessWidget {
  int price;
  String serviceType;
  String providerTitle;
  String imageUrl;

   NearbyServiceCardWidget({
    super.key,
    required this.price,
    required this.serviceType,
    required this.providerTitle,
     required this.imageUrl,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: null,
    );
  }
}
