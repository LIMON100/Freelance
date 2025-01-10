import 'dart:convert';
import 'dart:io';

import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_storage/firebase_storage.dart';
import 'package:geolocator/geolocator.dart';
import 'package:groom/repo/setting_repo.dart';

import '../constant/global_configuration.dart';
import '../data_models/provider_service_model.dart';
import '../data_models/provider_service_request_offer_model.dart';
import '../data_models/service_booking_model.dart';
import '../data_models/user_model.dart';

class ProviderServiceFirebase {
  final _ref = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'));
final _databaseReference = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL')).ref();

  final FirebaseStorage _storage = FirebaseStorage.instance;

  Future<String> getUserName(String userId) async {
    String username = "";
    var source = await _ref.ref('users').child(userId).child("fullName").once();
    var username1 = source.snapshot;
    return username;
  }

  Future<bool> writeServiceToFirebase(
      String serviceId, ProviderServiceModel providerServiceModel) async {
    try {
      await _ref
          .ref("providerServices")
          .child(serviceId)
          .set(providerServiceModel.toJson());
    } catch (e) {
      print(e);
    }

    return false;
  }

  Future<bool> deleteServiceById(String serviceId) async {
    try {
      await _ref.ref("providerServices").child(serviceId).remove();
      return true;
    } catch (e) {
      print(e);
      return false;
    }
  }

  Future<List<ProviderServiceModel>> getAllServices() async {
    List<ProviderServiceModel> lst = [];
    var source = await _ref.ref("providerServices").get();
    var data = source.children;
    if(data.isEmpty)return lst;
    int i = 0;
    do {
      ProviderServiceModel? providerServiceModel =
          ProviderServiceModel.fromJson(jsonDecode(jsonEncode(data.elementAt(i).value)));
      if(providerServiceModel.userId!=auth.value.uid) {
        var source = await _ref.ref()
            .child("users")
            .child(providerServiceModel.userId)
            .once();
        var values = source.snapshot;
        if(values.value!=null) {
          UserModel? userModel = UserModel.fromJson(
            jsonDecode(
              jsonEncode(values.value),
            ),
          );
          providerServiceModel.provider = userModel;
          double distance = 0.0;
          print('providerServiceModel==${providerServiceModel.location}');
          if (auth.value.location != null &&
              providerServiceModel.location != null) {
            distance = Geolocator.distanceBetween(
              auth.value.location!.latitude,
              auth.value.location!.longitude,
              providerServiceModel.location!.latitude,
              providerServiceModel.location!.longitude,
            ) / 1000; // Convert meters to kilometers
            if (providerServiceModel.location!= null &&
                userModel.uid != auth.value.uid && distance <= radius) {
              providerServiceModel.distance =
              "${distance.toStringAsFixed(2)} mi";
              lst.add(providerServiceModel);
            }
          }
        }
      }
      i++;
    } while (i<data.length);

    return lst;
  }
  Future<List<ProviderServiceModel>> getAllProviderServices(String providerID) async {
    List<ProviderServiceModel> lst = [];
    var source = await _ref.ref("providerServices").orderByChild("userId").equalTo(providerID).get();
    var usersource = await _ref.ref('users')
        .child(providerID)
        .once();
    var values = usersource.snapshot;
    UserModel? userModel = UserModel.fromJson(
      jsonDecode(
        jsonEncode(values.value),
      ),
    );
    var data = source.children;
    int i = 0;
    do {
      ProviderServiceModel? providerServiceModel =
      ProviderServiceModel.fromJson(jsonDecode(jsonEncode(data.elementAt(i).value)));
      providerServiceModel.provider = userModel;
      lst.add(providerServiceModel);
      i++;
    } while (i<data.length);
    return lst;
  }
  Future<List<ProviderServiceModel>> searchServices(String q) async {
    List<ProviderServiceModel> lst = [];
    var source = await _ref.ref("providerServices")
        .get();
    var data = source.children;
    int i = 0;
    do {
      ProviderServiceModel? providerServiceModel =
          ProviderServiceModel.fromJson(jsonDecode(jsonEncode(data.elementAt(i).value)));
      if(providerServiceModel.userId!=auth.value.uid) {
        var source = await _ref.ref()
            .child("users")
            .child(providerServiceModel.userId)
            .once();
        var values = source.snapshot;
        UserModel? userModel = UserModel.fromJson(
          jsonDecode(
            jsonEncode(values.value),
          ),
        );
        providerServiceModel.provider = userModel;
        if(providerServiceModel.provider!.providerUserModel!=null) {
          lst.add(providerServiceModel);
        }
      }
      i++;
    } while (i<data.length);

    return lst;
  }

