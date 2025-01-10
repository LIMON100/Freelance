import 'package:cached_network_image/cached_network_image.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/Utils/tools.dart';
import 'package:groom/data_models/user_model.dart';
import 'package:groom/firebase/provider_service_firebase.dart';
import 'package:groom/firebase/user_firebase.dart';
import 'package:groom/screens/chat_screen.dart';
import 'package:groom/states/customer_request_state.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/loading_widget.dart';
import 'package:groom/widgets/read_more_text.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../data_models/provider_service_request_offer_model.dart';
import '../../../data_models/request_reservation_model.dart';
import '../../../firebase/customer_offer_firebase.dart';
import '../../../states/provider_request_state.dart';
import '../../../states/provider_service_request_state.dart';
import '../../../widgets/network_image_widget.dart';
import '../service_request/customer_create_offer_screen.dart';

class CustomerServiceRequestViewScreen extends StatefulWidget {
  const CustomerServiceRequestViewScreen({super.key});

  @override
  State<CustomerServiceRequestViewScreen> createState() =>
      _CustomerServiceRequestViewScreenState();
}

class _CustomerServiceRequestViewScreenState
    extends State<CustomerServiceRequestViewScreen> {
  CustomerRequestState customerRequestState = Get.find();
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  UserFirebase userFirebase = UserFirebase();
  ProviderRequestState providerRequestState = ProviderRequestState();
  ProviderServiceRequestState providerServiceRequestState =
      Get.put(ProviderServiceRequestState());
  CustomerOfferFirebase customerOfferFirebase = CustomerOfferFirebase();
  RxBool showEditButton=false.obs;
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        title:HeaderTxtWidget('${customerRequestState.selectedCustomerRequest.value.serviceName}'),
        iconTheme: IconThemeData(
          color: primaryColorCode
        ),
        actions: [
          Obx(() => Visibility(visible: showEditButton.value,child: IconButton(onPressed: (){
            Get.to(() =>  CustomerCreateOfferScreen(data: customerRequestState.selectedCustomerRequest.value,),);
          }, icon: const Icon(Icons.edit)),),)
        ],
      ),
      body: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          NetworkImageWidget(
            url: customerRequestState.selectedCustomerRequest.value.offerImages!.isEmpty?"":customerRequestState.selectedCustomerRequest.value.offerImages!.first,
            height: 200,
            fit: BoxFit.cover,),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 15,vertical: 10),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                HeaderTxtWidget('${customerRequestState.selectedCustomerRequest.value.serviceName}'),
                SubTxtWidget('${customerRequestState.selectedCustomerRequest.value.serviceType}',fontSize: 14,),
                ReadMoreText(customerRequestState.selectedCustomerRequest.value.description),
                const SizedBox(height: 10,),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  mainAxisSize: MainAxisSize.max,
                  children: [
                    Row(
                      children: [
                        const Icon(Icons.calendar_month),
                        Text(Tools.changeDateFormat(customerRequestState
                            .selectedCustomerRequest.value.dateTime.toString(), "MM-dd-yyyy"),
                        ),
                      ],
                    ),
                    Row(
                      children: [
                        const Icon(Icons.timelapse),
                        Text(
                          customerRequestState.selectedCustomerRequest.value.selectedTime!,
                        ),
                      ],
                    ),
                  ],
                ),
              ],
            ),
          ),
          Padding(
            padding: const EdgeInsets.all(15.0),
            child: HeaderTxtWidget(
              "Offers Received ",
            ),
          ),
          Expanded(child: _request())
        ],
      ),
    );
  }
  Widget _request(){
    if(customerRequestState.selectedCustomerRequest.value.status=="accepted"){
      showEditButton.value=false;
      return FutureBuilder(
        future: providerServiceFirebase.getSingleRequestOfferByRequestId(customerRequestState.selectedCustomerRequest.value.acceptedRequestId!),
        builder: (context, snapshot) {
          if (snapshot.connectionState == ConnectionState.waiting) {
            return const Text("");
          }
          else if (snapshot.hasError) {
            return Text("Error: ${snapshot.error}");
          }
          var request=snapshot.data!;
          return FutureBuilder(
            future: userFirebase.getUser(request.providerId),
            builder: (context, snapshot) {
              if (snapshot.connectionState == ConnectionState.waiting) {
                return const Text("");
              } else if (snapshot.hasError) {
                return Text("Error: ${snapshot.error}");
              } else if (snapshot.hasData) {
                var user = snapshot.data as UserModel;
                return Wrap(
                  children: [
                    ListTile(
                      leading: ClipRRect(
                        borderRadius: BorderRadius.circular(12),
                        child: NetworkImageWidget(url: user.photoURL??"",
                          width: 60,height: 60,),
                      ),
                      title: user.providerUserModel!.providerType !=
                          "Salon"
                          ? Text(
                        user.fullName.capitalizeFirst!,
                        style: const TextStyle(
                            fontSize: 20,
                            fontWeight: FontWeight.bold),
                      )
                          : Text(
                        user.providerUserModel!.salonTitle!
                            .capitalizeFirst!,
                        style: const TextStyle(
                            fontSize: 20,
                            fontWeight: FontWeight.bold),
                      ),
                      subtitle: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(request.description),
                          const Text("Accepted"),
                        ],
                      ),
                      trailing: Column(
                        mainAxisAlignment: MainAxisAlignment.start,
                        children: [
                          const Text(
                            "Deposit :",
                            style: TextStyle(
                                fontWeight: FontWeight.bold,
                                fontSize: 16,
                                color: Colors.black),
                          ),
                          request.deposit != false
                              ? Text(
                            "${request.depositAmount.toString()} \$",
                            style: const TextStyle(
                                fontWeight: FontWeight.bold,
                                fontSize: 14,
                                color: Colors.black),
                          )
                              : const Text(
                            "0 \$",
                            style: TextStyle(
                                fontWeight: FontWeight.bold,
                                fontSize: 14,
                                color: Colors.black),
                          ),
                        ],
                      ),
                    )
                  ],
                );
              }
              return Text("");
            },
          );
        },
      );
    }
    return FutureBuilder(
      future: getList(),
      builder: (context, snapshot) {
        if (snapshot.connectionState == ConnectionState.waiting) {
          return LoadingWidget();
        }
        if(snapshot.data!.isEmpty){
          return  Container(
            padding: const EdgeInsets.symmetric(horizontal: 20,vertical: 10),
            child: const Text("No Request found"),
          );
        }
        else if (snapshot.hasError) {
          return Text("Error: ${snapshot.error}");
        } else if (snapshot.hasData) {
          var requests = snapshot.data as List<ProviderServiceRequestOfferModel>;

          return ListView.builder(
            itemCount: requests.length,
            itemBuilder: (context, index) {
              var request = requests[index];
              return Card(
                  child: FutureBuilder(
                    future: userFirebase.getUser(request.providerId),
                    builder: (context, snapshot) {
                      if (snapshot.connectionState == ConnectionState.waiting) {
                        return const Text("");
                      } else if (snapshot.hasError) {
                        return  Container(
                          alignment: AlignmentDirectional.center,
                          padding: const EdgeInsets.all(10),
                          child: const Text("No data found"),
                        );
                      } else if (snapshot.hasData) {
                        var user = snapshot.data as UserModel;
                        return ListTile(
                          leading: ClipRRect(
                            borderRadius: BorderRadius.circular(12),
                            child: user.photoURL != null
                                ? CachedNetworkImage(
                              imageUrl: user.photoURL!,
                              fit: BoxFit.fill,
                              width: 60,
                              placeholder: (context, url) =>
                              const CircularProgressIndicator(),
                              errorWidget: (context, url, error) =>
                                  const Icon(Icons.error),
                            )
                                : const Icon(Icons.person, size: 60),
                          ),
                          title: user.providerUserModel!.providerType !=
                              "Salon"
                              ? Text(
                            user.fullName.capitalizeFirst!,
                            style: const TextStyle(
                                fontSize: 20,
                                fontWeight: FontWeight.bold),
                          )
                              : Text(
                            user.providerUserModel!.salonTitle!
                                .capitalizeFirst!,
                            style: const TextStyle(
                                fontSize: 20,
                                fontWeight: FontWeight.bold),
                          ),
                          subtitle: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Text(request.description),
                              Row(
                                mainAxisAlignment:
                                MainAxisAlignment.spaceBetween,
                                children: [
                                  TextButton(
                                    style: ButtonStyle(
                                        padding: WidgetStateProperty.all(
                                          EdgeInsets.all(2),
                                        ),
                                        backgroundColor:
                                        WidgetStateProperty.all(
                                            mainBtnColor)),
                                    onPressed: () async{
                                      Get.toNamed("/payment_screen",arguments: {
                                        "type":"AcceptOffer",
                                        "requestOfferModel":request.toJson()
                                      });
                                    },
                                    child: const Text(
                                      "Accept",
                                      style: TextStyle(
                                          fontWeight: FontWeight.w500,
                                          fontSize: 14,
                                          color: Colors.black),
                                    ),
                                  ),
                                  TextButton(
                                    style: ButtonStyle(
                                        padding: WidgetStateProperty.all(const EdgeInsets.all(2),),
                                        backgroundColor:
                                        WidgetStateProperty.all(
                                            mainBtnColor)),
                                    onPressed: () {
                                      Get.to(() => ChatDetailScreen(
                                          userId: FirebaseAuth
                                              .instance.currentUser!.uid,
                                          friendId: user.uid));
                                    },
                                    child: Text(
                                      "Chat",
                                      style: TextStyle(
                                          fontWeight: FontWeight.w500,
                                          fontSize: 14,
                                          color: Colors.black),
                                    ),
                                  ),
                                ],
                              ),
                            ],
                          ),
                          trailing: Column(
                            mainAxisAlignment: MainAxisAlignment.start,
                            children: [
                              Text(
                                "Deposit :",
                                style: TextStyle(
                                    fontWeight: FontWeight.bold,
                                    fontSize: 16,
                                    color: Colors.black),
                              ),
                              request.deposit != false
                                  ? Text(
                                "${request.depositAmount.toString()} \$",
                                style: TextStyle(
                                    fontWeight: FontWeight.bold,
                                    fontSize: 14,
                                    color: Colors.black),
                              )
                                  : Text(
                                "0 \$",
                                style: TextStyle(
                                    fontWeight: FontWeight.bold,
                                    fontSize: 14,
                                    color: Colors.black),
                              ),
                            ],
                          ),
                        );
                      }
                      return Text("");
                    },
                  ));
            },
          );
        }
        return  Container(
          padding: const EdgeInsets.all(20),
          child: const Text("No Request found"),
        );
      },
    );
  }
  Future<List<ProviderServiceRequestOfferModel>> getList() async {
     var list=await providerServiceFirebase.getRequestOfferByServiceId(customerRequestState.selectedCustomerRequest.value.offerId);
     showEditButton.value=list.isEmpty;
  return list;
  }
}
