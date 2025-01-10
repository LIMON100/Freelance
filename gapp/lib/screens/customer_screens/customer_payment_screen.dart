import 'package:flutter/material.dart';
import 'package:get/get.dart';

import 'customer_payment_summary_screen.dart';

class CustomerPaymentScreen extends StatefulWidget {
  const CustomerPaymentScreen({super.key});

  @override
  State<CustomerPaymentScreen> createState() => _CustomerPaymentScreenState();
}

class _CustomerPaymentScreenState extends State<CustomerPaymentScreen> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(),
      body: Column(
        children: [
          GestureDetector(
            onTap: (){
              Get.to(()=>CustomerPaymentSummaryScreen());

            },
            child: Card(
              child: Column(
                children: [
                  Row(
                    children: [
                      Column(
                        children: [
                          Text(
                            "Basic",
                            style: TextStyle(
                                fontWeight: FontWeight.bold, fontSize: 18),
                          ),
                          Text("Service included")
                        ],
                      ),
                      TextButton(
                        onPressed: () {},
                        child: Text("\$ 0.0/Months"),
                      ),
                    ],
                  ),
                  Row(
                    children: [
                      Column(
                        children: [
                          Row(
                            children: [
                              Icon(
                                Icons.check,
                                color: Colors.green,
                              ),
                              Text("Service listing and inquires (5/mo)"),
                            ],
                          ),
                          Row(
                            children: [
                              Icon(
                                Icons.check,
                                color: Colors.green,
                              ),
                              Text("Service listing and inquires (5/mo)"),
                            ],
                          ),
                          Row(
                            children: [
                              Icon(
                                Icons.check,
                                color: Colors.green,
                              ),
                              Text("Service listing and inquires (5/mo)"),
                            ],
                          ),
                        ],
                      )
                    ],
                  )
                ],
              ),
            ),
          ),
          Card(
            child: Column(
              children: [
                Row(
                  children: [
                    Column(
                      children: [
                        Text(
                          "Barber pro",
                          style: TextStyle(
                              fontWeight: FontWeight.bold, fontSize: 18),
                        ),
                        Text("Service included")
                      ],
                    ),
                    TextButton(
                      onPressed: () {},
                      child: Text("\$ 0.0/Months"),
                    ),
                  ],
                ),
                Row(
                  children: [
                    Column(
                      children: [
                        Row(
                          children: [
                            Icon(
                              Icons.check,
                              color: Colors.green,
                            ),
                            Text("Service listing and inquires (5/mo)"),
                          ],
                        ),
                        Row(
                          children: [
                            Icon(
                              Icons.check,
                              color: Colors.green,
                            ),
                            Text("Service listing and inquires (5/mo)"),
                          ],
                        ),
                        Row(
                          children: [
                            Icon(
                              Icons.check,
                              color: Colors.green,
                            ),
                            Text("Service listing and inquires (5/mo)"),
                          ],
                        ),
                      ],
                    )
                  ],
                )
              ],
            ),
          )
        ],
      ),
    );
  }
}
