import 'package:cached_network_image/cached_network_image.dart';
import 'package:flutter/material.dart';
import 'package:flutter_svg/svg.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';

import '../../../generated/assets.dart';
import '../../../utils/colors.dart';
import '../../../widgets/sub_txt_widget.dart';
import 'report_controller.dart';

class ProviderReportPage extends StatefulWidget {
  ProviderReportPage({super.key});

  @override
  State<ProviderReportPage> createState() => _ProviderReportPageState();
}

class _ProviderReportPageState extends State<ProviderReportPage> {
  final _con = Get.put(ReportController());

  @override
  void initState() {
    // TODO: implement initState
    super.initState();
    _con.provider.value = Get.arguments;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: primaryColorCode,
      appBar: AppBar(
        title: HeaderTxtWidget(
          'Report',
          color: Colors.white,
        ),
      ),
      body: Stack(
        children: [
          Positioned(
            top: 0,
            left: 0,
            right: 0,
            child: Image.asset(
              Assets.imgProfileBg,
              fit: BoxFit.cover,
              height: 220,
            ),
          ),
          Column(
            children: [
              Container(
                margin:
                const EdgeInsets.symmetric(horizontal: 10, vertical: 10),
                decoration: BoxDecoration(
                    color: "#565082".toColor().withAlpha(80),
                    borderRadius:
                    const BorderRadius.all(Radius.circular(10))),
                child: Row(
                  children: [
                    ClipRRect(
                      borderRadius: BorderRadius.circular(9),
                      child: SizedBox(
                        width: 100,
                        height: 100,
                        child: CachedNetworkImage(
                          imageUrl: _con.provider.value!.providerUserModel!
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
                            HeaderTxtWidget(
                              _con.provider.value!.providerUserModel!
                                  .providerType !=
                                  "Independent"
                                  ? _con.provider.value!.providerUserModel!
                                  .salonTitle!
                                  : _con.provider.value!.fullName,
                              color: Colors.white,
                            ),
                            Row(
                              children: [
                                SvgPicture.asset(Assets.svgLocationGray,
                                    color: Colors.white),
                                const SizedBox(
                                  width: 5,
                                ),
                                SubTxtWidget(
                                  '${_con.provider.value!.providerUserModel!.addressLine.toMiniAddress(limit: 20)} (${_con.provider.value!.distance})',
                                  color: Colors.white,
                                ),
                              ],
                            ),
                            Row(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              mainAxisAlignment: MainAxisAlignment.spaceBetween,
                              children: [
                                Row(
                                  children: [
                                    SvgPicture.asset(
                                      Assets.svgStar,
                                      color: Colors.white,
                                    ),
                                    const SizedBox(
                                      width: 5,
                                    ),
                                    SubTxtWidget(
                                      '${_con.provider.value!.providerUserModel!.overallRating}',
                                      color: Colors.white,
                                    ),
                                  ],
                                ),
                              ],
                            ),
                          ],
                        ))
                  ],
                ),
              ),
              Expanded(child: Container(
                margin: const EdgeInsets.only(top: 20),
                color: Colors.white,
                padding: const EdgeInsets.symmetric(horizontal: 15,vertical: 10),
                alignment: AlignmentDirectional.topStart,
                child: Column(
                  children: [
                    Expanded(child: SingleChildScrollView(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          SubTxtWidget('Tell us about the issue'),
                          Container(
                            padding: const EdgeInsets.all(10),
                            margin: const EdgeInsets.symmetric(vertical: 10),
                            height: 200,
                            alignment: AlignmentDirectional.topStart,
                            decoration: BoxDecoration(
                                border: Border.all(color: Colors.grey),
                                borderRadius: BorderRadius.circular(10)
                            ),
                            child: SingleChildScrollView(
                              child: Wrap(
                                children: _con.regionList.where((p0) => p0.selected,)
                                    .map(
                                      (element) => Container(
                                    margin: const EdgeInsets.symmetric(horizontal: 3,vertical: 3),
                                    padding: const EdgeInsets.symmetric(vertical: 3,horizontal: 6),
                                    decoration: BoxDecoration(
                                        borderRadius: BorderRadius.circular(10),
                                        color:element.selected?Colors.grey.shade300:Colors.white,
                                        border:element.selected?Border.all(color: Colors.grey):null
                                    ),
                                    child: InkWell(
                                      child: Wrap(
                                        alignment: WrapAlignment.center,
                                        children: [
                                          SubTxtWidget(element.region),
                                          const SizedBox(width: 5,),
                                          const Icon(Icons.cancel_outlined,size: 15,)
                                        ],
                                      ),
                                      onTap: () {
                                        setState(() {
                                          element.selected=false;
                                        });
                                      },
                                    ),
                                  ),
                                )
                                    .toList(),
                              ),
                            ),
                          ),
                          Wrap(
                            children: _con.regionList
                                .map(
                                  (element) => Container(
                                margin: const EdgeInsets.symmetric(horizontal: 3,vertical: 3),
                                padding: const EdgeInsets.symmetric(vertical: 3,horizontal: 6),
                                decoration: BoxDecoration(
                                    borderRadius: BorderRadius.circular(10),
                                    color:element.selected?Colors.grey.shade300:Colors.white,
                                    border:element.selected?Border.all(color: Colors.grey):null
                                ),
                                child: InkWell(
                                  child: SubTxtWidget(element.region),
                                  onTap: () {
                                    setState(() {
                                      element.selected=true;
                                    });
                                  },
                                ),
                              ),
                            )
                                .toList(),
                          )
                        ],
                      ),
                    )),
                    ButtonPrimaryWidget('Send',onTap: () {
                      _con.reportUser();
                    },),
                  ],
                ),
              ))
            ],
          ),
        ],
      ),
    );
  }
}
