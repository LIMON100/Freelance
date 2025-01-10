import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:groom/data_models/complaint_model.dart';
import 'package:groom/utils/utils.dart';
import '../firebase/complaint_firebase.dart';
import '../utils/colors.dart';
import '../widgets/signup_screen_widgets/button.dart';

class AddComplaintProviderScreen extends StatefulWidget {
  const AddComplaintProviderScreen({super.key});

  @override
  State<AddComplaintProviderScreen> createState() => _AddComplaintProviderScreenState();
}

class _AddComplaintProviderScreenState extends State<AddComplaintProviderScreen> {
  TextEditingController emailController = TextEditingController();
  TextEditingController titleController = TextEditingController();
  TextEditingController descriptionController = TextEditingController();
  ComplaintFirebase complaintFirebase = ComplaintFirebase();
  bool isLoading = false;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        iconTheme: IconThemeData(color: colorwhite),
        title: Text(
          "Complaint",
          style: GoogleFonts.workSans(
              color: colorwhite, fontSize: 18, fontWeight: FontWeight.w500),
        ),
        backgroundColor: mainBtnColor,
      ),
      body: Column(
        children: [
          Padding(
            padding: const EdgeInsets.only(left: 20.0, right: 20, top: 13),
            child: TextFormField(
              controller: titleController,
              decoration: InputDecoration(
                filled: true,
                hintText: "Complaint Title",
                fillColor: Color(0xffF6F7F9),
                enabledBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.all(Radius.circular(8)),
                  borderSide: BorderSide(
                    color: borderColor,
                  ),
                ),
                disabledBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.all(Radius.circular(8)),
                  borderSide: BorderSide(
                    color: borderColor,
                  ),
                ),
                focusedBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.all(Radius.circular(8)),
                  borderSide: BorderSide(
                    color: borderColor,
                  ),
                ),
              ),
            ),
          ),
          Padding(
            padding: const EdgeInsets.only(left: 20.0, right: 20, top: 13),
            child: TextFormField(
              controller: emailController,
              decoration: InputDecoration(
                filled: true,
                hintText: "Enter Your Email",
                fillColor: Color(0xffF6F7F9),
                enabledBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.all(Radius.circular(8)),
                  borderSide: BorderSide(
                    color: borderColor,
                  ),
                ),
                disabledBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.all(Radius.circular(8)),
                  borderSide: BorderSide(
                    color: borderColor,
                  ),
                ),
                focusedBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.all(Radius.circular(8)),
                  borderSide: BorderSide(
                    color: borderColor,
                  ),
                ),
              ),
            ),
          ),
          Padding(
            padding: const EdgeInsets.only(left: 20.0, right: 20, top: 13),
            child: TextFormField(
              maxLines: 3,
              controller: descriptionController,
              decoration: InputDecoration(
                filled: true,
                hintText: "Write Your Issue",
                fillColor: Color(0xffF6F7F9),
                enabledBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.all(Radius.circular(8)),
                  borderSide: BorderSide(
                    color: borderColor,
                  ),
                ),
                disabledBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.all(Radius.circular(8)),
                  borderSide: BorderSide(
                    color: borderColor,
                  ),
                ),
                focusedBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.all(Radius.circular(8)),
                  borderSide: BorderSide(
                    color: borderColor,
                  ),
                ),
              ),
            ),
          ),
          Padding(
            padding: const EdgeInsets.all(20.0),
            child: isLoading
                ? Center(
              child: CircularProgressIndicator(
                color: mainBtnColor,
              ),
            )
                : SaveButton(
              title: "Submit",
              onTap: () async {
                String complaintId = generateProjectId();

                ComplaintModel complaintModel = ComplaintModel(
                    complaint: descriptionController.text,
                    title: titleController.text,
                    userId: FirebaseAuth.instance.currentUser!.uid,
                    complaintId: complaintId,
                    email: emailController.text,
                    createdOn: DateTime.now().millisecondsSinceEpoch);
                await complaintFirebase.writeProviderComplaintToFirebase(
                  complaintId,
                  complaintModel,
                );
                Get.back(
                    closeOverlays: true
                );
              },
            ),
          )
        ],
      ),
    );
  }
}