  Future<List<ProviderServiceModel>> getAllServicesForCurrentUser() async {
    List<ProviderServiceModel> serviceList = [];
    String userId = FirebaseAuth.instance.currentUser!.uid;

    // Step 1: Fetch serviceId strings for the current user from serviceBooking node
    var serviceBookingSource = await _ref
        .ref()
        .child("serviceBooking")
        .orderByChild("clientId")
        .equalTo(userId)
        .once();
    var bookingData = serviceBookingSource.snapshot;

    if (bookingData.value != null) {
      List<String> serviceIds = [];
      bookingData.children.forEach((element) {
        var serviceBookingModel = ServiceBookingModel.fromJson(
          jsonDecode(
            jsonEncode(element.value),
          ),
        );
        serviceIds.add(serviceBookingModel.serviceId!);
      });

      // Step 2: Fetch each ProviderServiceModel using the list of serviceId strings
      for (String serviceId in serviceIds) {
        var providerServiceSource =
            await _ref.ref().child("providerServices").child(serviceId).once();
        var serviceData = providerServiceSource.snapshot;
        if (serviceData.value != null) {
          var providerServiceModel = ProviderServiceModel.fromJson(
            jsonDecode(
              jsonEncode(serviceData.value),
            ),
          );
          serviceList.add(providerServiceModel);
        }
      }
    }

    return serviceList;
  }

  Future<List<ProviderServiceModel>> getAllServicesByUserId(
      String userId) async {
    List<ProviderServiceModel> lst = [];
    var source = await _ref
        .ref("providerServices")
        .orderByChild("userId")
        .equalTo(userId)
        .once();
    var data = source.snapshot;

    if (data.value != null) {
      data.children.forEach((e) {
        ProviderServiceModel? providerServiceModel =
            ProviderServiceModel.fromJson(
          jsonDecode(
            jsonEncode(e.value),
          ),
        );
        lst.add(providerServiceModel);
      });
    }

    return lst;
  }

  Future<List<ProviderServiceModel>> filterAllServicesByUserId(
      String userId,String type) async {
    List<ProviderServiceModel> lst = [];
    var source = await _ref
        .ref("providerServices")
        .orderByChild("userId")
        .equalTo(userId)
        .once();
    var data = source.snapshot;

    if (data.value != null) {
      data.children.where((element) =>element.child('serviceType').value==type,).forEach((e) {
        ProviderServiceModel? providerServiceModel =
        ProviderServiceModel.fromJson(
          jsonDecode(
            jsonEncode(e.value),
          ),
        );
        lst.add(providerServiceModel);
      });
    }

    return lst;
  }
  Future<ProviderServiceModel> getServiceByServiceId(String serviceId) async {
    var source =
        await _ref.ref().child("providerServices").child(serviceId).once();
    ProviderServiceModel? providerServiceModel;
    var data = source.snapshot;
    if (data.value != null) {
      ProviderServiceModel providerServiceModelJ =
          ProviderServiceModel.fromJson(
        jsonDecode(
          jsonEncode(data.value),
        ),
      );
      providerServiceModel = providerServiceModelJ;
    }
    return providerServiceModel!;
  }

  Future<String> uploadImage(File image, String userID) async {
    try {
      String fileName =
          'users/$userID/providerServiceImages/${DateTime.now().millisecondsSinceEpoch}.jpg';
      Reference ref = _storage.ref().child(fileName);
      UploadTask uploadTask = ref.putFile(image);
      TaskSnapshot snapshot = await uploadTask;
      return await snapshot.ref.getDownloadURL();
    } catch (e) {
      print(e);
      return '';
    }
  }

  Future<bool> writeProviderServiceRequestOfferToFirebase(
      ProviderServiceRequestOfferModel providerServiceRequestModel,
      String requestId) async {
    try {
      _ref
          .ref()
          .child("providerServiceRequestOffer")
          .child(requestId)
          .set(providerServiceRequestModel.toJson());
    } catch (e) {
      print(e);
    }

    return false;
  }

