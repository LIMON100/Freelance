import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';

import '../../../generated/assets.dart';
import 'payment_controller.dart';

class PaymentPage extends StatelessWidget {
  PaymentPage({Key? key}) : super(key: key);
  final _con = Get.put(PaymentController());
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        backgroundColor: Colors.white,
        leading: IconButton(onPressed: () {
          Get.back();
        }, icon: const Icon(Icons.close,color: Colors.black,)),
      ),
      body: Container(
        padding: const EdgeInsets.symmetric(horizontal: 20,vertical: 10),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            HeaderTxtWidget('Payment methods',fontSize: 23,),
            const SizedBox(height: 20,),
            ListTile(
              contentPadding:  EdgeInsets.zero,
              leading: Image.asset(Assets.imgApplePay),
              title: HeaderTxtWidget('Apple Pay'),
              subtitle: SubTxtWidget("Coming soon",color: Colors.green,),
              onTap: () {

              },
            ),
            ListTile(
              contentPadding:  EdgeInsets.zero,
              leading: Image.asset(Assets.imgAffirm),
              title: HeaderTxtWidget('Affirm'),
              subtitle: SubTxtWidget("Coming soon",color: Colors.green,),
              onTap: () {

              },
            ),
            ListTile(
              contentPadding:  EdgeInsets.zero,
              leading: Image.asset(Assets.imgCreditCard),
              title: HeaderTxtWidget('Credit or debit card'),
              onTap: () {
                if(_con.type=="AcceptOffer"){
                  _con.payWithCard(context,amount: _con.requestOfferModel!.depositAmount.toString(),
                  description: _con.requestOfferModel!.description);
                }else {
                  _con.payWithCard(context,amount: _con.bookingState.selectedBooking.value.serviceDetails!.servicePrice.toString(),
                      description:_con.bookingState.selectedBooking.value.serviceDetails!.description);
                }
                },
            ),
            ListTile(
              contentPadding:  EdgeInsets.zero,
              leading: Image.asset(Assets.imgCash),
              title: HeaderTxtWidget('Cash'),
              trailing: Icon(Icons.arrow_forward_ios_outlined,size: 15,color: Colors.grey.shade400,),
              onTap: () {
                if(_con.type=="AcceptOffer"){
                  _con.acceptOffer(context, paymentType: "cash", transactionId: "");
                }else {
                  _con.confirmReservation(
                      context, paymentType: "cash", transactionId: "");
                }
              },
            ),
          ],
        ),
      ),
    );
  }
}
