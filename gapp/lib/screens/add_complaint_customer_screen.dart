import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/data_models/complaint_model.dart';
import 'package:groom/utils/utils.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/input_widget.dart';
import '../firebase/complaint_firebase.dart';

class AddComplaintCustomerScreen extends StatefulWidget {
  const AddComplaintCustomerScreen({super.key});

  @override
  State<AddComplaintCustomerScreen> createState() => _AddComplaintCustomerScreenState();
}

class _AddComplaintCustomerScreenState extends State<AddComplaintCustomerScreen> {
  TextEditingController emailController = TextEditingController();
  TextEditingController titleController = TextEditingController();
  TextEditingController descriptionController = TextEditingController();
  ComplaintFirebase complaintFirebase = ComplaintFirebase();
  bool isLoading = false;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: HeaderTxtWidget(
          "Complaint",
          color: Colors.white,
          ),
      ),
      body: Container(
        padding: const EdgeInsets.all(20),
        child: AbsorbPointer(
          absorbing: isLoading,
          child: Column(
            children: [
              InputWidget(
                controller: titleController,
                title: "Complaint Title",
                hint: "Enter complaint title",
              ),InputWidget(
                controller: emailController,
                title: "Email",
                hint: "Enter your email",
                inputType: TextInputType.emailAddress,
              ),InputWidget(
                controller: descriptionController,
                title: "Description",
                hint: "Write your issue",
                inputType: TextInputType.emailAddress,
              ),
               const SizedBox(height: 20,),
              ButtonPrimaryWidget('Submit',isLoading: isLoading,onTap: () async {
                String complaintId = generateProjectId();
                ComplaintModel complaintModel = ComplaintModel(
                    complaint: descriptionController.text,
                    title: titleController.text,
                    userId: FirebaseAuth.instance.currentUser!.uid,
                    complaintId: complaintId,
                    email: emailController.text,
                    createdOn: DateTime.now().millisecondsSinceEpoch);
                await complaintFirebase.writeCustomerComplaintToFirebase(
                  complaintId,
                  complaintModel,
                );
                Get.back(
                    closeOverlays: true
                );
              },),
            ],
          ),
        ),
      ),
    );
  }
}
