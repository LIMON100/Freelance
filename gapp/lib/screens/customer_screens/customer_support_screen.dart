import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:get/get.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:groom/constant/asset.dart';
import 'package:groom/data_models/complaint_model.dart';
import 'package:groom/ext/themehelper.dart';
import 'package:groom/screens/customer_screens/customer_support_details_screen.dart';
import 'package:groom/utils/utils.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/input_widget.dart';
import '../../firebase/complaint_firebase.dart';
import '../../utils/colors.dart';
import '../add_complaint_customer_screen.dart';

class CustomerSupportScreen extends StatefulWidget {
  const CustomerSupportScreen({super.key});

  @override
  State<CustomerSupportScreen> createState() => _CustomerSupportScreenState();
}

class _CustomerSupportScreenState extends State<CustomerSupportScreen> {
  TextEditingController descriptionController = TextEditingController();
  TextEditingController emailController = TextEditingController();
  ComplaintFirebase complaintFirebase = ComplaintFirebase();
  TextEditingController titleController = TextEditingController();
  bool isLoading = false;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      // floatingActionButton: FloatingActionButton(
      //     backgroundColor: mainBtnColor,
      //     child: Icon(
      //       Icons.add,
      //       color: colorwhite,
      //     ),
      //     onPressed: () {
      //       Navigator.push(
      //         context,
      //         MaterialPageRoute(
      //           builder: (builder) => AddComplaintCustomerScreen(),
      //         ),
      //       );
      //     }),
      appBar: AppBar(
        iconTheme: IconThemeData(color: colorwhite),
        title: HeaderTxtWidget(
          fontSize: 16,
          fontWeight: FontWeight.w700,
          // Line height divided by font size
          decoration: TextDecoration.none,
          "Submit Support Request",
          color: Colors.white,
        ),
      ),
      body: SingleChildScrollView(
        child: Column(
          children: [
            SizedBox(
              height: 20,
            ),
            Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                // Icon with question mark

                // Button Container
              ],
            ),
            Container(
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: Colors.blueAccent.withOpacity(0.1),
              ),
              child: Padding(
                padding: const EdgeInsets.all(8.0),
                child: SvgPicture.asset(AssetConstants.supporticon),
              ),
            ),
            const SizedBox(height: 10),
            // Main question text
            Text(
              "How we can help you today?",
              style: ThemeText.mediumText.copyWith(
                  fontFamily: 'Plus Jakarta Sans',
                  fontSize: 22,
                  fontWeight: FontWeight.w700,
                  height: 27.72 / 22, // Line height divided by font size
                  decoration: TextDecoration.none,
                  color: btnColorCode.withOpacity(1)),
              textAlign: TextAlign.center,
            ),
            const SizedBox(height: 8),
            // Subtitle text
            Padding(
              padding: const EdgeInsets.only(left: 20, right: 20),
              child: Center(
                child: Text(
                  "Please enter your personal data and describe your care needs or something we can help you with",
                  style: ThemeText.mediumText.copyWith(
                    fontSize: 14,
                    color: Color(0x8683A1).withOpacity(1),

                    fontWeight: FontWeight.w400,
                    height: 17.64 / 14, // Line height divided by font size
                    decoration: TextDecoration.none,
                  ),
                  textAlign: TextAlign.center,
                ),
              ),
            ),
            const SizedBox(height: 16),
            Container(
              padding: const EdgeInsets.only(left: 20, right: 20),
              child: AbsorbPointer(
                absorbing: isLoading,
                child: Column(
                  children: [
                    SizedBox(
                      height: 20,
                    ),
                    InputWidget(
                      controller: titleController,
                      title: "Complaint Title",
                      hint: "Enter complaint title",
                    ),
                    InputWidget(
                      controller: emailController,
                      title: "Email",
                      hint: "Enter your email",
                      inputType: TextInputType.emailAddress,
                    ),
                    InputWidget(
                      controller: descriptionController,
                      maxLines: 3,
                      title: "Description",
                      hint: "Write your issue",
                      inputType: TextInputType.emailAddress,
                    ),
                    const SizedBox(
                      height: 20,
                    ),
                    ButtonPrimaryWidget(
                      fontSize: 14,
                      fontWeight: FontWeight.w500,
                      'Submit',
                      isLoading: isLoading,
                      onTap: () async {
                        String complaintId = generateProjectId();
                        ComplaintModel complaintModel = ComplaintModel(
                            complaint: descriptionController.text,
                            title: titleController.text,
                            userId: FirebaseAuth.instance.currentUser!.uid,
                            complaintId: complaintId,
                            email: emailController.text,
                            createdOn: DateTime.now().millisecondsSinceEpoch);
                        await complaintFirebase
                            .writeCustomerComplaintToFirebase(
                          complaintId,
                          complaintModel,
                        );
                        Get.back(closeOverlays: true);
                      },
                    ),
                  ],
                ),
              ),
            ),

            // Row(
            //   mainAxisSize: MainAxisSize.max,
            //   children: [
            //     Text("Complaint Date"),
            //     SizedBox(
            //       width: 30,
            //     ),
            //     Text("Complaint Title")
            //   ],
            // ),
            // SizedBox(
            //   height: MediaQuery.of(context).size.height,
            //   child: FutureBuilder(
            //     future: complaintFirebase.getAllCustomerComplaintsByUserId(
            //         FirebaseAuth.instance.currentUser!.uid),
            //     builder: (context, snapshot) {
            //       if (snapshot.connectionState == ConnectionState.waiting) {
            //         return const Center(
            //           child: CircularProgressIndicator(),
            //         );
            //       } else if (snapshot.hasError) {
            //         return Text("Error: ${snapshot.error}");
            //       } else if (snapshot.hasData) {
            //         var complaints = snapshot.data as List<ComplaintModel>;
            //         if (complaints.isEmpty) {
            //           return Center(child: Text("No Complaints Found"));
            //         }
            //         return ListView.builder(
            //             itemCount: complaints.length,
            //             itemBuilder: (context, index) {
            //               var complaint = complaints[index];
            //               return GestureDetector(
            //                 onTap: () {
            //                   Get.to(
            //                     () => CustomerSupportDetailsScreen(
            //                       complaint: complaint,
            //                     ),
            //                   );
            //                 },
            //                 child: Card(
            //                   child: Row(
            //                     children: [
            //                       SizedBox(
            //                         width: 15,
            //                       ),
            //                       Expanded(
            //                         flex: 1,
            //                         child: Container(
            //                           child: Text(
            //                               formatDateInt(complaint.createdOn)),
            //                         ),
            //                       ),
            //                       SizedBox(
            //                         width: 25,
            //                       ),
            //                       Expanded(
            //                         flex: 4,
            //                         child: Container(
            //                           child: Text(complaint.title),
            //                         ),
            //                       ),
            //                       Expanded(
            //                         flex: 1,
            //                         child: Container(
            //                           child: Text(complaint.email),
            //                         ),
            //                       ),
            //                     ],
            //                   ),
            //                 ),
            //               );
            //             });
            //       }
            //       return Center(
            //         child: Text("No Data"),
            //       );
            //     },
            //   ),
            // ),
          ],
        ),
      ),
    );
  }
}
