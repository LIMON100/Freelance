import 'dart:convert';

import 'package:cached_network_image/cached_network_image.dart';
import 'package:firebase_database/ui/firebase_animated_list.dart';
import 'package:flutter/material.dart';
import 'package:flutter_rating_bar/flutter_rating_bar.dart';
import 'package:flutter_svg/svg.dart';
import 'package:get/get.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/loading_widget.dart';
import '../../../utils/tools.dart';
import '../../../firebase/user_firebase.dart';
import '../../../widgets/network_image_widget.dart';
import '../../../widgets/sub_txt_widget.dart';
import 'm/report_model.dart';
import 'report_controller.dart';

class ReportPage extends StatelessWidget {
   ReportPage({super.key});
  final _con = Get.put(ReportController());
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: HeaderTxtWidget('Report List',color: Colors.white,),
      ),
      body: FirebaseAnimatedList(query: _con.getList(), itemBuilder: (context, snapshot, animation, index) {
        if(!snapshot.exists){
          return Center(
            child: HeaderTxtWidget("No review found"),
          );
        }
        ReportModel? data = ReportModel.fromJson(jsonDecode(jsonEncode(snapshot.value)));
        return FutureBuilder(future: UserFirebase().getUser(data.providerId!),
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
              child: Row(
                children: [
                  ClipRRect(
                    borderRadius: BorderRadius.circular(9),
                    child: SizedBox(
                      width: 100,
                      height: 100,
                      child: CachedNetworkImage(
                        imageUrl: snapshot.data!.providerUserModel!
                            .providerImages!.first,
                        fit: BoxFit.fill,
                      ),
                    ),
                  ),
                  const SizedBox(
                    width: 10,
                  ),
                  Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Row(
                            children: [
                              Expanded(child: HeaderTxtWidget(
                                snapshot.data!.providerUserModel!
                                    .providerType !=
                                    "Independent"
                                    ? snapshot.data!.providerUserModel!
                                    .salonTitle!
                                    : snapshot.data!.fullName,
                              ),),
                              Container(
                                margin: const EdgeInsets.symmetric(horizontal: 5),
                                padding: const EdgeInsets.symmetric(horizontal: 5,vertical: 3),
                                decoration: const BoxDecoration(
                                  borderRadius: BorderRadius.all(Radius.circular(10)),
                                  color: Colors.red,
                                ),
                                child: SubTxtWidget(data.status!,fontSize: 12,color: Colors.white,),
                              )
                            ],
                          ),
                          SubTxtWidget(data.region!,maxLines: 2,overflow: TextOverflow.ellipsis,),
                          SubTxtWidget(Tools.changeDateFormat(data.createdDate!, "MM/dd/yyyy")),
                        ],
                      ))
                ],
              ),
            );
          },);
      },shrinkWrap: true,
      defaultChild: LoadingWidget(),),
    );
  }
}