  Future<List<ProviderServiceRequestOfferModel>> getRequestOfferByServiceId(String serviceId) async {
    List<ProviderServiceRequestOfferModel> lst = [];
    var source = await _ref
        .ref("providerServiceRequestOffer")
        .orderByChild("serviceId")
        .equalTo(serviceId)
        .once();
    var data = source.snapshot;

    if (data.value != null) {
      data.children.forEach((e) {
        ProviderServiceRequestOfferModel? providerServiceRequestOfferModel =
            ProviderServiceRequestOfferModel.fromJson(
          jsonDecode(
            jsonEncode(e.value),
          ),
        );
        lst.add(providerServiceRequestOfferModel);
      });
    }

    return lst;
  }

  Future<ProviderServiceRequestOfferModel?> getSingleRequestOfferByRequestId(
      String requestId) async {
    var source =
        await _ref.ref("providerServiceRequestOffer/$requestId").once();
    var data = source.snapshot;

    if (data.value != null) {
      ProviderServiceRequestOfferModel? providerServiceRequestOfferModel =
          ProviderServiceRequestOfferModel.fromJson(
        jsonDecode(
          jsonEncode(data.value),
        ),
      );
      return providerServiceRequestOfferModel;
    }

    return null;
  }

  Future<bool> hasUserMadeOffer(String userId, String serviceId) async {
    var source = await _ref
        .ref('providerServiceRequestOffer')
        .orderByChild('providerId')
        .equalTo(userId)
        .once();
    var data = source.snapshot;

    if (data.value != null) {
      for (var child in data.children) {
        var offer = ProviderServiceRequestOfferModel.fromJson(
          jsonDecode(
            jsonEncode(child.value),
          ),
        );
        if (offer.serviceId == serviceId) {
          return true;
        }
      }
    }
    return false;
  }

  Future<ProviderServiceModel> getService(String serviceId) async {
    final snapshot =
        await _databaseReference.child('providerServices').child(serviceId).get();
    if (snapshot.exists) {
      return ProviderServiceModel.fromJson(
          Map<String, dynamic>.from(snapshot.value as Map));
    } else {
      throw Exception('Service not found');
    }
  }
  Future<UserModel> getProvider(String providerId) async {
    var source = await _databaseReference.child("users")
        .child(providerId)
        .once();
    var values = source.snapshot;
    return UserModel.fromJson(
      jsonDecode(
        jsonEncode(values.value),
      ),
    );
  }

  Future<List<ProviderServiceModel>> getServicesByIds(
      List<String> serviceIds) async {
    List<ProviderServiceModel> services = [];
    for (String serviceId in serviceIds) {
      ProviderServiceModel service = await getService(serviceId);
      services.add(service);
    }
    return services;
  }
  Future<List<UserModel>> getProviderByIds(List<String> serviceIds) async {
    List<UserModel> services = [];
    for (String serviceId in serviceIds) {
      UserModel? service = await getProvider(serviceId);
        services.add(service);

    }
    return services;
  }

  Future<bool> updateServiceEdit(String serviceId, List images,
      String description, int servicePrice) async {
    try {
      await _ref.ref("providerServices").child(serviceId).update({
        "serviceImages": images,
        "description": description,
        "servicePrice": servicePrice
      });

      return true;
    } catch (e) {
      print(e);
    }
    return false;
  }
  Query getReviewListByProviderId(providerId){
    return _ref.ref("ratings").orderByChild("providerId").equalTo(providerId);
  }
  Query getReviewListByServiceId(serviceId){
    return _ref.ref("ratings").orderByChild("serviceId").equalTo(serviceId);
  }
  Future<ProviderServiceRequestOfferModel?> getProviderOffer(String serviceId,String providerId) async {
    print(serviceId);
    ProviderServiceRequestOfferModel? offerModel;
    var source = await _ref
        .ref("providerServiceRequestOffer")
        .orderByChild("serviceId")
        .equalTo(serviceId)
        .once();
    var data = source.snapshot;

    if (data.value != null) {
      data.children.forEach((e) {
        ProviderServiceRequestOfferModel? providerServiceRequestOfferModel =
        ProviderServiceRequestOfferModel.fromJson(
          jsonDecode(
            jsonEncode(e.value),
          ),
        );
        if(providerServiceRequestOfferModel.providerId==providerId){
          offerModel= providerServiceRequestOfferModel;
          return;
        }
      });
    }

    return offerModel;
  }
}
