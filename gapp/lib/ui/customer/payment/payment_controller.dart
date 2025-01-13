import 'package:get/get.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/data_models/user_model.dart';
import 'package:groom/repo/setting_repo.dart';

import '../../../data_models/notification_model.dart';
import '../../../data_models/provider_service_request_offer_model.dart';
import '../../../data_models/request_reservation_model.dart';
import '../../../data_models/service_reservation_model.dart';
import '../../../firebase/customer_offer_firebase.dart';
import '../../../firebase/firebase_notifications.dart';
import '../../../firebase/notifications_firebase.dart';
import '../../../firebase/reservation_firebase.dart';
import '../../../firebase/service_booking_firebase.dart';
import '../../../firebase/transaction_firebase.dart';
import '../../../firebase/user_firebase.dart';
import '../../../repo/stripre_repo.dart';
import '../../../states/booking_state.dart';
import '../../../states/customer_request_state.dart';
import '../../../utils/utils.dart';
import '../customer_dashboard/customer_dashboard_page.dart';

class PaymentController extends GetxController {
  CustomerRequestState customerRequestState = Get.find();
  ReservationFirebase reservationFirebase = ReservationFirebase();
  NotificationsFirebase notificationsFirebase = NotificationsFirebase();
  ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();
  CustomerOfferFirebase customerOfferFirebase = CustomerOfferFirebase();
  TransactionFirebase transactionFirebase = TransactionFirebase();
  BookingState bookingState = Get.put(BookingState());
  UserFirebase userFirebase = UserFirebase();
  String? type;
  ProviderServiceRequestOfferModel? requestOfferModel;

  @override
  void onInit() {
    // TODO: implement onInit
    super.onInit();
    if(Get.arguments!=null) {
      type = Get.arguments['type'];
      if (type == "AcceptOffer") {
        requestOfferModel = ProviderServiceRequestOfferModel.fromJson(Get.arguments['requestOfferModel']);
      }
    }
    print("type==>$type");
  }

  void payWithCard(context,{required String amount,required String description,}) {
    stripeMakePayment(
        amount: amount,
        description: description,
        paymentFailed: () {
          Tools.ShowErrorMessage("Payment failed");
        },
        paymentSuccess: (data) {
          if (type == "AcceptOffer") {
            acceptOffer(context,paymentType: "card", transactionId: data['id']);
          } else {
            confirmReservation(context, paymentType: "card", transactionId: data['id']);
          }
        },
        username: auth.value.fullName);
  }

  Future<void> confirmReservation(context,
      {required String paymentType, required String transactionId}) async {
    String reservationIdGen = generateProjectId();
    ServiceReservationModel reservationModel = ServiceReservationModel(
        selectedDate: bookingState.selectedBooking.value.selectedDate,
        bookingId: bookingState.selectedBooking.value.bookingId,
        reservationId: reservationIdGen,
        paymentType: paymentType,
        transactionId: transactionId,
        clientId: bookingState.selectedBooking.value.clientId,
        providerId: bookingState.selectedBooking.value.serviceDetails!.userId,
        serviceId: bookingState.selectedBooking.value.serviceId,
        createdOn: DateTime.now().millisecondsSinceEpoch,
        status: "Reserved",
        review_status: "0",
        serviceDetails: bookingState.selectedBooking.value.serviceDetails);

    reservationFirebase.writeServiceReservationToFirebase(
        reservationIdGen, reservationModel);
    serviceBookingFirebase.updateBookingStatus(
        bookingState.selectedBooking.value.bookingId, "Reserved");

    transactionFirebase.addTransactionToFirebase({
      "userId":auth.value.uid,
      "providerId":bookingState.selectedBooking.value.serviceDetails!.userId,
      "amount":bookingState.selectedBooking.value.serviceDetails!.servicePrice,
      "transactionId":transactionId,
      "paymentMode":paymentType,
      "createdDate":DateTime.now().toString(),
      "paymentFor":bookingState.selectedBooking.value.toJson(),
    });

    UserModel _providerUser = await userFirebase.getUser(bookingState.selectedBooking.value.providerId);

    PushNotificationService.sendCustomerConfirmReservationNotificationToProvider(
            _providerUser.notificationToken!, context, "", auth.value.fullName);

    NotificationModel notificationModel = NotificationModel(
        userId: _providerUser.uid,
        createdOn: DateTime.now().millisecondsSinceEpoch,
        notificationText:
            "Payment By your customer has been received and Your Reservation is confirmed ",
        type: "BOOKING",
        sendBy: "USER",
        serviceId: bookingState.selectedBooking.value.serviceId,
        notificationId: generateProjectId());

    notificationsFirebase.writeNotificationToUser(
        _providerUser.uid, notificationModel, generateProjectId());
    Get.to(
      () => CustomerDashboardPage(),
    );
  }

  Future<void> acceptOffer(context,
      {required String paymentType, required String transactionId}) async {
    String reservationIdGen = generateProjectId();
    RequestReservationModel requestReservationModel = RequestReservationModel(
        selectedDate: requestOfferModel!.selectedDate,
        reservationId: reservationIdGen,
        clientId: requestOfferModel!.clientId,
        providerId: requestOfferModel!.providerId,
        offerId: requestOfferModel!.serviceId,
        createdOn: DateTime.now().millisecondsSinceEpoch,
        requestId: requestOfferModel!.requestId,
        paymentType: paymentType,
        status: "accepted",
        transactionId: transactionId,
        deposit: requestOfferModel!.deposit,
        description: requestOfferModel!.description,
        depositAmount: requestOfferModel!.depositAmount);
    reservationFirebase.writeRequestReservationToFirebase(
        reservationIdGen, requestReservationModel);
    customerOfferFirebase.updateOffer(requestOfferModel!.serviceId,{
      "status":"accepted",
      "acceptedRequestId":requestOfferModel!.requestId,
    });

    transactionFirebase.addTransactionToFirebase({
      "userId":auth.value.uid,
      "providerId":requestOfferModel!.clientId,
      "amount":requestOfferModel!.depositAmount,
      "transactionId":transactionId,
      "paymentMode":paymentType,
      "createdDate":DateTime.now().toString(),
      "paymentFor":requestOfferModel!.toJson(),
    });

    UserModel _providerModel = await userFirebase.getUser(requestOfferModel!.clientId);
    customerRequestState.selectedCustomerRequest.value.status="accepted";
    customerRequestState.selectedCustomerRequest.value.acceptedRequestId=requestOfferModel!.requestId;

    if(_providerModel.notification_status!) {
      PushNotificationService.sendCustomerAcceptPaymentNotificationToProvider(
          _providerModel.notificationToken!,
          context,
          "tripId",
          auth.value.fullName);
    }
    NotificationModel notificationModel = NotificationModel(
        userId: _providerModel.uid,
        createdOn: DateTime.now().millisecondsSinceEpoch,
        notificationText: "A payment has been accepted  to your service",
        type: "OFFER",
        sendBy: "USER",
        serviceId: customerRequestState.selectedCustomerRequest.value.offerId,
        notificationId: generateProjectId());
    notificationsFirebase.writeNotificationToUser(
        _providerModel.uid, notificationModel, generateProjectId());
    Get.to(() => CustomerDashboardPage());
  }
}
