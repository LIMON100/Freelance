import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/data_models/notification_model.dart';
import 'package:groom/firebase/notifications_firebase.dart';
import 'package:groom/repo/setting_repo.dart';

import 'package:groom/widgets/loading_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';

import '../../firebase/customer_offer_firebase.dart';
import '../../states/customer_request_state.dart';
import '../customer/booking/customer_booking_service_request_list_screen.dart';

class NotificationScreen extends StatefulWidget {
  const NotificationScreen({super.key});

  @override
  State<NotificationScreen> createState() => _NotificationScreenState();
}

class _NotificationScreenState extends State<NotificationScreen> {
  NotificationsFirebase notificationsFirebase = NotificationsFirebase();
  int _currentTab = 0; // 0 for All, 1 for Not Received

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Column(
        children: [
          SizedBox(height: 25),
          _buildHeader(),
          SizedBox(height: 8),
          _buildTabBar(),
          Expanded(child: _buildNotificationList()),
        ],
      ),
    );
  }

  Widget _buildHeader() {
    return Padding(
      padding: const EdgeInsets.all(10.0),
      child: Row(
        children: [
          _buildBackButton(),
          Text(
            'Notifications',
            style: TextStyle(
              fontSize: 16,
              fontWeight: FontWeight.w700,
              height: 20.16 / 16, // Line height divided by font size
              textBaseline: TextBaseline.alphabetic,
              decoration: TextDecoration.none, // No decoration applied
              decorationStyle: TextDecorationStyle.solid,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildBackButton() {
    return Container(
      height: 50,
      width: 50,
      // margin: EdgeInsets.only(left: 8, top: 16),
      decoration: BoxDecoration(
        color: Color(0xF5F4F8).withOpacity(1),
        shape: BoxShape.circle,
      ),
      child: IconButton(
        icon: Icon(Icons.arrow_back_ios, color: Colors.black, size: 18),
        onPressed: () => Navigator.pop(context),
      ),
    );
  }

  Widget _buildDeleteButton() {
    return Container(
      height: 50,
      width: 50,
      margin: EdgeInsets.only(right: 8, top: 16),
      decoration: BoxDecoration(
        color: Color(0xF5F4F8).withOpacity(1),
        shape: BoxShape.circle,
      ),
      child: IconButton(
        icon: Icon(Icons.delete, color: Colors.black, size: 18),
        onPressed: () {
          // Handle delete action
        },
      ),
    );
  }

  Widget _buildTabBar() {
    return Padding(
      padding: const EdgeInsets.only(left: 10.0, right: 10),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          _buildTab('All', 0),
          SizedBox(
            width: 10,
          ),
          _buildTab('Not Received', 1),
        ],
      ),
    );
  }

  Widget _buildTab(String title, int tabIndex) {
    return GestureDetector(
      onTap: () {
        setState(() {
          _currentTab = tabIndex;
        });
      },
      child: Container(
        padding: EdgeInsets.symmetric(vertical: 10, horizontal: 20),
        decoration: BoxDecoration(
          color: _currentTab == tabIndex
              ? Color(0x363062).withOpacity(1)
              : Color(0xF5F4F8).withOpacity(1),
          borderRadius: BorderRadius.circular(20),
        ),
        child: Text(
          title,
          style: TextStyle(
            fontSize: 10,
            fontWeight: FontWeight.w500,
            height: 11.74 / 10, // line height divided by font size
            letterSpacing: 0.03,
            textBaseline: TextBaseline.alphabetic,

            color: _currentTab == tabIndex
                ? Colors.white
                : Color(0x252B5C).withOpacity(1),
          ),
        ),
      ),
    );
  }

  Widget _buildNotificationList() {
    return FutureBuilder(
      future: notificationsFirebase.getNotificationsByUserId(auth.value.uid),
      builder: (context, snapshot) {
        if (snapshot.connectionState == ConnectionState.waiting) {
          return LoadingWidget(type: LoadingType.LIST);
        } else if (snapshot.hasError) {
          return Text("Error: ${snapshot.error}");
        } else if (snapshot.hasData) {
          var notifications = snapshot.data as List<NotificationModel>;

          // Separate notifications into today and older
          DateTime now = DateTime.now();
          DateTime startOfToday = DateTime(now.year, now.month, now.day);

          var todayNotifications = notifications
              .where((n) => DateTime.fromMillisecondsSinceEpoch(n.createdOn)
                  .isAfter(startOfToday))
              .toList();
          var olderNotifications = notifications
              .where((n) => DateTime.fromMillisecondsSinceEpoch(n.createdOn)
                  .isBefore(startOfToday))
              .toList();

          List<Widget> notificationWidgets = [];

          // Add today's notifications
          if (todayNotifications.isNotEmpty) {
            notificationWidgets.add(
              Padding(
                padding: const EdgeInsets.symmetric(vertical: 8),
                child: Text(
                  'Today',
                  style: TextStyle(
                      fontWeight: FontWeight.bold,
                      fontSize: 18,
                      color: Color(0x252B5C).withOpacity(1)),
                ),
              ),
            );
            notificationWidgets.addAll(todayNotifications.map((notification) {
              return _buildNotificationCard(notification);
            }));
          }

          // Add older notifications
          if (olderNotifications.isNotEmpty) {
            notificationWidgets.add(
              Padding(
                padding: const EdgeInsets.symmetric(vertical: 8),
                child: Text(
                  'Older Notifications',
                  style: TextStyle(
                      fontWeight: FontWeight.bold,
                      fontSize: 18,
                      color: Color(0x252B5C).withOpacity(1)),
                ),
              ),
            );
            notificationWidgets.addAll(olderNotifications.map((notification) {
              return _buildNotificationCard(notification);
            }));
          }

          if (notificationWidgets.isEmpty) {
            return Center(child: SubTxtWidget('No Notification Found'));
          }

          return Padding(
            padding: const EdgeInsets.all(16.0),
            child: ListView(children: notificationWidgets),
          );
        }
        return SubTxtWidget('No Notification Found');
      },
    );
  }

  Widget _buildNotificationCard(NotificationModel notification) {
    return Card(
      color: Color(0xF5F4F8).withOpacity(1),
      margin: const EdgeInsets.symmetric(vertical: 8),
      child: ListTile(
        contentPadding: const EdgeInsets.all(16.0),
        leading: CircleAvatar(
          backgroundImage: NetworkImage(''), // Placeholder for user image
        ),
        title: Text(notification.sendBy,
            style: TextStyle(
              fontSize: 12,
              color: Color(0x252B5C).withOpacity(1),
              fontWeight: FontWeight.w700,
              height: 14.09 / 12, // line height divided by font size
              letterSpacing: 0.03,
              textBaseline: TextBaseline.alphabetic,

              decorationStyle: TextDecorationStyle.solid,
              // Requires manual implementation
            )),
        subtitle: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              notification.notificationText,
              style: TextStyle(
                  color: Color(0x53587A).withOpacity(1),
                  fontSize: 10,
                  fontWeight: FontWeight.w500,
                  height: 20 / 10, // line-height as a factor
                  letterSpacing: 0.03,
                  decoration: TextDecoration.none),
            ),
            SizedBox(height: 4),
            Text(
              getRelativeTime(notification.createdOn),
              style: TextStyle(
                fontSize: 10,
                fontWeight: FontWeight.w400,
                height: 17 / 8, // line-height as a factor
                letterSpacing: -0.02,

                decoration: TextDecoration.none,
              ),
            ),
          ],
        ),
        onTap: () {
          if (notification.sendBy == "USER") {
            // Handle user notification
          } else {
            userClientEvent(notification);
          }
        },
      ),
    );
  }

  Future<void> userClientEvent(NotificationModel noti) async {
    CustomerOfferFirebase customerOfferFirebase = CustomerOfferFirebase();
    CustomerRequestState customerRequestState = Get.put(CustomerRequestState());
    if (noti.type == "OFFER") {
      customerRequestState.selectedCustomerRequest.value =
          await customerOfferFirebase.getOfferByOfferId(noti.serviceId);
      Get.to(() => const CustomerServiceRequestViewScreen());
    }
  }

  // Function to calculate relative time
  String getRelativeTime(int timestamp) {
    final DateTime notificationTime =
        DateTime.fromMillisecondsSinceEpoch(timestamp);
    final Duration difference = DateTime.now().difference(notificationTime);

    if (difference.inMinutes < 1) {
      return 'Just now';
    } else if (difference.inHours == 0) {
      return '${difference.inMinutes} minute${difference.inMinutes > 1 ? 's' : ''} ago';
    } else if (difference.inDays == 0) {
      return '${difference.inHours} hour${difference.inHours > 1 ? 's' : ''} ago';
    } else {
      return '${difference.inDays} day${difference.inDays > 1 ? 's' : ''} ago';
    }
  }
}
