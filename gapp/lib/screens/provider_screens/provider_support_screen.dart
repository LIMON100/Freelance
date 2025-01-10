import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:groom/data_models/complaint_model.dart';
import 'package:groom/screens/customer_screens/customer_support_details_screen.dart';
import 'package:groom/utils/utils.dart';
import '../../firebase/complaint_firebase.dart';
import '../../utils/colors.dart';
import '../add_complaint_customer_screen.dart';

class ProviderSupportScreen extends StatefulWidget {
  const ProviderSupportScreen({super.key});

  @override
  State<ProviderSupportScreen> createState() => _ProviderSupportScreenState();
}

class _ProviderSupportScreenState extends State<ProviderSupportScreen> {
  TextEditingController descriptionController = TextEditingController();
  TextEditingController emailController = TextEditingController();
  ComplaintFirebase complaintFirebase = ComplaintFirebase();
  bool isLoading = false;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      floatingActionButton: FloatingActionButton(
          backgroundColor: mainBtnColor,
          child: Icon(
            Icons.add,
            color: colorwhite,
          ),
          onPressed: () {
            Navigator.push(context,
                MaterialPageRoute(builder: (builder) => AddComplaintCustomerScreen()));
          }),
      appBar: AppBar(
        iconTheme: IconThemeData(color: colorwhite),
        title: Text(
          "Support",
          style: GoogleFonts.workSans(
              color: colorwhite, fontSize: 18, fontWeight: FontWeight.w500),
        ),
        backgroundColor: mainBtnColor,
      ),
      body: SingleChildScrollView(
        child: Column(
          children: [
            Row(
              mainAxisSize: MainAxisSize.max,
              children: [
                Text("Complaint Date"),
                SizedBox(width: 30,),Text("Complaint Title")
              ],
            ),
            SizedBox(
                height: MediaQuery.of(context).size.height,
                child: FutureBuilder(
                  future: complaintFirebase.getAllProviderComplaintsByUserId(
                      FirebaseAuth.instance.currentUser!.uid),
                  builder: (context, snapshot) {
                    if (snapshot.connectionState == ConnectionState.waiting) {
                      return const Center(
                        child: CircularProgressIndicator(),
                      );
                    } else if (snapshot.hasError) {
                      return Text("Error: ${snapshot.error}");
                    } else if (snapshot.hasData) {
                      var complaints = snapshot.data as List<ComplaintModel>;
                      if (complaints.isEmpty) {
                        return Center(child: Text("No Complaints Found"));
                      }
                      return ListView.builder(itemCount: complaints.length,itemBuilder: (context, index) {

                        var complaint = complaints[index];
                        return GestureDetector(
                          onTap: (){
                            Get.to(()=>CustomerSupportDetailsScreen(complaint: complaint,));
                          }
                          ,child: Card(
                          child: Row(
                            children: [
                              SizedBox(width: 15,),

                              Expanded(
                                flex: 1,
                                child: Container(
                                  child: Text(formatDateInt(complaint.createdOn)),
                                ),
                              ),
                              SizedBox(width: 25,),
                              Expanded(
                                flex: 4,
                                child: Container(
                                  child: Text(complaint.title),
                                ),
                              ),
                              Expanded(
                                flex: 1,
                                child: Container(
                                  child: Text(complaint.email),
                                ),
                              ),
                            ],
                          ),
                        ),
                        );
                      });
                    }
                    return Center(child: Text("No Data"),);
                  },
                )),
          ],
        ),
      ),
    );
  }
}
