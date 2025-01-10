import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:groom/widgets/header_txt_widget.dart';

import '../utils/colors.dart';

class CustomerTerms extends StatefulWidget {
  const CustomerTerms({super.key});

  @override
  State<CustomerTerms> createState() => _CustomerTermsState();
}

class _CustomerTermsState extends State<CustomerTerms> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: HeaderTxtWidget(
          "Terms of service",
          color: Colors.white,
        ),
      ),
      body: const SingleChildScrollView(
        child: Padding(
          padding: EdgeInsets.all(8.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.center,
            children: [
              Padding(
                padding: EdgeInsets.all(8.0),
                child: SizedBox(
                  height: 600,
                  width: 327,
                  child: Text(
                      "Welcome to Groom! These Terms of Service ('Terms') outline the rules and regulations for using Groom's platform ('Platform'), connecting beauty service providers and clients. By accessing or using the Platform, you agree to be bound by these Terms. Groom provides the platform as a venue for communication and service requests.  We encourage you to review these Terms carefully before using Groom. If you disagree with any part of these Terms, you may not access or use the Platform.These Terms cover important topics like user eligibility, acceptable use, limitations of liability, user content, privacy, and dispute resolution. We reserve the right to modify these Terms at any time, so please review them periodically. Your continued use of the Platform after any changes constitutes your acceptance of the new Terms."),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
