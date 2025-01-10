import 'package:flutter/material.dart';
import 'package:flutter_stripe/flutter_stripe.dart';
import 'package:get/get.dart';
import 'package:groom/states/user_state.dart';

import '../../payment/payment_function.dart';

class CustomerPaymentSummaryScreen extends StatefulWidget {
  const CustomerPaymentSummaryScreen({super.key});

  @override
  State<CustomerPaymentSummaryScreen> createState() =>
      _CustomerPaymentSummaryScreenState();
}

class _CustomerPaymentSummaryScreenState
    extends State<CustomerPaymentSummaryScreen> {
  UserStateController userStateController = Get.find();
  String? countryValue;
  String? stateValue;
  String? cityValue;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(),
      body: Column(
        children: [
          Text("Membership summary"),
          Container(
            height: 300,
            width: MediaQuery.sizeOf(context).width * 0.8,
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(9),
              border: Border.all(color: Colors.green, width: 2),
            ),
            child: Column(
              children: [
                Text("Membership name :"),
                Text("Price :"),
                Text("Starts from"),
                Text("Ends on")
              ],
            ),
          ),
          Container(
            height: 300,
            width: MediaQuery.sizeOf(context).width * 0.8,
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(9),
              border: Border.all(color: Colors.brown, width: 2),
            ),
            child: Column(
              children: [
                Row(
                  children: [
                    Expanded(
                        child: TextField(
                      decoration: InputDecoration(hintText: "Card holder name"),
                    ),),
                  ],
                ),
                Row(
                  children: [
                    Expanded(
                        child: TextField(
                      decoration: InputDecoration(hintText: "address"),
                    ),),
                  ],
                ),
                Row(
                  children: [
                    Expanded(
                        child: TextField(
                      decoration: InputDecoration(hintText: "postal code"),
                    ),),
                  ],
                ),
                /*Padding(
                  padding: const EdgeInsets.symmetric(
                      horizontal: 22.0, vertical: 4.0),
                  child: CSCPicker(
                    showCities: true,
                    showStates: true,
                    onCountryChanged: (country) {
                      countryValue = country;
                    },
                    onStateChanged: (state) {
                      stateValue = state;
                    },
                    onCityChanged: (city) {
                      cityValue = city;
                    },
                    countrySearchPlaceholder: "Search Country",
                    stateSearchPlaceholder: "Search State",
                    citySearchPlaceholder: "Search City",
                  ),
                ),*/
              ],
            ),
          ),
          Center(
            child: ElevatedButton(
              onPressed: () async {
                await initPaymentSheet(
                  context,
                  "",
                  "",
                  "",
                  "",
                  "",
                  "",
                  "",
                  "",
                );
                try {
                  await Stripe.instance.presentPaymentSheet();
                  ScaffoldMessenger.of(context).showSnackBar(
                    SnackBar(content: Text('good: good')),
                  );
                } catch (e) {
                  print(e);
                }
              },
              child: Text("Pay Now"),
            ),
          )
        ],
      ),
    );
  }
}
