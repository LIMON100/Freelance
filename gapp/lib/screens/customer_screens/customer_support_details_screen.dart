import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:groom/data_models/complaint_model.dart';
import 'package:groom/utils/utils.dart';

class CustomerSupportDetailsScreen extends StatelessWidget {
  final ComplaintModel complaint;

  const CustomerSupportDetailsScreen({super.key, required this.complaint});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(),
      body: Padding(
        padding: const EdgeInsets.all(8.0),
        child: Column(
          mainAxisSize: MainAxisSize.max,
          mainAxisAlignment: MainAxisAlignment.start,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            SizedBox(
              height: 40,
            ),
            Center(
              child: Text(
                "Complaint Screen",
                style: TextStyle(fontWeight: FontWeight.bold, fontSize: 24),
              ),
            ),
            SizedBox(
              height: 40,
            ),
            Text(
              "Complaint title :",
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 24),
            ),
            Text(
              complaint.title,
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 24),
            ),
            Text(
              "Complaint Date :",
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 24),
            ),
            Text(
              formatDateInt(complaint.createdOn),
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 24),
            ),
            Text(
              "Complaint description :",
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 24),
            ),
            Text(
              complaint.complaint,
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 24),
            ),
          ],
        ),
      ),
    );
  }
}
