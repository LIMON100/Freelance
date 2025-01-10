import 'dart:convert';

import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_database/ui/firebase_animated_list.dart';
import 'package:flutter/material.dart';
import 'package:flutter_rating_bar/flutter_rating_bar.dart';
import 'package:groom/firebase/user_firebase.dart';
import 'package:groom/widgets/loading_widget.dart';
import '../../../Constant/global_configuration.dart';
import '../../../repo/setting_repo.dart';
import '../../../utils/utils.dart';
import '../../../widgets/header_txt_widget.dart';
import '../../../widgets/network_image_widget.dart';
import '../../../widgets/sub_txt_widget.dart';
import 'model/review_model.dart';

class ReviewList extends StatefulWidget {
  const ReviewList({super.key});

  @override
  State<ReviewList> createState() => _ListState();
}

class _ListState extends State<ReviewList> {
  final _ref = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'));
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: HeaderTxtWidget("Review List",color: Colors.white,),
      ),
      body:_body(),
    );
  }
  Widget _body(){
    return FirebaseAnimatedList(query: _ref.ref("ratings").orderByChild("providerId").equalTo(auth.value.uid),
      itemBuilder: (context, snapshot, animation, index) {
      if(!snapshot.exists){
        return Center(
          child: HeaderTxtWidget("No review found"),
        );
      }
      print("review==>${snapshot.value}");
      if(snapshot.value==null){
        return Center(
          child: HeaderTxtWidget("No review found"),
        );
      }
      ReviewModel? data = ReviewModel.fromJson(jsonDecode(jsonEncode(snapshot.value)));
        return FutureBuilder(future: UserFirebase().getUser(data.userId!),
          builder: (context, snapshot) {
          if(snapshot.data==null){
            return const Center(
              child: CircularProgressIndicator(),
            );
          }
          return Container(
            padding: const EdgeInsets.all(10),
            margin: const EdgeInsets.all(10),
            decoration: const BoxDecoration(
              borderRadius: BorderRadius.all(Radius.circular(10)),
              color: Colors.white
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    ClipRRect(
                      borderRadius: const BorderRadius.all(Radius.circular(30)),
                      child: NetworkImageWidget(url: snapshot.data!.photoURL,
                        height: 50,width: 50,),
                    ),
                    const SizedBox(width: 10,),
                    Expanded(child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      mainAxisAlignment: MainAxisAlignment.start,
                      children: [
                        HeaderTxtWidget(snapshot.data!.fullName),
                        RatingBarIndicator(itemBuilder: (context, index) {
                          return const Icon(Icons.star,color: Colors.amber,);
                        },itemCount: 5,rating: data.ratingValue??0,
                          itemSize: 20,)
                      ],
                    ))
                  ],
                ),
                const SizedBox(height: 5,),
                SubTxtWidget(data.comment!),
                SubTxtWidget(formatDateInt(data.createdOn!),fontSize: 12,),
              ],
            ),
          );
        },);
    },defaultChild: LoadingWidget(),);
  }
}