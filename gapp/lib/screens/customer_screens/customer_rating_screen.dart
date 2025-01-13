import 'package:flutter/material.dart';
import 'package:flutter_rating_bar/flutter_rating_bar.dart';
import 'package:get/get.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/utils/utils.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import '../../firebase/provider_user_firebase.dart';
import '../../firebase/ratings_firebase.dart';
import '../../data_models/rating_model.dart';
import '../../firebase/reservation_firebase.dart';

class FeedbackScreen extends StatefulWidget {
  final String serviceId;
  final String providerId;
  Function()? onSuccess;

  FeedbackScreen(
      {required this.serviceId, required this.providerId, this.onSuccess});

  @override
  _FeedbackScreenState createState() => _FeedbackScreenState();
}

class _FeedbackScreenState extends State<FeedbackScreen> {
  double _ratingValue = 0.0;
  TextEditingController _commentController = TextEditingController();
  bool _isLoading = false;
  ReservationFirebase reservationFirebase = ReservationFirebase();

  void _submitRating() async {
    if (_ratingValue == 0.0) {
      Tools.ShowErrorMessage("Please provide a rating");
      return;
    }
    setState(() {
      _isLoading = true;
    });
    RatingModel rating = RatingModel(
      ratingId: generateServiceBookingId(),
      providerId: widget.providerId,
      ratingValue: _ratingValue,
      userId: auth.value.uid,
      comment: _commentController.text,
      createdOn: DateTime.now().millisecondsSinceEpoch,
    );
    await RatingsFirebase().addRating(widget.serviceId, rating);
    await ProviderUserFirebase().updateProviderOverallRating(widget.providerId, _ratingValue);
    Tools.ShowSuccessMessage("Thank you for your feedback!");
    setState(() {
      _isLoading = false;
    });
    if (widget.onSuccess != null) {
      widget.onSuccess!.call();
    }
    Navigator.pop(context);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: HeaderTxtWidget("Rate the Service",color: Colors.white,),
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              "How would you rate the service?",
              style: GoogleFonts.lato(
                textStyle: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
              ),
            ),
            SizedBox(height: 16),
            RatingBar.builder(
              initialRating: 0,
              minRating: 1,
              direction: Axis.horizontal,
              allowHalfRating: true,
              itemCount: 5,
              itemPadding: EdgeInsets.symmetric(horizontal: 4.0),
              itemBuilder: (context, _) => Icon(
                Icons.star,
                color: Colors.amber,
              ),
              onRatingUpdate: (rating) {
                setState(() {
                  _ratingValue = rating;
                });
              },
            ),
            SizedBox(height: 16),
            TextField(
              controller: _commentController,
              maxLines: 3,
              decoration: InputDecoration(
                labelText: "Leave a comment (optional)",
                border: OutlineInputBorder(),
              ),
            ),
            SizedBox(height: 16),
            _isLoading
                ? Center(child: CircularProgressIndicator())
                : ElevatedButton(
                    onPressed: _submitRating,
                    child: Text("Submit"),
                  ),
          ],
        ),
      ),
    );
  }
}
