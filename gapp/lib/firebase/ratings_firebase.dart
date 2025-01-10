
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import '../constant/global_configuration.dart';
import '../data_models/rating_model.dart';

  class RatingsFirebase {
    final DatabaseReference _databaseReference = FirebaseDatabase.instanceFor(
        app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'))
        .ref();


  Future<void> addRating(String serviceId, RatingModel rating) async {
    try {
      await _databaseReference.child('ratings').child(serviceId).set(rating.toJson());
    } catch (e) {
      print('Failed to add rating: $e');
    }
  }
  Future<List<RatingModel>> getRatingsByProviderId(String providerId) async {
    List<RatingModel> ratings = [];

    var source = await _databaseReference
        .child('ratings')
        .orderByChild('providerId')
        .equalTo(providerId)
        .once();

    var data = source.snapshot.value as Map<dynamic, dynamic>?;

    if (data != null) {
      data.forEach((key, value) {
        var ratingModel = RatingModel.fromJson(
          Map<String, dynamic>.from(value),
        );
        ratings.add(ratingModel);
      });
    }

    return ratings;
  }
}
