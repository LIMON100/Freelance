import 'package:get/get.dart';
import 'package:groom/data_models/service_booking_model.dart';

class BookingState extends GetxController {
  var bookings = <ServiceBookingModel>[].obs;
  var isLoading=false.obs;
  var selectedBooking = ServiceBookingModel(
          bookingId: "bookingId",
          serviceId: "serviceId",
          providerId: "providerId",
          clientId: "clientId",
          createdOn: 0,
          status: "status",
          selectedDate: '',selectedTime: "")
      .obs;
}
